#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/decode_uri.hpp"
#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <memory>

namespace tempest
{
	struct abstract_client
	{
		virtual ~abstract_client();
		virtual std::istream &request() = 0;
		virtual std::ostream &response() = 0;
	};

	abstract_client::~abstract_client()
	{
	}

	struct tcp_client : abstract_client
	{
		explicit tcp_client(std::unique_ptr<boost::asio::ip::tcp::iostream> socket);
		virtual std::istream &request() override;
		virtual std::ostream &response() override;

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

	void handle_request_threaded(boost::shared_ptr<abstract_client> client)
	{
		//We have to catch exceptions before they propagate to
		//boost::thread because that would terminate the whole process.
		try
		{
			http_request request = parse_request(client->request());

			http_response response;
			response.version = "HTTP/1.1";
			response.status = 200;
			response.reason = "OK";
			response.headers["Content-Type"] = "text/html";

			response.body =
			        "<h2>Hello, World!</h2>"
			        "<p>" + request.method +
			        " " + request.file +
			        " " + request.version + "</p>"
			        ;

			response.body += "<ul>";
			for (auto const &header : request.headers)
			{
				response.body += "<li>" + header.first + ": " + header.second;
			}
			response.body += "</ul>";

			response.headers["Content-Length"] =
			        boost::lexical_cast<std::string>(response.body.size());
			print_response(response, client->response());
			client->response().flush();
		}
		catch (std::exception const &ex)
		{
			//TODO
		}
	}

	void handle_client(std::unique_ptr<abstract_client> &client)
	{
		boost::shared_ptr<abstract_client> const shared_client(client.release());
		boost::thread client_thread(handle_request_threaded, shared_client);
		client_thread.detach();
	}

	void run_tcp_server(boost::uint16_t port)
	{
		boost::asio::io_service io_service;
		tcp_acceptor acceptor(port, handle_client, io_service);
		io_service.run();
	}
}

int main(int argc, char **argv)
{
	namespace po = boost::program_options;

	boost::uint16_t port = 8080;

	po::options_description options("Tempest web server options");
	options.add_options()
		("help,h", "produce help message to stdout and exit")
		("version,v", "print version number to stdout and exit")
	    ("port", po::value(&port), ("the port to listen on (default: " +
	                                boost::lexical_cast<std::string>(port) + ")").c_str())
		;

	po::positional_options_description positions;
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

	tempest::run_tcp_server(port);
}
