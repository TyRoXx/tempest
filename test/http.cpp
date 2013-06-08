#include <boost/test/unit_test.hpp>
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/decode_uri.hpp"

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

namespace
{
	bool test_decode_uri(std::string const &encoded,
	                     std::string const &raw)
	{
		std::string decoded;
		tempest::decode_uri(encoded.begin(),
		                    encoded.end(),
		                    std::back_inserter(decoded));
		return (raw == decoded);
	}

	void decode_invalid(std::string const &invalid)
	{
		std::string decoded;
		tempest::decode_uri(invalid.begin(),
		                    invalid.end(),
		                    std::back_inserter(decoded));
	}
}

BOOST_AUTO_TEST_CASE(http_decode_uri)
{
	BOOST_CHECK(test_decode_uri("", ""));
	BOOST_CHECK(test_decode_uri("abc", "abc"));
	BOOST_CHECK(test_decode_uri("%20", " "));
	BOOST_CHECK(test_decode_uri("a%202", "a 2"));

	BOOST_CHECK_THROW(decode_invalid("%"), std::invalid_argument);
	BOOST_CHECK_THROW(decode_invalid("%1"), std::invalid_argument);
	BOOST_CHECK_THROW(decode_invalid("%%"), std::invalid_argument);
	BOOST_CHECK_THROW(decode_invalid("%2G"), std::invalid_argument);
	BOOST_CHECK_THROW(decode_invalid("%H1"), std::invalid_argument);
}
