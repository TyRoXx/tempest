#include "file_handle.hpp"
#include <boost/system/system_error.hpp>
#include <limits>

#if TEMPEST_USE_POSIX
#	include <sys/sendfile.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <fcntl.h>
#endif


namespace tempest
{
	namespace posix
	{
#if TEMPEST_USE_POSIX
		status::status()
		    : size(std::numeric_limits<file_size>::max())
		    , is_regular(false)
		{
		}


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
				auto const max_per_piece = std::numeric_limits<ssize_t>::max();
				//any positive ssize_t must be representible with file_size for this to work
				assert(static_cast<file_size>(max_per_piece) < std::numeric_limits<file_size>::max());

				auto const piece_length = static_cast<size_t>(
				            std::min<file_size>(max_per_piece, rest));

				ssize_t const sent = sendfile(destination, m_fd, NULL, piece_length);
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

		void swap(file_handle &left, file_handle &right)
		{
			left.swap(right);
		}


		file_handle open_read(std::string const &file_name)
		{
			int const fd = ::open(file_name.c_str(), O_RDONLY | O_LARGEFILE);
			if (fd < 0)
			{
				throw make_errno_exception();
			}
			return file_handle(fd);
		}
#endif
	}
}
