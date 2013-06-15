#include "portable_fs_directory.hpp"
#include "responses.hpp"
#include "client.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/copy.hpp>
#include <fstream>


namespace tempest
{
	namespace portable
	{
		file_system_directory::file_system_directory(boost::filesystem::path dir)
			: m_dir(std::move(dir))
		{
		}

		void file_system_directory::respond(http_request const &request,
		                                    std::string const &sub_path,
		                                    sender &sender)
		{
			if (request.method != "GET" &&
				request.method != "POST")
			{
				return send_in_memory_response(make_not_implemented_response(request.file), sender);
			}

			boost::optional<boost::filesystem::path> const full_path =
					complete_served_path(m_dir, sub_path);

			if (!full_path ||
					!boost::filesystem::is_regular_file(*full_path))
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			//ifstream does not support std::string in old implementations
			//therefore c_str()
			std::ifstream file(full_path->string().c_str(),
							   std::ios::binary);
			if (!file)
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			file.exceptions(std::ios::failbit | std::ios::badbit);

			boost::uintmax_t const file_size = boost::filesystem::file_size(*full_path);
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
}
