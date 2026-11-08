#include "HttpFetcher.h"
#include "Url.h"
#include "UrlNormalizer.h"
#include "SsrfGuard.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <sstream>
#include <chrono>

namespace crawler {

namespace {

void initSockets() {
#if defined(_WIN32)
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = true;
    }
#endif
}

} // namespace

HttpFetcher::HttpFetcher(bool allowTestLoopback)
    : m_allowTestLoopback(allowTestLoopback) {
    initSockets();
}

HttpFetcher::~HttpFetcher() = default;

Result<FetchResponse> HttpFetcher::fetchSingle(const std::string& targetUrl, 
                                              const FetchRequest& req) {
    auto parsedUrlRes = Url::parse(targetUrl);
    if (parsedUrlRes.isError()) {
        return Result<FetchResponse>::error(parsedUrlRes.code(), parsedUrlRes.errorMessage());
    }

    const Url& url = parsedUrlRes.value();

    if (url.scheme() != "http") {
        // Plain socket fetcher handles http:// directly. For https, FakeFetcher or TLS wrapper applies.
        return Result<FetchResponse>::error(StatusCode::UNSUPPORTED_SCHEME, 
            "Socket fetcher supports http directly. For https use FakeFetcher or TLS tunnel.");
    }

    // DNS Resolution with SSRF Validation
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* resList = nullptr;
    std::string portStr = std::to_string(url.port());
    int dnsRes = getaddrinfo(url.host().c_str(), portStr.c_str(), &hints, &resList);
    if (dnsRes != 0 || !resList) {
        return Result<FetchResponse>::error(StatusCode::HTTP_CLIENT_ERROR, 
            "DNS lookup failed for host: " + url.host());
    }

    // SSRF Check on resolved IP
    char ipBuffer[INET6_ADDRSTRLEN] = {0};
    void* addrPtr = nullptr;
    if (resList->ai_family == AF_INET) {
        addrPtr = &reinterpret_cast<struct sockaddr_in*>(resList->ai_addr)->sin_addr;
        inet_ntop(AF_INET, addrPtr, ipBuffer, sizeof(ipBuffer));
    } else if (resList->ai_family == AF_INET6) {
        addrPtr = &reinterpret_cast<struct sockaddr_in6*>(resList->ai_addr)->sin6_addr;
        inet_ntop(AF_INET6, addrPtr, ipBuffer, sizeof(ipBuffer));
    }

    std::string ipStr(ipBuffer);
    auto ssrfCheck = SsrfGuard::validateIp(ipStr, m_allowTestLoopback);
    if (ssrfCheck.isError()) {
        freeaddrinfo(resList);
        return Result<FetchResponse>::error(ssrfCheck.code(), ssrfCheck.errorMessage());
    }

    // Create socket and set timeouts
#if defined(_WIN32)
    SOCKET sock = socket(resList->ai_family, resList->ai_socktype, resList->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(resList);
        return Result<FetchResponse>::error(StatusCode::IO_ERROR, "Failed to create socket");
    }

    DWORD timeoutMs = req.totalTimeoutMs;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
    int sock = socket(resList->ai_family, resList->ai_socktype, resList->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(resList);
        return Result<FetchResponse>::error(StatusCode::IO_ERROR, "Failed to create socket");
    }

    struct timeval tv{};
    tv.tv_sec = req.totalTimeoutMs / 1000;
    tv.tv_usec = (req.totalTimeoutMs % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    // Connect to pinned IP
    if (connect(sock, resList->ai_addr, static_cast<int>(resList->ai_addrlen)) != 0) {
        freeaddrinfo(resList);
#if defined(_WIN32)
        closesocket(sock);
#else
        close(sock);
#endif
        return Result<FetchResponse>::error(StatusCode::CONNECT_TIMEOUT, "Connection failed to " + ipStr);
    }
    freeaddrinfo(resList);

    // Format HTTP Request
    std::string pathAndQuery = url.path().empty() ? "/" : url.path();
    if (!url.query().empty()) pathAndQuery += "?" + url.query();

    std::ostringstream reqStream;
    reqStream << "GET " << pathAndQuery << " HTTP/1.1\r\n"
              << "Host: " << url.host() << "\r\n"
              << "User-Agent: SearchBot/1.0 (+https://github.com/crawler/CSE-207-Data-Structure)\r\n"
              << "Connection: close\r\n"
              << "Accept: text/html,application/xhtml+xml\r\n\r\n";

    std::string reqStr = reqStream.str();
    send(sock, reqStr.c_str(), static_cast<int>(reqStr.size()), 0);

    // Read Response
    std::string rawResponse;
    char buffer[4096];
    int bytesRead = 0;
    while ((bytesRead = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        rawResponse.append(buffer, static_cast<size_t>(bytesRead));
        if (rawResponse.size() > req.maxSizeBytes + 8192) {
            break; // Exceeded max response limit
        }
    }

#if defined(_WIN32)
    closesocket(sock);
#else
    close(sock);
#endif

    if (rawResponse.empty()) {
        return Result<FetchResponse>::error(StatusCode::READ_TIMEOUT, "Empty response or timeout");
    }

    // Parse HTTP response headers & body
    size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return Result<FetchResponse>::error(StatusCode::PARSE_ERROR, "Malformed HTTP response headers");
    }

    std::string headerPart = rawResponse.substr(0, headerEnd);
    std::string bodyPart = rawResponse.substr(headerEnd + 4);

    FetchResponse resp;
    resp.finalUrl = targetUrl;

    std::istringstream hStream(headerPart);
    std::string statusLine;
    if (std::getline(hStream, statusLine)) {
        std::istringstream sStream(statusLine);
        std::string httpVersion;
        sStream >> httpVersion >> resp.statusCode;
        std::getline(sStream, resp.statusText);
        if (!resp.statusText.empty() && resp.statusText.front() == ' ') {
            resp.statusText.erase(0, 1);
        }
    }

    std::string headerLine;
    while (std::getline(hStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
        size_t colon = headerLine.find(':');
        if (colon != std::string::npos) {
            std::string key = headerLine.substr(0, colon);
            std::string val = headerLine.substr(colon + 1);
            while (!val.empty() && val.front() == ' ') val.erase(0, 1);
            resp.headers[key] = val;
        }
    }

    if (resp.headers.contains("Content-Type")) {
        resp.contentType = resp.headers["Content-Type"];
    }

    if (bodyPart.size() > req.maxSizeBytes) {
        resp.body = bodyPart.substr(0, req.maxSizeBytes);
    } else {
        resp.body = std::move(bodyPart);
    }

    return Result<FetchResponse>::ok(std::move(resp));
}

Result<FetchResponse> HttpFetcher::fetch(const FetchRequest& req) {
    std::string currentUrl = req.url;
    std::vector<std::string> redirectChain;

    for (uint32_t hop = 0; hop <= req.maxRedirects; ++hop) {
        auto res = fetchSingle(currentUrl, req);
        if (res.isError()) return res;

        FetchResponse resp = std::move(res.value());

        if (resp.isRedirect() && resp.headers.contains("Location")) {
            redirectChain.push_back(currentUrl);
            std::string loc = resp.headers["Location"];
            auto nextRes = UrlNormalizer::resolve(currentUrl, loc);
            if (nextRes.isError()) return Result<FetchResponse>::error(nextRes.code(), nextRes.errorMessage());

            std::string nextUrl = nextRes.value();
            if (std::find(redirectChain.begin(), redirectChain.end(), nextUrl) != redirectChain.end()) {
                return Result<FetchResponse>::error(StatusCode::MAX_REDIRECTS_EXCEEDED, 
                    "Circular redirect detected: " + nextUrl);
            }

            currentUrl = nextUrl;
            continue;
        }

        resp.redirectChain = std::move(redirectChain);
        return Result<FetchResponse>::ok(std::move(resp));
    }

    return Result<FetchResponse>::error(StatusCode::MAX_REDIRECTS_EXCEEDED, 
        "Maximum redirects exceeded (" + std::to_string(req.maxRedirects) + ")");
}

} // namespace crawler

