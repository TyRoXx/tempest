//Clang 1.1 is not able to compile ASIO header-only for unknown reasons.
//Workaround:
//http://www.boost.org/doc/libs/1_46_1/doc/html/boost_asio/using.html#boost_asio.using.optional_separate_compilation

#ifdef BOOST_ASIO_SEPARATE_COMPILATION
#	include <boost/asio/impl/src.hpp>
#endif
