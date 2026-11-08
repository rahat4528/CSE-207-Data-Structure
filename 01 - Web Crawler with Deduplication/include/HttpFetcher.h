#ifndef CRAWLER_HTTP_FETCHER_H
#define CRAWLER_HTTP_FETCHER_H

#include "IFetcher.h"
#include <string>
#include <memory>

namespace crawler {

class HttpFetcher : public IFetcher {
public:
    explicit HttpFetcher(bool allowTestLoopback = false);
    ~HttpFetcher() override;

    Result<FetchResponse> fetch(const FetchRequest& req) override;

private:
    bool m_allowTestLoopback{false};

    Result<FetchResponse> fetchSingle(const std::string& targetUrl, 
                                     const FetchRequest& req);
};

} // namespace crawler

#endif // CRAWLER_HTTP_FETCHER_H

