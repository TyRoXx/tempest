#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <memory>
#include <fstream>

#ifdef __linux__
#	define TEMPEST_USE_POSIX 1
#else
#	define TEMPEST_USE_POSIX 0
#endif

#if TEMPEST_USE_POSIX
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tempest
{
	struct abstract_client
	{
		virtual ~abstract_client();
		virtual std::istream &request() = 0;
		virtual std::ostream &response() = 0;
		virtual boost::optional<int> posix_response() = 0;
	};

	abstract_client::~abstract_client()
	{
	}

	struct tcp_client : abstract_client
	{
		explicit tcp_client(std::unique_ptr<boost::asio::ip::tcp::iostream> socket);
		virtual std::istream &request() override;
		virtual std::ostream &response() override;
		virtual boost::optional<int> posix_response() override;

	private:

		const std::unique_ptr<boost::asio::ip::tcp::iostream> m_stream;
	};

	tcp_client::tcp_client(std::unique_ptr<boost::asio::ip::tcp::iostream> stream)
	    : m_stream(std::move(stream))
	{
	}

	std::istream &tcp_client::request()
	{
		return *m_stream;
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

	struct tcp_acceptor
	{
		typedef boost::function<void (std::unique_ptr<abstract_client> &)>
			client_handler;

		explicit tcp_acceptor(boost::uint16_t port,
		                      client_handler on_client,
		                      boost::asio::io_service &io_service);

	private:

		boost::asio::ip::tcp::acceptor m_impl;
		const client_handler m_on_client;
		std::unique_ptr<boost::asio::ip::tcp::iostream> m_next_client;

		void begin_accept();
		void handle_accept(boost::system::error_code error);
	};

	tcp_acceptor::tcp_acceptor(boost::uint16_t port,
	                           client_handler on_client,
	                           boost::asio::io_service &io_service)
	    : m_impl(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), port))
	    , m_on_client(std::move(on_client))
	{
		begin_accept();
	}

	void tcp_acceptor::begin_accept()
	{
		m_next_client.reset(
		            new boost::asio::ip::tcp::iostream());
		m_impl.async_accept(*m_next_client->rdbuf(),
		                    boost::bind(&tcp_acceptor::handle_accept, this,
		                                boost::asio::placeholders::error));
	}

	void tcp_acceptor::handle_accept(boost::system::error_code error)
	{
		if (error)
		{
			//TODO
			return;
		}

		std::unique_ptr<abstract_client> client(
		            new tcp_client(std::move(m_next_client)));
		m_on_client(client);
		begin_accept();
	}

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

	http_response make_not_found_response(std::string const &requested_file)
	{
		http_response response;
		response.version = "HTTP/1.1";
		response.status = 404;
		response.reason = "Not Found";
		response.headers["Content-Type"] = "text/html";

		response.body =
		        "<h2>The requested file could not be found</h2>"
		        "<p>" + requested_file + "</p>"
		        ;

		response.headers["Content-Length"] =
		        boost::lexical_cast<std::string>(response.body.size());
		return response;
	}

	http_response make_not_implemented_response(std::string const &requested_file)
	{
		http_response response;
		response.version = "HTTP/1.1";
		response.status = 501;
		response.reason = "Not Implemented";
		response.headers["Content-Type"] = "text/html";

		response.body =
		        "<h2>could not serve the file because some required "
		        "functionality is not implemented in the web server</h2>"
		        "<p>" + requested_file + "</p>"
		        ;

		response.headers["Content-Length"] =
		        boost::lexical_cast<std::string>(response.body.size());
		return response;
	}

	http_response make_file_response(std::istream &content,
	                                 std::size_t file_size)
	{
		http_response response;
		response.headers["Content-Length"] =
		        boost::lexical_cast<std::string>(file_size);
		response.status = 200;
		response.reason = "OK";
		response.version = "HTTP/1.1";
		std::copy_n(std::istreambuf_iterator<char>(content),
		            file_size,
		            std::back_inserter(response.body));
		return response;
	}

	struct directory
	{
		virtual ~directory();
		virtual void respond(http_request const &request,
		                     abstract_client &client) = 0;
	};

	directory::~directory()
	{
	}

	typedef boost::uintmax_t file_size;

#if TEMPEST_USE_POSIX
	namespace posix
	{
		struct status
		{
			file_size size;
			bool is_regular;
		};

		struct file_handle final : boost::noncopyable
		{
			file_handle();
			explicit file_handle(int fd);
			file_handle(file_handle &&other);
			~file_handle();
			file_handle &operator = (file_handle &&other);
			void swap(file_handle &other);
			status get_status();
			file_size send_to(int destination, file_size byte_count);
			int handle() const;

		private:

			int m_fd;
		};

		file_handle::file_handle()
		    : m_fd(-1)
		{
		}

		file_handle::file_handle(int fd)
			: m_fd(fd)
		{
		}

		file_handle::file_handle(file_handle &&other)
		    : m_fd(other.m_fd)
		{
			other.m_fd = -1;
		}

		file_handle::~file_handle()
		{
			if (m_fd >= 0)
			{
				close(m_fd);
			}
		}

		file_handle &file_handle::operator = (file_handle &&other)
		{
			if (this != &other)
			{
				swap(other);
				file_handle().swap(other);
			}
			return *this;
		}

		void file_handle::swap(file_handle &other)
		{
			std::swap(m_fd, other.m_fd);
		}

		namespace
		{
			boost::system::system_error make_errno_exception()
			{
				return boost::system::system_error(errno, boost::system::posix_category);
			}
		}

		status file_handle::get_status()
		{
			struct stat64 s;
			if (fstat64(m_fd, &s) == 0)
			{
				status result;
				result.size = s.st_size;
				result.is_regular = S_ISREG(s.st_mode);
				return result;
			}
			throw make_errno_exception();
		}

		file_size file_handle::send_to(int destination, file_size byte_count)
		{
			file_size total_sent = 0;
			file_size rest = byte_count;
			while (rest > 0)
			{
				auto constexpr max_per_piece = std::numeric_limits<ssize_t>::max();
				static_assert(static_cast<file_size>(max_per_piece) < std::numeric_limits<file_size>::max(),
				              "any positive ssize_t must be representible with file_size for this to work");

				auto const piece_length = static_cast<size_t>(
				            std::min<file_size>(max_per_piece, rest));

				ssize_t const sent = sendfile(destination, m_fd, nullptr, piece_length);
				if (sent >= 0)
				{
					file_size const sent_unsigned = static_cast<size_t>(sent);
					total_sent += sent_unsigned;
					rest -= sent_unsigned;
				}
				else
				{
					throw make_errno_exception();
				}
			}
			return total_sent;
		}

		int file_handle::handle() const
		{
			return m_fd;
		}


		file_handle open_read(std::string const &file_name)
		{
			int const fd = open(file_name.c_str(), O_RDONLY | O_LARGEFILE);
			if (fd < 0)
			{
				throw make_errno_exception();
			}
			return file_handle(fd);
		}


		struct file_system_directory : directory
		{
			explicit file_system_directory(boost::filesystem::path dir);
			virtual void respond(http_request const &request,
			                     abstract_client &client) override;

		private:

			const boost::filesystem::path m_dir;
		};

		file_system_directory::file_system_directory(boost::filesystem::path dir)
		    : m_dir(std::move(dir))
		{
		}

		void file_system_directory::respond(http_request const &request,
		                                    abstract_client &client)
		{
			if (request.method != "GET" &&
			    request.method != "POST")
			{
				return print_response(make_not_implemented_response(request.file), client.response());
			}

			boost::optional<boost::filesystem::path> const full_path =
			        complete_served_path(m_dir, request.file);

			if (!full_path)
			{
				return print_response(make_not_found_response(request.file), client.response());
			}

			file_handle file = open_read(full_path->string());
			auto const status = file.get_status();

			if (status.size > std::numeric_limits<std::size_t>::max())
			{
				return print_response(make_not_implemented_response(request.file), client.response());
			}

			if (!status.is_regular)
			{
				return print_response(make_not_found_response(request.file), client.response());
			}

			http_response response;
			response.headers["Content-Length"] =
			        boost::lexical_cast<std::string>(status.size);
			response.status = 200;
			response.reason = "OK";
			response.version = "HTTP/1.1";

			boost::optional<int> const client_fd = client.posix_response();
			if (client_fd)
			{
				print_response_header(response, client.response());
				client.response().flush();

				file_size sent = file.send_to(*client_fd, status.size);
				if (sent != status.size)
				{
					throw std::runtime_error("Sending the whole file failed");
				}
			}
			else
			{
				print_response(response, client.response());
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
			                     abstract_client &client) override;

		private:

			const boost::filesystem::path m_dir;
		};

		file_system_directory::file_system_directory(boost::filesystem::path dir)
			: m_dir(std::move(dir))
		{
		}

		void file_system_directory::respond(http_request const &request,
		                                    abstract_client &client)
		{
			if (request.method != "GET" &&
				request.method != "POST")
			{
				return print_response(make_not_implemented_response(request.file), client.response());
			}

			boost::optional<boost::filesystem::path> const full_path =
					complete_served_path(m_dir, request.file);

			if (!full_path ||
					!boost::filesystem::is_regular(*full_path))
			{
				return print_response(make_not_found_response(request.file), client.response());
			}

			std::ifstream file(full_path->string(),
							   std::ios::binary);
			if (!file)
			{
				return print_response(make_not_found_response(request.file), client.response());
			}

			file.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);

			auto const file_size = boost::filesystem::file_size(*full_path);
			if (file_size > std::numeric_limits<std::size_t>::max())
			{
				return print_response(make_not_implemented_response(request.file), client.response());
			}

			auto const response = make_file_response(file, file_size);
			return print_response(response, client.response());
		}
	}


	void handle_request_threaded(boost::shared_ptr<abstract_client> client,
	                             boost::filesystem::path const &directory)
	{
		//We have to catch exceptions before they propagate to
		//boost::thread because that would terminate the whole process.
		try
		{
			typedef
#if TEMPEST_USE_POSIX
				posix::file_system_directory
#else
				portable::file_system_directory
#endif
			default_directory;

			http_request request = parse_request(client->request());
			default_directory(directory).respond(request, *client);

			while (client->request())
			{
				client->request().get();
			}
		}
		catch (std::exception const &ex)
		{
			std::cerr << ex.what() << '\n';
		}
	}

	void handle_client(std::unique_ptr<abstract_client> &client,
	                   boost::filesystem::path const &directory)
	{
		boost::shared_ptr<abstract_client> const shared_client(client.release());
		boost::thread client_thread(handle_request_threaded,
		                            shared_client, directory);
		client_thread.detach();
	}

	void run_file_server(boost::uint16_t port,
	                     boost::filesystem::path directory)
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

	auto served_directory_absolute = boost::filesystem::canonical(served_directory);
	tempest::run_file_server(port, std::move(served_directory_absolute));
}
