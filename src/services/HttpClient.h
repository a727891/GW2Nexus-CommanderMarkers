#pragma once

#include <optional>
#include <string>
#include <vector>

namespace cm {

struct HttpRequestOptions {
    std::string bearerToken;
    std::string ifNoneMatch;
    std::string contentType = "application/json";
    int connectTimeoutMs = 5000;
    int readTimeoutMs = 15000;
};

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::string etag;
};

HttpResponse HttpRequestEx(const std::string& method,
                           const std::string& url,
                           const std::string& body = {},
                           const HttpRequestOptions& options = {});

HttpResponse HttpGetUrlEx(const std::string& url, const HttpRequestOptions& options = {});
std::optional<std::string> HttpGetUrl(const std::string& url,
                                      const HttpRequestOptions& options = {});

HttpResponse HttpPostJsonUrlEx(const std::string& url,
                               const std::string& jsonBody,
                               const HttpRequestOptions& options = {});

}  // namespace cm
