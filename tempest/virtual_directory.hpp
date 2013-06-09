#ifndef TEMPEST_VIRTUAL_DIRECTORY_HPP
#define TEMPEST_VIRTUAL_DIRECTORY_HPP


#include <tempest/directory.hpp>
#include <boost/function.hpp>


namespace tempest
{
	struct virtual_directory : directory
	{
		typedef boost::function<directory *(std::string const &)> sub_dir_mapping;

		explicit virtual_directory(sub_dir_mapping mapping);
		virtual void respond(http_request const &request,
		                     std::string const &sub_path,
		                     sender &sender) override;

	private:

		sub_dir_mapping const m_mapping;
	};
}


#endif
