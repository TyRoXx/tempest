#ifndef TEMPEST_HTTP_RESPONSE_HPP
#define TEMPEST_HTTP_RESPONSE_HPP


#include <string>
#include <map>
#include <ostream>


namespace tempest
{
	struct http_response
	{
		std::string version;
		unsigned status;
		std::string reason;
		std::map<std::string, std::string> headers;
	};

	void print_response(http_response const &response, std::ostream &out);
}

#endif
