#ifndef TEMPEST_HTTP_REQUEST_HPP
#define TEMPEST_HTTP_REQUEST_HPP


#include <string>
#include <map>
#include <istream>


namespace tempest
{
	typedef std::string url;

	struct http_request
	{
		std::string method;
		url file;
		std::string version;
		std::map<std::string, std::string> headers;
	};

	http_request parse_request(std::istream &source);
}


#endif
