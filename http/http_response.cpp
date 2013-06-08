#include "http_response.hpp"


namespace tempest
{
	void print_response(http_response const &response, std::ostream &out)
	{
		//HTTP/1.1 200 OK\r\n
		out << response.version << ' '
		    << response.status << ' '
		    << response.reason << "\r\n";

		//key: value\r\n
		for (auto const &header : response.headers)
		{
			out << header.first << ": " << header.second << "\r\n";
		}

		out << "\r\n";
	}
}
