// HttpClient.h
#pragma once
#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <Windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <regex>
#include <memory>
#include <sstream>
#include "json.hpp"

// Link with WinHTTP library
#pragma comment(lib, "Winhttp.lib")

namespace HttpClientLib {

	// Forward declaration
	class HttpResponse;

	// Helper RAII wrapper for HINTERNET handles
	class WinHttpHandle {
	public:
		WinHttpHandle(HINTERNET handle = nullptr) : handle_(handle) {}
		~WinHttpHandle() {
			if (handle_) {
				WinHttpCloseHandle(handle_);
			}
		}

		// Disable copy
		WinHttpHandle(const WinHttpHandle&) = delete;
		WinHttpHandle& operator=(const WinHttpHandle&) = delete;

		// Enable move
		WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(other.handle_) {
			other.handle_ = nullptr;
		}
		WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
			if (this != &other) {
				if (handle_) {
					WinHttpCloseHandle(handle_);
				}
				handle_ = other.handle_;
				other.handle_ = nullptr;
			}
			return *this;
		}

		HINTERNET get() const { return handle_; }
		void reset(HINTERNET handle = nullptr) {
			if (handle_) {
				WinHttpCloseHandle(handle_);
			}
			handle_ = handle;
		}

	private:
		HINTERNET handle_;
	};

	// Represents an HTTP response
	class HttpResponse {
	public:
		HttpResponse() : status_code(0) {}

		int status_code;
		std::string body;
		std::unordered_map<std::string, std::string> headers;
		std::string error;

		bool is_success() const {
			return status_code >= 200 && status_code < 300 && error.empty();
		}

		// Parses headers from a raw header string
		void parseHeaders(const std::string& raw_headers) {
			headers.clear();
			std::istringstream stream(raw_headers);
			std::string line;
			std::getline(stream, line); // Skip status line
			while (std::getline(stream, line)) {
				if (line.empty() || line == "\r") continue;
				auto delimiter_pos = line.find(':');
				if (delimiter_pos != std::string::npos) {
					std::string key = trim(line.substr(0, delimiter_pos));
					std::string value = trim(line.substr(delimiter_pos + 1));
					headers[key] = value;
				}
			}
		}

	private:
		// Helper function to trim whitespace and carriage return
		std::string trim(const std::string& str) const {
			size_t first = str.find_first_not_of(" \t\r\n");
			size_t last = str.find_last_not_of(" \t\r\n");
			return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
		}
	};

	// The main HttpClient class
	class HttpClient {
	public:
		HttpClient(const std::string& userAgent = "HttpClient/1.0") : user_agent_(userAgent) {}

		HttpResponse get(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("GET", url, "", headers);
		}

		HttpResponse post(const std::string& url, const std::string& data, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("POST", url, data, headers);
		}

		HttpResponse put(const std::string& url, const std::string& data, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("PUT", url, data, headers);
		}

		HttpResponse patch(const std::string& url, const std::string& data, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("PATCH", url, data, headers);
		}

		HttpResponse del(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("DELETE", url, "", headers);
		}

		HttpResponse head(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("HEAD", url, "", headers);
		}

		HttpResponse options(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {}) const {
			return sendRequest("OPTIONS", url, "", headers);
		}

		// Sends a POST request with JSON data
		HttpResponse postJson(const std::string& url, const nlohmann::json& jsonData,
			const std::unordered_map<std::string, std::string>& headers = {}) const {
			std::string data = jsonData.dump();
			auto headersWithContentType = headers;
			headersWithContentType["Content-Type"] = "application/json";
			return sendRequest("POST", url, data, headersWithContentType);
		}

	private:
		std::string user_agent_;

		// Converts UTF-8 string to wide string (UTF-16)
		std::wstring toWideString(const std::string& utf8Str) const {
			if (utf8Str.empty()) return std::wstring();
			int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), NULL, 0);
			if (size_needed == 0) {
				throw std::runtime_error("Failed to convert string to wide string.");
			}
			std::wstring wideStr(size_needed, 0);
			MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), &wideStr[0], size_needed);
			// Remove the null terminator added by MultiByteToWideChar
			if (!wideStr.empty() && wideStr.back() == L'\0') {
				wideStr.pop_back();
			}
			return wideStr;
		}

		// Converts wide string (UTF-16) to UTF-8 string
		std::string toUTF8String(const std::wstring& wideStr) const {
			if (wideStr.empty()) return std::string();
			int size_needed = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), static_cast<int>(wideStr.length()), NULL, 0, NULL, NULL);
			if (size_needed == 0) {
				throw std::runtime_error("Failed to convert wide string to UTF-8 string.");
			}
			std::string utf8Str(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), static_cast<int>(wideStr.length()), &utf8Str[0], size_needed, NULL, NULL);
			return utf8Str;
		}

		// Parses the URL into scheme, host, port, and path
		bool parseUrl(const std::string& url, std::string& scheme, std::string& host,
			unsigned short& port, std::string& path) const {
			std::regex urlRegex(R"((https?)://([^/:]+)(?::(\d+))?([^?]*)?(\?.*)?$)");
			std::smatch match;
			if (std::regex_match(url, match, urlRegex)) {
				scheme = match[1].str();
				host = match[2].str();
				port = match[3].matched ? static_cast<unsigned short>(std::stoi(match[3].str()))
					: (scheme == "https" ? 443 : 80);
				path = match[4].matched ? match[4].str() : "/";
				return true;
			}
			return false;
		}

		// Sends an HTTP request
		HttpResponse sendRequest(const std::string& method, const std::string& url,
			const std::string& data,
			const std::unordered_map<std::string, std::string>& headers) const {
			HttpResponse response;
			try {
				std::string scheme, host, path;
				unsigned short port;
				if (!parseUrl(url, scheme, host, port, path)) {
					response.error = "Invalid URL format.";
					return response;
				}

				bool isHttps = (scheme == "https");

				// Initialize WinHTTP session
				WinHttpHandle hSession(WinHttpOpen(
					toWideString(user_agent_).c_str(),
					WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
					WINHTTP_NO_PROXY_NAME,
					WINHTTP_NO_PROXY_BYPASS, 0));

				if (!hSession.get()) {
					response.error = "WinHttpOpen failed.";
					return response;
				}

				// Connect to server
				WinHttpHandle hConnect(WinHttpConnect(
					hSession.get(),
					toWideString(host).c_str(),
					port,
					0));

				if (!hConnect.get()) {
					response.error = "WinHttpConnect failed.";
					return response;
				}

				// Open request
				WinHttpHandle hRequest(WinHttpOpenRequest(
					hConnect.get(),
					toWideString(method).c_str(),
					toWideString(path).c_str(),
					NULL,
					WINHTTP_NO_REFERER,
					WINHTTP_DEFAULT_ACCEPT_TYPES,
					isHttps ? WINHTTP_FLAG_SECURE : 0));

				if (!hRequest.get()) {
					response.error = "WinHttpOpenRequest failed.";
					return response;
				}

				// Set headers
				std::wstring headerString;
				for (const auto& [key, value] : headers) {
					headerString += toWideString(key) + L": " + toWideString(value) + L"\r\n";
				}
				if (!headerString.empty()) {
					if (!WinHttpAddRequestHeaders(hRequest.get(), headerString.c_str(),
						static_cast<DWORD>(headerString.length()),
						WINHTTP_ADDREQ_FLAG_ADD)) {
						response.error = "WinHttpAddRequestHeaders failed.";
						return response;
					}
				}

				// Send request
				BOOL bResult = WinHttpSendRequest(
					hRequest.get(),
					WINHTTP_NO_ADDITIONAL_HEADERS,
					0,
					(LPVOID)(data.empty() ? NULL : data.c_str()),
					data.empty() ? 0 : static_cast<DWORD>(data.length()),
					data.empty() ? 0 : static_cast<DWORD>(data.length()),
					0);

				if (!bResult) {
					response.error = "WinHttpSendRequest failed.";
					return response;
				}

				// Receive response
				bResult = WinHttpReceiveResponse(hRequest.get(), NULL);
				if (!bResult) {
					response.error = "WinHttpReceiveResponse failed.";
					return response;
				}

				// Get status code
				DWORD dwStatusCode = 0;
				DWORD dwSize = sizeof(dwStatusCode);
				if (WinHttpQueryHeaders(hRequest.get(),
					WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&dwStatusCode,
					&dwSize,
					WINHTTP_NO_HEADER_INDEX)) {
					response.status_code = static_cast<int>(dwStatusCode);
				}
				else {
					response.error = "WinHttpQueryHeaders for status code failed.";
					return response;
				}

				// Get response headers
				DWORD dwHeaderSize = 0;
				WinHttpQueryHeaders(hRequest.get(),
					WINHTTP_QUERY_RAW_HEADERS_CRLF,
					WINHTTP_HEADER_NAME_BY_INDEX,
					NULL,
					&dwHeaderSize,
					WINHTTP_NO_HEADER_INDEX);
				std::vector<wchar_t> headerBuffer(dwHeaderSize / sizeof(wchar_t));
				if (WinHttpQueryHeaders(hRequest.get(),
					WINHTTP_QUERY_RAW_HEADERS_CRLF,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&headerBuffer[0],
					&dwHeaderSize,
					WINHTTP_NO_HEADER_INDEX)) {
					std::wstring headersW(headerBuffer.begin(), headerBuffer.end() - 1); // Remove last null
					std::string headersA = toUTF8String(headersW);
					response.parseHeaders(headersA);
				}

				// Read response body
				std::vector<char> buffer(4096);
				DWORD dwBytesRead = 0;
				std::string responseBody;
				do {
					if (!WinHttpReadData(hRequest.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &dwBytesRead)) {
						response.error = "WinHttpReadData failed.";
						return response;
					}
					responseBody.append(buffer.data(), dwBytesRead);
				} while (dwBytesRead > 0);
				response.body = responseBody;

			}
			catch (const std::exception& ex) {
				response.error = ex.what();
			}

			return response;
		}

	};

} // namespace HttpClientLib

#endif // HTTPCLIENT_H
