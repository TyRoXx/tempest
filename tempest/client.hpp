#ifndef TEMPEST_CLIENT_HPP
#define TEMPEST_CLIENT_HPP


#include <boost/optional.hpp>
#include <istream>
#include <ostream>


namespace tempest
{
	struct sender
	{
		virtual ~sender();
		virtual std::ostream &response() = 0;
		virtual boost::optional<int> posix_response() = 0;
	};

	struct receiver
	{
		virtual ~receiver();
		virtual std::istream &request() = 0;
	};

	struct abstract_client
	{
		virtual ~abstract_client();
		virtual void shutdown() = 0;
		virtual sender &get_sender() = 0;
		virtual receiver &get_receiver() = 0;
	};
}


#endif
