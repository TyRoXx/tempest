#ifndef TEMPEST_TCP_CLIENT_HPP
#define TEMPEST_TCP_CLIENT_HPP


#include "client.hpp"
#include <memory>
#include <boost/asio/ip/tcp.hpp>


namespace tempest
{
	struct tcp_client : public abstract_client, private sender, private receiver
	{
		explicit tcp_client(std::unique_ptr<boost::asio::ip::tcp::iostream> socket);
		virtual void shutdown() override;
		virtual sender &get_sender() override;
		virtual receiver &get_receiver() override;

	private:

		const std::unique_ptr<boost::asio::ip::tcp::iostream> m_stream;


		virtual std::ostream &response() override;
		virtual boost::optional<int> posix_response() override;

		virtual std::istream &request() override;
	};
}


#endif
