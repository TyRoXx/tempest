#ifndef TEMPEST_TCP_CLIENT_HPP
#define TEMPEST_TCP_CLIENT_HPP


#include "client.hpp"
#include <tempest/config.hpp>
#include <memory>
#include <boost/asio/ip/tcp.hpp>


namespace tempest
{
	struct tcp_client : public abstract_client, private sender, private receiver
	{
		typedef movable_ptr<boost::asio::ip::tcp::iostream>::type socket_ptr;

		explicit tcp_client(socket_ptr socket);
		virtual void shutdown() TEMPEST_OVERRIDE;
		virtual sender &get_sender() TEMPEST_OVERRIDE;
		virtual receiver &get_receiver() TEMPEST_OVERRIDE;

	private:

		const socket_ptr m_stream;


		virtual std::ostream &response() TEMPEST_OVERRIDE;
		virtual boost::optional<int> posix_response() TEMPEST_OVERRIDE;

		virtual std::istream &request() TEMPEST_OVERRIDE;
	};
}


#endif
