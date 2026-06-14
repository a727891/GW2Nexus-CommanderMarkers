#pragma once

#include <optional>
#include <string>

namespace cm {

struct HttpRequestOptions {
    std::string bearerToken;
    int connectTimeoutMs = 5000;
    int readTimeoutMs = 15000;
};

struct HttpResponse {
    int statusCode = 0;
    std::string body;
};

HttpResponse HttpGetUrlEx(const std::string& url, const HttpRequestOptions& options = {});
std::optional<std::string> HttpGetUrl(const std::string& url,
                                      const HttpRequestOptions& options = {});

}  // namespace cm
