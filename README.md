# Cpp-HttpClient
A simple C++ HTTP client library for Windows, using WinHTTP API. This library provides an easy-to-use interface for making HTTP requests, supporting various HTTP methods and JSON data handling.

## Description

**HttpClientLib** is a lightweight HTTP client library designed for Windows platforms using Visual C++. It leverages the WinHTTP API to perform HTTP and HTTPS requests. The library supports common HTTP methods such as GET, POST, PUT, DELETE, PATCH, HEAD, and OPTIONS. It also integrates with the [nlohmann/json](https://github.com/nlohmann/json) library for convenient JSON handling.

## Features

- Supports HTTP methods: GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS
- Easy integration with JSON data using `nlohmann::json`
- Simple and clean API
- Utilizes RAII for resource management
- Error handling with detailed messages
- Supports HTTPS requests

## Requirements

- **Operating System**: Windows
- **Compiler**: Microsoft Visual C++ (MSVC)
- **C++ Standard**: C++20
- **Libraries**:
  - WinHTTP (Included with Windows SDK)
  - [nlohmann/json](https://github.com/nlohmann/json) (Include `json.hpp` in your project)

## Example Usage

Below is an example of how to use **HttpClientLib** to perform various HTTP requests:

```cpp
// main.cpp

#include <iostream>
#include "HttpClient.h"
#include "json.hpp"

int main() {
	// Create an instance of the HttpClient
	HttpClientLib::HttpClient client;

	// Base URL for testing
	std::string base_url = "http://httpbin.org";

	// Test GET request
	{
		std::string url = base_url + "/get";
		std::cout << "Testing GET request to: " << url << "\n";

		HttpClientLib::HttpResponse response = client.get(url);

		if (response.is_success()) {
			std::cout << "GET Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Body:\n" << response.body << "\n";
		}
		else {
			std::cerr << "GET Request failed: " << response.error << "\n";
		}
	}

	// Test POST request with form data
	{
		std::string url = base_url + "/post";
		std::cout << "\nTesting POST request to: " << url << "\n";

		std::string data = "field1=value1&field2=value2";
		std::unordered_map<std::string, std::string> headers = {
			{"Content-Type", "application/x-www-form-urlencoded"}
		};

		HttpClientLib::HttpResponse response = client.post(url, data, headers);

		if (response.is_success()) {
			std::cout << "POST Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Body:\n" << response.body << "\n";
		}
		else {
			std::cerr << "POST Request failed: " << response.error << "\n";
		}
	}

	// Test POST request with JSON data
	{
		std::string url = base_url + "/post";
		std::cout << "\nTesting POST JSON request to: " << url << "\n";

		nlohmann::json jsonData = {
			{"name", "John Doe"},
			{"age", 30},
			{"city", "New York"}
		};

		HttpClientLib::HttpResponse response = client.postJson(url, jsonData);

		if (response.is_success()) {
			std::cout << "POST JSON Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Body:\n" << response.body << "\n";
		}
		else {
			std::cerr << "POST JSON Request failed: " << response.error << "\n";
		}
	}

	// Test PUT request
	{
		std::string url = base_url + "/put";
		std::cout << "\nTesting PUT request to: " << url << "\n";

		std::string data = "key=updated_value";
		HttpClientLib::HttpResponse response = client.put(url, data);

		if (response.is_success()) {
			std::cout << "PUT Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Body:\n" << response.body << "\n";
		}
		else {
			std::cerr << "PUT Request failed: " << response.error << "\n";
		}
	}

	// Test DELETE request
	{
		std::string url = base_url + "/delete";
		std::cout << "\nTesting DELETE request to: " << url << "\n";

		HttpClientLib::HttpResponse response = client.del(url);

		if (response.is_success()) {
			std::cout << "DELETE Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Body:\n" << response.body << "\n";
		}
		else {
			std::cerr << "DELETE Request failed: " << response.error << "\n";
		}
	}

	// Test HEAD request
	{
		std::string url = base_url + "/get";
		std::cout << "\nTesting HEAD request to: " << url << "\n";

		HttpClientLib::HttpResponse response = client.head(url);

		if (response.is_success()) {
			std::cout << "HEAD Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Headers:\n";
			for (const auto& [key, value] : response.headers) {
				std::cout << key << ": " << value << "\n";
			}
		}
		else {
			std::cerr << "HEAD Request failed: " << response.error << "\n";
		}
	}

	// Test OPTIONS request
	{
		std::string url = base_url + "/get";
		std::cout << "\nTesting OPTIONS request to: " << url << "\n";

		HttpClientLib::HttpResponse response = client.options(url);

		if (response.is_success()) {
			std::cout << "OPTIONS Request succeeded with status " << response.status_code << "\n";
			std::cout << "Allowed Methods: " << response.headers["Allow"] << "\n";
		}
		else {
			std::cerr << "OPTIONS Request failed: " << response.error << "\n";
		}
	}

	// Test PATCH request
	{
		std::string url = base_url + "/patch";
		std::cout << "\nTesting PATCH request to: " << url << "\n";

		std::string data = "key=patched_value";
		HttpClientLib::HttpResponse response = client.patch(url, data);

		if (response.is_success()) {
			std::cout << "PATCH Request succeeded with status " << response.status_code << "\n";
			std::cout << "Response Body:\n" << response.body << "\n";
		}
		else {
			std::cerr << "PATCH Request failed: " << response.error << "\n";
		}
	}

	return 0;
}
```

**Explanation**:

- **Creating the Client**:
  - Instantiate the `HttpClient` class.

- **GET Request**:
  - Use the `get` method to send a GET request.
  - Check if the request was successful with `is_success()`.
  - Output the status code and response body.

- **POST Request with JSON Data**:
  - Create a JSON object using `nlohmann::json`.
  - Use the `postJson` method to send a POST request with JSON data.
  - Output the status code and response body.

- **PUT Request**:
  - Use the `put` method to send a PUT request with data.
  - Output the status code and response body.

- **DELETE Request**:
  - Use the `del` method to send a DELETE request.
  - Output the status code and response body.

## Important Notes

- **Windows Platform**:
  - The library is designed for Windows and uses Windows-specific APIs (WinHTTP).
  - It cannot be compiled or run on non-Windows platforms.

- **C++ Standard**:
  - The code requires **C++20** standard due to modern C++ features used.
  - Ensure your compiler supports C++20.

- **Dependencies**:
  - **WinHTTP**:
    - Part of the Windows SDK; usually included with Visual Studio installations.
  - **nlohmann/json**:
    - Download `json.hpp` from the [official repository](https://github.com/nlohmann/json) and include it in your project.

- **Error Handling**:
  - The `HttpResponse` class includes an `error` field that contains error messages if a request fails.
  - Always check `is_success()` before processing the response.

- **HTTPS Support**:
  - The library supports HTTPS requests.
  - Ensure that the `WINHTTP_FLAG_SECURE` flag is set when making requests to HTTPS URLs.

## License

This library is provided "as is", without warranty of any kind. Use it freely in your projects.

## Contact

For questions or suggestions, please contact the library author.

---

**Disclaimer**: The above code and instructions are provided for educational purposes. Ensure you have the necessary permissions and comply with the relevant licenses when using third-party libraries.
