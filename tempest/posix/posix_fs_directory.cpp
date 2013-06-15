#include "posix_fs_directory.hpp"
#include <tempest/client.hpp>
#include <tempest/posix/file_handle.hpp>
#include <tempest/responses.hpp>
#include <http/http_request.hpp>
#include <http/http_response.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/lexical_cast.hpp>


namespace tempest
{
	namespace posix
	{
#if TEMPEST_USE_POSIX
		namespace
		{
			void copy_file(int source, std::ostream &sink)
			{
				boost::iostreams::file_descriptor_source
						source_device(source,
									  boost::iostreams::never_close_handle);
				boost::iostreams::copy(source_device, sink);
			}
		}

		file_system_directory::file_system_directory(boost::filesystem::path dir)
			: m_dir(boost::move(dir))
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

			if (!full_path)
			{
				return send_in_memory_response(make_not_found_response(request.file), sender);
			}

			file_handle file = open_read(full_path->string());
			posix::status const status = file.get_status();

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
#endif
	}
}
