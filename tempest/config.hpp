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

#	if __GNUC__ >= 4 && (__GNUC_MINOR__ >= 4 || __GNUC__ >= 5)
#		define TEMPEST_IS_MOVABLE_PTR_UNIQUE 1
#	else
#		define TEMPEST_IS_MOVABLE_PTR_UNIQUE 0
#	endif

#elif defined(_MSC_VER)

#	define TEMPEST_FINAL
#	define TEMPEST_OVERRIDE
#	define TEMPEST_IS_MOVABLE_PTR_UNIQUE 1
	
#else
#	define TEMPEST_FINAL
#	define TEMPEST_OVERRIDE
#	define TEMPEST_IS_MOVABLE_PTR_UNIQUE 0
#endif

#if TEMPEST_IS_MOVABLE_PTR_UNIQUE
#	include <memory>
#endif

#include <boost/shared_ptr.hpp>

namespace tempest
{
	typedef boost::uintmax_t file_size;

	template <class Pointee>
	struct movable_ptr
	{
#if TEMPEST_IS_MOVABLE_PTR_UNIQUE
		typedef std::unique_ptr<Pointee> type;
		static boost::shared_ptr<Pointee> to_shared(type &ptr)
		{
			return ptr.release();
		}
#else
		typedef boost::shared_ptr<Pointee> type;
		static boost::shared_ptr<Pointee> to_shared(type &ptr)
		{
			return ptr;
		}
#endif
	};
}


#endif
