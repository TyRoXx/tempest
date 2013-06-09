#ifndef TEMPEST_DIRECTORY_HPP
#define TEMPEST_DIRECTORY_HPP


namespace tempest
{
	struct http_request;
	struct sender;

	struct directory
	{
		virtual ~directory();
		virtual void respond(http_request const &request,
		                     sender &sender) = 0;
	};
}


#endif
