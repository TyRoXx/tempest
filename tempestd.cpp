#include <http/http_request.hpp>
#include <tempest/tcp_acceptor.hpp>
#include <tempest/portable_fs_directory.hpp>
#include <tempest/posix/posix_fs_directory.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>

namespace tempest
{
	typedef
#if TEMPEST_USE_POSIX
		posix::file_system_directory
#else
		portable::file_system_directory
#endif
		optimal_file_system_directory;

	void handle_request_threaded(boost::shared_ptr<abstract_client> client,
								 boost::shared_ptr<directory> directory)
	{
		assert(client);
		assert(directory);

		//We have to catch exceptions before they propagate to
		//boost::thread because that would terminate the whole process.
		try
		{
			http_request request = parse_request(client->get_receiver().request());
			directory->respond(request, request.file, client->get_sender());

			client->shutdown();
		}
		catch (std::exception const &ex)
		{
			std::cerr << ex.what() << '\n';
		}
	}

	void handle_client(std::unique_ptr<abstract_client> &client,
					   boost::shared_ptr<directory> directory)
	{
		boost::shared_ptr<abstract_client> const shared_client(client.release());
		boost::thread client_thread(handle_request_threaded,
									shared_client, directory);
		client_thread.detach();
	}

	void run_file_server(boost::uint16_t port,
						 boost::shared_ptr<directory> directory)
	{
		boost::asio::io_service io_service;
		tcp_acceptor acceptor(port,
							  boost::bind(handle_client, _1, directory),
							  io_service);
		io_service.run();
	}
}

int main(int argc, char **argv)
{
	namespace po = boost::program_options;

	boost::uint16_t port = 8080;
	std::string served_directory;

	po::options_description options("Tempest web server options");
	options.add_options()
		("help,h", "produce help message to stdout and exit")
		("version,v", "print version number to stdout and exit")
		("port", po::value(&port), ("the port to listen on (default: " +
									boost::lexical_cast<std::string>(port) + ")").c_str())
		("dir", po::value(&served_directory), "the directory accessible to clients")
		("portable", "avoid possibly platform-specific system calls")
		;

	po::positional_options_description positions;
	positions.add("dir", 1);
	positions.add("port", 1);

	po::variables_map variables;
	po::store(po::command_line_parser(argc, argv).options(options).positional(positions).run(),
			  variables);
	po::notify(variables);

	if (variables.count("help"))
	{
		std::cout << options << '\n';
		return 0;
	}

	if (variables.count("version"))
	{
		std::cout << "0.1\n";
		return 0;
	}

	if (!variables.count("dir"))
	{
		std::cout << "'dir' argument required\n\n";
		std::cout << options << '\n';
		return 1;
	}

	bool const favor_portability = variables.count("portable");
	auto served_directory_absolute = boost::filesystem::absolute(served_directory);

	boost::shared_ptr<tempest::directory> directory_handler;
	if (favor_portability)
	{
		directory_handler = boost::make_shared<tempest::portable::file_system_directory>(served_directory_absolute);
	}
	else
	{
		directory_handler = boost::make_shared<tempest::optimal_file_system_directory>(served_directory_absolute);
	}

	tempest::run_file_server(port, directory_handler);
}
