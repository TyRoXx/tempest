#ifndef TEMPEST_PORTABLE_FS_DIRECTORY_HPP
#define TEMPEST_PORTABLE_FS_DIRECTORY_HPP


#include "directory.hpp"
#include <boost/filesystem/path.hpp>


namespace tempest
{
	namespace portable
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
}


#endif
