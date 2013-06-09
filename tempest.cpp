#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "tempest/file_handle.hpp"
#include "tempest/tcp_client.hpp"
#include "tempest/tcp_acceptor.hpp"
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/make_shared.hpp>
#include <memory>
#include <fstream>

#if TEMPEST_USE_POSIX
#	include <sys/sendfile.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

namespace tempest
{
	boost::optional<boost::filesystem::path>
	complete_served_path(boost::filesystem::path const &top,
	                     std::string const &requested)
	{
		//TODO white-listing instead of black-listing
		if (requested.find("..") != std::string::npos)
		{
			return boost::optional<boost::filesystem::path>();
		}

		return top / requested;
	}

	http_response make_bad_request()
	{
		http_response response;
		response.version = "HTTP/1.1";
		response.status = 400;
		response.reason = "Bad Request";
		response.headers["Content-Type"] = "text/html";
		response.headers["Content-Length"] = "0";
		return response;
	}

	std::pair<http_response, std::string>
	make_not_found_response(std::string const &requested_file)
	{
		http_response response;
		response.version = "HTTP/1.1";
		response.status = 404;
		response.reason = "Not Found";
		response.headers["Content-Type"] = "text/html";

		std::string body =
		        "<h2>The requested file could not be found</h2>"
		        "<p>" + requested_file + "</p>"
		        ;

		response.headers["Content-Length"] =
		        boost::lexical_cast<std::string>(body.size());
		return std::make_pair(std::move(response), std::move(body));
	}

	std::pair<http_response, std::string>
	make_not_implemented_response(std::string const &requested_file)
	{
		http_response response;
		response.version = "HTTP/1.1";
		response.status = 501;
		response.reason = "Not Implemented";
		response.headers["Content-Type"] = "text/html";

		std::string body =
		        "<h2>could not serve the file because some required "
		        "functionality is not implemented in the web server</h2>"
		        "<p>" + requested_file + "</p>"
		        ;

		response.headers["Content-Length"] =
		        boost::lexical_cast<std::string>(body.size());
		return std::make_pair(std::move(response), std::move(body));
	}

	void send_in_memory_response(
		std::pair<http_response, std::string> const &response,
		sender &sender)
	{
		print_response(response.first, sender.response());

		auto &body = response.second;
		sender.response().write(body.data(), body.size());
	}

	struct directory
	{
		virtual ~directory();
		virtual void respond(http_request const &request,
		                     sender &sender) = 0;
	};

	directory::~directory()
	{
	}

#if TEMPEST_USE_POSIX
	namespace posix
	{
		void copy_file(int source, std::ostream &sink)
		{
			boost::iostreams::file_descriptor_source
			        source_device(source,
			                      boost::iostreams::never_close_handle);
			boost::iostreams::copy(source_device, sink);
		}

		struct file_system_directory : directory
		{
			explicit file_system_directory(boost::filesystem::path dir);
			virtual void respond(http_request const &request,
			                     sender &sender) override;

		private:

			const boost::filesystem::path m_dir;
		};

		file_system_directory::file_system_directory(boost::filesystem::path dir)
		    : m_dir(std::move(dir))
		{
		}

		void file_system_directory::respond(http_request const &request,
		                                    sender &sender)
		{
			if (request.method != "GET" &&
			    request.method != "POST")
			{
				return send_in_memory_response(make_not_implemented_response(request.file), sender);
			}

			boost::optional<boost::filesystem::path> const full_path =
			        complete_served_path(m_dir, request.file);

			if (!full_path)
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			file_handle file = open_read(full_path->string());
			auto const status = file.get_status();

			if (status.size > std::numeric_limits<std::size_t>::max())
			{
				return send_in_memory_response(make_not_implemented_response(request.file), sender);
			}

			if (!status.is_regular)
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			http_response response;
			response.headers["Content-Length"] =
			        boost::lexical_cast<std::string>(status.size);
			response.status = 200;
			response.reason = "OK";
			response.version = "HTTP/1.1";

			print_response(response, sender.response());

			boost::optional<int> const client_fd = sender.posix_response();
			if (client_fd)
			{
				sender.response().flush();
				file_size sent = file.send_to(*client_fd, status.size);
				if (sent != status.size)
				{
					throw std::runtime_error("Sending the whole file failed");
				}
			}
			else
			{
				copy_file(file.handle(), sender.response());
				sender.response().flush();
			}
		}
	}
#endif

	namespace portable
	{
		struct file_system_directory : directory
		{
			explicit file_system_directory(boost::filesystem::path dir);
			virtual void respond(http_request const &request,
			                     sender &sender) override;

		private:

			const boost::filesystem::path m_dir;
		};

		file_system_directory::file_system_directory(boost::filesystem::path dir)
			: m_dir(std::move(dir))
		{
		}

		void file_system_directory::respond(http_request const &request,
		                                    sender &sender)
		{
			if (request.method != "GET" &&
				request.method != "POST")
			{
				return send_in_memory_response(make_not_implemented_response(request.file), sender);
			}

			boost::optional<boost::filesystem::path> const full_path =
					complete_served_path(m_dir, request.file);

			if (!full_path ||
					!boost::filesystem::is_regular(*full_path))
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			std::ifstream file(full_path->string(),
							   std::ios::binary);
			if (!file)
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			file.exceptions(std::ios::failbit | std::ios::badbit);

			auto const file_size = boost::filesystem::file_size(*full_path);
			if (file_size > std::numeric_limits<std::size_t>::max())
			{
				return send_in_memory_response(make_not_implemented_response(request.file), sender);
			}

			http_response response;
			response.version = "HTTP/1.1";
			response.status = 200;
			response.reason = "OK";
			response.headers["Content-Length"] =
			        boost::lexical_cast<std::string>(file_size);

			print_response(response, sender.response());
			boost::iostreams::copy(file, sender.response());
			sender.response().flush();
		}
	}

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
			directory->respond(request, client->get_sender());

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
	auto served_directory_absolute = boost::filesystem::canonical(served_directory);

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
