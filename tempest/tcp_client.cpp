#include "tcp_client.hpp"


namespace tempest
{
	tcp_client::tcp_client(std::unique_ptr<boost::asio::ip::tcp::iostream> stream)
	    : m_stream(std::move(stream))
	{
	}

	void tcp_client::shutdown()
	{
		m_stream->rdbuf()->shutdown(boost::asio::socket_base::shutdown_both);
	}

	sender &tcp_client::get_sender()
	{
		return *this;
	}

	receiver &tcp_client::get_receiver()
	{
		return *this;
	}

	std::ostream &tcp_client::response()
	{
		return *m_stream;
	}

	boost::optional<int> tcp_client::posix_response()
	{
#if TEMPEST_USE_POSIX
		int const fd = m_stream->rdbuf()->native_handle();
		return fd;
#else
		return boost::optional<int>();
#endif
	}

	std::istream &tcp_client::request()
	{
		return *m_stream;
	}
}
