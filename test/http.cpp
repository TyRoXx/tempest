#include <boost/test/unit_test.hpp>
#include "http/http_request.hpp"
#include "http/http_response.hpp"

BOOST_AUTO_TEST_CASE(http_request_parse)
{
	std::istringstream source(
	            "GET /file HTTP/1.0\r\n"
	            "Host: localhost\r\n"
	            "\r\n"
	            );

	auto parsed = tempest::parse_request(source);
	BOOST_CHECK_EQUAL(parsed.method, "GET");
	BOOST_CHECK_EQUAL(parsed.file, "/file");
	BOOST_CHECK_EQUAL(parsed.version, "HTTP/1.0");
	BOOST_CHECK_EQUAL(parsed.headers.size(), 1);
	BOOST_CHECK_EQUAL(parsed.headers["Host"], "localhost");
}

BOOST_AUTO_TEST_CASE(http_response_print)
{
	tempest::http_response response;
	response.version = "HTTP/1.0";
	response.status = 404;
	response.reason = "Not Found";
	response.headers["Content-Length"] = "3";
	response.body = "404";

	std::ostringstream sink;
	print_response(response, sink);

	auto const printed = sink.str();
	BOOST_CHECK_EQUAL(printed,
	                  "HTTP/1.0 404 Not Found\r\n"
	                  "Content-Length: 3\r\n"
	                  "\r\n"
	                  "404"
	                  );
}
