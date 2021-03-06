#include "responses.hpp"
#include "client.hpp"
#include "http/http_response.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/move/move.hpp>


namespace tempest
{
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
		return std::make_pair(boost::move(response), boost::move(body));
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
		return std::make_pair(boost::move(response), boost::move(body));
	}

	void send_in_memory_response(
		std::pair<http_response, std::string> const &response,
		sender &sender)
	{
		print_response(response.first, sender.response());

		std::string const &body = response.second;
		sender.response().write(body.data(), body.size());
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
}
