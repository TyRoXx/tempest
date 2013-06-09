#ifndef TEMPEST_POSIX_FS_DIRECTORY_HPP
#define TEMPEST_POSIX_FS_DIRECTORY_HPP


#include <tempest/config.hpp>
#include <tempest/directory.hpp>
#include <boost/filesystem/path.hpp>


namespace tempest
{
#if TEMPEST_USE_POSIX
	namespace posix
	{
		struct file_system_directory : directory
		{
			explicit file_system_directory(boost::filesystem::path dir);
			virtual void respond(http_request const &request,
			                     sender &sender) override;

		private:

			const boost::filesystem::path m_dir;
		};
	}
#endif
}


#endif
