#include <tempest/tcp_acceptor.hpp>

namespace
{
	void handle_client(tempest::tcp_acceptor::client_ptr &client)
	{

	}
}

int main()
{
	boost::asio::io_service io;
	tempest::tcp_acceptor acceptor(8080, handle_client, io);
	io.run();
}
