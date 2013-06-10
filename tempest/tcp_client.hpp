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
		explicit tcp_client(std::unique_ptr<boost::asio::ip::tcp::iostream> socket);
		virtual void shutdown() TEMPEST_OVERRIDE;
		virtual sender &get_sender() TEMPEST_OVERRIDE;
		virtual receiver &get_receiver() TEMPEST_OVERRIDE;

	private:

		const std::unique_ptr<boost::asio::ip::tcp::iostream> m_stream;


		virtual std::ostream &response() TEMPEST_OVERRIDE;
		virtual boost::optional<int> posix_response() TEMPEST_OVERRIDE;

		virtual std::istream &request() TEMPEST_OVERRIDE;
	};
}


#endif
