#ifndef TEMPEST_RESPONSES_HPP
#define TEMPEST_RESPONSES_HPP


#include <string>
#include <utility>
#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>


namespace tempest
{
	struct http_response;
	struct sender;

	http_response make_bad_request();

	std::pair<http_response, std::string>
	make_not_found_response(std::string const &requested_file);

	std::pair<http_response, std::string>
	make_not_implemented_response(std::string const &requested_file);

	void send_in_memory_response(
		std::pair<http_response, std::string> const &response,
		sender &sender);

	boost::optional<boost::filesystem::path>
	complete_served_path(boost::filesystem::path const &top,
	                     std::string const &requested);
}


#endif
