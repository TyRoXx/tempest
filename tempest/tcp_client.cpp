#include "tcp_client.hpp"


namespace tempest
{
	tcp_client::tcp_client(socket_ptr stream)
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
		//Boost 1.46.1 does not know native_handle(), so we use native() here
		int const fd = m_stream->rdbuf()->native();
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
