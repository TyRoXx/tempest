#ifndef TEMPEST_CONFIG_HPP
#define TEMPEST_CONFIG_HPP


#include <boost/cstdint.hpp>


#ifndef TEMPEST_USE_POSIX
#	ifdef __linux__
#		define TEMPEST_USE_POSIX 1
#	else
#		define TEMPEST_USE_POSIX 0
#	endif
#endif


namespace tempest
{
	typedef boost::uintmax_t file_size;
}


#endif
