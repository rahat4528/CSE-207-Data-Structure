#include "Exporter.h"
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace crawler {

std::string Exporter::escapeJsonString(std::string_view str) {
    std::string out;
    out.reserve(str.size() + 16);
    for (char c : str) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char hexBuf[8];
                    snprintf(hexBuf, sizeof(hexBuf), "\\u%04x", static_cast<unsigned char>(c));
                    out += hexBuf;
                } else {
                    out.push_back(c);
                }
                break;
        }
    }
    return out;
}

std::string Exporter::escapeCsvCell(std::string_view cell) {
    std::string s(cell);

    // Formula injection mitigation: if cell begins with =, +, -, @, prefix with '
    if (!s.empty() && (s[0] == '=' || s[0] == '+' || s[0] == '-' || s[0] == '@')) {
        s.insert(0, "'");
    }

    bool needsQuotes = s.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuotes) {
        return s;
    }

    std::string out = "\"";
    for (char c : s) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out.push_back(c);
        }
    }
    out += "\"";
    return out;
}

Result<void> Exporter::exportJson(const std::string& filePath, const CrawlerEngine& engine) {
    std::filesystem::path p(filePath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream ofs(filePath);
    if (!ofs.is_open()) {
        return Result<void>::error(StatusCode::IO_ERROR, "Cannot write JSON to: " + filePath);
    }

    ofs << "{\n";
    ofs << "  \"crawler\": \"SearchBot/1.0\",\n";
    ofs << "  \"totalCrawled\": " << engine.crawledCount() << ",\n";
    ofs << "  \"pages\": [\n";

    auto pages = engine.storage().getAllPages();
    for (size_t i = 0; i < pages.size(); ++i) {
        const auto& page = pages[i];
        ofs << "    {\n";
        ofs << "      \"url\": \"" << escapeJsonString(page.url) << "\",\n";
        ofs << "      \"statusCode\": " << page.statusCode << ",\n";
        ofs << "      \"depth\": " << page.depth << ",\n";
        ofs << "      \"title\": \"" << escapeJsonString(page.title) << "\",\n";
        ofs << "      \"metaDescription\": \"" << escapeJsonString(page.metaDescription) << "\",\n";
        ofs << "      \"contentLength\": " << page.contentLength << ",\n";
        ofs << "      \"contentHash\": \"" << page.contentHash << "\",\n";
        ofs << "      \"simHash\": \"" << page.simHash << "\",\n";
        ofs << "      \"outLinks\": [";
        for (size_t j = 0; j < page.outLinks.size(); ++j) {
            ofs << "\"" << escapeJsonString(page.outLinks[j]) << "\"";
            if (j + 1 < page.outLinks.size()) ofs << ", ";
        }
        ofs << "]\n";
        ofs << "    }" << (i + 1 < pages.size() ? "," : "") << "\n";
    }

    ofs << "  ]\n";
    ofs << "}\n";
    return Result<void>::ok();
}

Result<void> Exporter::exportCsv(const std::string& filePath, const CrawlerEngine& engine) {
    std::filesystem::path p(filePath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream ofs(filePath);
    if (!ofs.is_open()) {
        return Result<void>::error(StatusCode::IO_ERROR, "Cannot write CSV to: " + filePath);
    }

    // CSV Header
    ofs << "url,statusCode,depth,title,contentLength,contentHash,simHash,outLinksCount\n";

    auto pages = engine.storage().getAllPages();
    for (const auto& page : pages) {
        ofs << escapeCsvCell(page.url) << ","
            << page.statusCode << ","
            << page.depth << ","
            << escapeCsvCell(page.title) << ","
            << page.contentLength << ","
            << page.contentHash << ","
            << page.simHash << ","
            << page.outLinks.size() << "\n";
    }

    return Result<void>::ok();
}

} // namespace crawler

