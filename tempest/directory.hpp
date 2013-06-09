#ifndef TEMPEST_DIRECTORY_HPP
#define TEMPEST_DIRECTORY_HPP


#include <string>


namespace tempest
{
	struct http_request;
	struct sender;

	struct directory
	{
		virtual ~directory();
		virtual void respond(http_request const &request,
		                     std::string const &sub_path,
		                     sender &sender) = 0;
	};
}


#endif
