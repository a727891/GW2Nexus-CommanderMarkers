#include "services/HttpClient.h"

#include "core/Branding.h"

#include <windows.h>
#include <winhttp.h>

#include <vector>

namespace cm {

namespace {

std::wstring ToWide(const std::string& value) {
    if (value.empty()) return L"";
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 0) return L"";
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, wide.data(), size);
    if (!wide.empty() && wide.back() == L'\0') {
        wide.pop_back();
    }
    return wide;
}

bool ParseUrl(const std::string& url, std::wstring& host, std::wstring& path, bool& secure) {
    secure = url.rfind("https://", 0) == 0;
    const auto schemeLen = secure ? 8u : (url.rfind("http://", 0) == 0 ? 7u : 0u);
    if (schemeLen == 0) return false;

    const auto withoutScheme = url.substr(schemeLen);
    const auto slash = withoutScheme.find('/');
    const auto hostPart = slash == std::string::npos ? withoutScheme : withoutScheme.substr(0, slash);
    path = slash == std::string::npos ? L"/" : ToWide(withoutScheme.substr(slash));
    host = ToWide(hostPart);
    return !host.empty();
}

bool UsesModuleUserAgent(const std::wstring& host) {
    return host == L"bhm.blishhud.com" || host == L"assets.gw2dat.com";
}

}  // namespace

HttpResponse HttpGetUrlEx(const std::string& url, const HttpRequestOptions& options) {
    HttpResponse response;
    std::wstring host;
    std::wstring path;
    bool secure = false;
    if (!ParseUrl(url, host, path, secure)) {
        return response;
    }

    const bool moduleUserAgent = UsesModuleUserAgent(host);
    const std::wstring sessionAgent =
        moduleUserAgent ? ToWide(kHttpUserAgent) : L"CommanderMarkers-GW2API/1.0.0";

    HINTERNET session = WinHttpOpen(sessionAgent.c_str(),
                                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS,
                                    0);
    if (!session) return response;

    WinHttpSetTimeouts(session, options.connectTimeoutMs, options.connectTimeoutMs,
                       options.readTimeoutMs, options.readTimeoutMs);

    HINTERNET connection =
        WinHttpConnect(session, host.c_str(), secure ? INTERNET_DEFAULT_HTTPS_PORT
                                                     : INTERNET_DEFAULT_HTTP_PORT,
                       0);
    if (!connection) {
        WinHttpCloseHandle(session);
        return response;
    }

    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request =
        WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    std::wstring headers;
    if (moduleUserAgent) {
        headers = L"User-Agent: " + ToWide(kHttpUserAgent) + L"\r\n";
    }
    if (!options.bearerToken.empty()) {
        headers += L"Authorization: Bearer " + ToWide(options.bearerToken) + L"\r\n";
    }

    const BOOL sent = WinHttpSendRequest(
        request, headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
        headers.empty() ? 0 : static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                            WINHTTP_NO_HEADER_INDEX)) {
        response.statusCode = static_cast<int>(statusCode);
    }

    DWORD available = 0;
    do {
        if (!WinHttpQueryDataAvailable(request, &available)) break;
        if (available == 0) break;

        std::vector<char> buffer(available);
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &read)) break;
        response.body.append(buffer.data(), buffer.data() + read);
    } while (available > 0);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return response;
}

std::optional<std::string> HttpGetUrl(const std::string& url,
                                      const HttpRequestOptions& options) {
    const auto response = HttpGetUrlEx(url, options);
    if (response.statusCode < 200 || response.statusCode >= 300 || response.body.empty()) {
        return std::nullopt;
    }
    return response.body;
}

}  // namespace cm
