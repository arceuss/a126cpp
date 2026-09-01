#pragma once

#include <string>
#include <vector>

#include "java/Type.h"

namespace HttpGet
{

struct Response
{
	// 0 when the request never produced a status line.
	int_t status = 0;
	std::vector<byte_t> body;
	std::string error;

	bool ok() const
	{
		return error.empty() && status >= 200 && status < 400;
	}
};

// Plain HTTP/1.1 GET over a blocking TCP socket. Only what the skin fetch
// needs: no TLS, no keep-alive, no redirects, no chunked request bodies.
// Chunked *responses* are decoded because servers may use them.
//
// url is "http://host[:port]/path". Never throws; failures are reported in
// Response::error.
Response fetch(const std::string &url, int_t timeoutSeconds = 30);

}
