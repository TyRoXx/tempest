#include "virtual_directory.hpp"
#include "responses.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"


namespace tempest
{
	virtual_directory::virtual_directory(sub_dir_mapping mapping)
	    : m_mapping(std::move(mapping))
	{
		assert(m_mapping);
	}

	void virtual_directory::respond(http_request const &request,
	                                std::string const &sub_path,
	                                sender &sender)
	{
		std::string::const_iterator sub_dir_begin = sub_path.begin();
		if (sub_dir_begin != sub_path.end())
		{
			if (*sub_dir_begin == '/')
			{
				++sub_dir_begin;
			}
		}
		std::string::const_iterator const sub_dir_end =
		    std::find(sub_dir_begin, sub_path.end(), '/');
		std::string const sub_dir_name(sub_dir_begin, sub_dir_end);
		directory * const sub_dir = m_mapping(sub_dir_name);
		if (sub_dir)
		{
			std::string const rest(sub_dir_end, sub_path.end());
			return sub_dir->respond(request, rest, sender);
		}
		else
		{
			return send_in_memory_response(
			            make_not_found_response(request.file), sender);
		}
	}
}
