#include <boost/asio.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
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

	typedef std::string url;

	struct http_request
	{
		std::string method;
		url file;
		std::string version;
		std::map<std::string, std::string> headers;
	};

	void trim_trailing_char(std::string &str, char trailing)
	{
		if (!str.empty() &&
		        str.back() == trailing)
		{
			str.resize(str.size() - 1);
		}
	}

	http_request parse_request(std::istream &source)
	{
		source.exceptions(std::ios::failbit | std::ios::badbit);

		http_request request;
		getline(source, request.method, ' ');
		getline(source, request.file, ' ');
		getline(source, request.version, '\n');
		trim_trailing_char(request.version, '\r');

		while (source.peek() != '\r')
		{
			std::string key, value;
			getline(source, key, ':');
			if (source.peek() == ' ')
			{
				source.get();
			}
			getline(source, value, '\n');
			trim_trailing_char(request.version, '\r');

			request.headers.insert(
			            std::make_pair(std::move(key), std::move(value)));
		}

		//skip \r
		source.get();

		if (source.get() != '\n')
		{
			throw std::runtime_error("Bad request: Missing new-line at the end");
		}
		return request;
	}

	struct http_response
	{
		std::string version;
		unsigned status;
		std::string reason;
		std::map<std::string, std::string> headers;
		std::string body;
	};

	void print_response(http_response const &response, std::ostream &out)
	{
		out << response.version << ' '
		    << response.status << ' '
		    << response.reason << "\r\n";
		for (auto const &header : response.headers)
		{
			out << header.first << ": " << header.second << "\r\n";
		}
		out << "\r\n";
		out << response.body;
	}

	void handle_request_threaded(boost::shared_ptr<abstract_client> client)
	{
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
	boost::uint16_t port = 8080;
	if (argc >= 2)
	{
		port = boost::lexical_cast<boost::uint16_t>(argv[1]);
	}
	tempest::run_tcp_server(port);
}
