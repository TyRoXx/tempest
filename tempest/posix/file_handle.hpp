#ifndef TEMPEST_POSIX_FILE_HANDLE_HPP
#define TEMPEST_POSIX_FILE_HANDLE_HPP


#include <tempest/config.hpp>
#include <boost/noncopyable.hpp>
#include <boost/move/move.hpp>
#include <string>


namespace tempest
{
	namespace posix
	{
#if TEMPEST_USE_POSIX
		struct status TEMPEST_FINAL
		{
			file_size size;
			bool is_regular;

			status();
		};

		struct file_handle TEMPEST_FINAL : boost::noncopyable
		{
			file_handle();
			explicit file_handle(int fd);
			file_handle(BOOST_RV_REF(file_handle) other);
			~file_handle();
			file_handle &operator = (BOOST_RV_REF(file_handle) other);
			void swap(file_handle &other);
			status get_status();
			file_size send_to(int destination, file_size byte_count);
			int handle() const;

		private:

			BOOST_MOVABLE_BUT_NOT_COPYABLE(file_handle)

			int m_fd;
		};

		void swap(file_handle &left, file_handle &right);

		file_handle open_read(std::string const &file_name);
#endif
	}
}


#endif
