#include "http_request.hpp"
#include "decode_uri.hpp"
#include <stdexcept>
#include <boost/move/utility.hpp>


namespace tempest
{
	namespace
	{
		void trim_trailing_char(std::string &str, char trailing)
		{
			if (!str.empty() &&
			    //use rbegin instead of back to support old implementations
			    *str.rbegin() == trailing)
			{
				str.resize(str.size() - 1);
			}
		}
	}

	http_request parse_request(std::istream &source)
	{
		//throw in case of errors because there is no sensible way to handle
		//errors here
		source.exceptions(std::ios::failbit | std::ios::badbit);

		http_request request;

		//GET /file HTTP/1.1\r\n
		getline(source, request.method, ' ');

		getline(source, request.file, ' ');
		decode_uri(request.file);

		getline(source, request.version, '\n');
		trim_trailing_char(request.version, '\r');

		while (source.peek() != '\r')
		{
			std::string key, value;

			//key: value\r\n
			getline(source, key, ':');
			if (source.peek() == ' ')
			{
				source.get();
			}
			getline(source, value, '\n');
			trim_trailing_char(value, '\r');

			request.headers.insert(
			            std::make_pair(boost::move(key), boost::move(value)));
		}

		//the headers end with \r\n

		//skip \r
		source.get();

		if (source.get() != '\n')
		{
			throw std::runtime_error("Bad request: Missing new-line at the end");
		}

		//TODO: here comes the body..

		return request;
	}
}
