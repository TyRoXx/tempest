#ifndef TEMPEST_TCP_ACCEPTOR_HPP
#define TEMPEST_TCP_ACCEPTOR_HPP


#include "client.hpp"
#include <tempest/config.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <memory>


namespace tempest
{
	struct tcp_acceptor TEMPEST_FINAL
	{
		typedef movable_ptr<abstract_client>::type client_ptr;
		typedef boost::function<void (client_ptr &)>
			client_handler;

		explicit tcp_acceptor(boost::uint16_t port,
		                      client_handler on_client,
		                      boost::asio::io_service &io_service);

	private:

		boost::asio::ip::tcp::acceptor m_impl;
		const client_handler m_on_client;
		movable_ptr<boost::asio::ip::tcp::iostream>::type m_next_client;

		void begin_accept();
		void handle_accept(boost::system::error_code error);
	};

}


#endif
