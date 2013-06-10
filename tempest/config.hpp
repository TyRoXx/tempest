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

#ifdef __GNUC__
#	if __GNUC__ >= 4 && (__GNUC_MINOR__ >= 7 || __GNUC__ >= 5)
#		define TEMPEST_FINAL final
#		define TEMPEST_OVERRIDE override
#	else
#		define TEMPEST_FINAL
#		define TEMPEST_OVERRIDE
#	endif
#else
#	define TEMPEST_FINAL
#	define TEMPEST_OVERRIDE
#endif


namespace tempest
{
	typedef boost::uintmax_t file_size;
}


#endif
