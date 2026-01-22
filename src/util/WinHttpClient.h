#pragma once
#include <string>
#include <vector>

class WinHttpClient {
public:
    WinHttpClient(const std::wstring& userAgent = L"WinHTTP Client");
    ~WinHttpClient();

    // Simple GET request
    bool Get(const std::wstring& url, std::string& responseBody);

    // Simple POST request
    bool Post(const std::wstring& url, const std::string& data, std::string& responseBody);

    int GetLastError() const { return lastError_; }

private:
    std::wstring userAgent_;
    int lastError_ = 0;
};
