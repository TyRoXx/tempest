#ifndef TEMPEST_HTTP_DECODE_URI_HPP
#define TEMPEST_HTTP_DECODE_URI_HPP


#include <iterator>
#include <stdexcept>
#include <boost/range.hpp>


namespace tempest
{
	namespace detail
	{
		template <class InputIterator>
		typename std::iterator_traits<InputIterator>::value_type
		require_char(InputIterator &position,
		             InputIterator end)
		{
			if (position == end)
			{
				throw std::invalid_argument("Unexpected end of input");
			}

			return *position++;
		}

		template <class Char>
		unsigned decode_hex(Char c)
		{
			if (c >= '0' && c <= '9')
			{
				return (c - '0');
			}

			c = std::tolower(c);
			if (c >= 'a' && c <= 'f')
			{
				return (c - 'a' + 10);
			}

			throw std::invalid_argument("Hexadecimal digit expected");
		}
	}

	template <class InputIterator, class OutputIterator>
	OutputIterator
	decode_uri(InputIterator source_begin,
	           InputIterator source_end,
	           OutputIterator dest)
	{
		while (source_begin != source_end)
		{
			char const current = *source_begin++;
			if (current == '%')
			{
				unsigned char_value = 0;
				for (unsigned i = 0; i < 2; ++i)
				{
					char const digit = detail::require_char(source_begin, source_end);
					unsigned digit_value = detail::decode_hex(digit);
					char_value *= 16;
					char_value += digit_value;
				}

				*dest++ = static_cast<char>(char_value);
			}
			else
			{
				*dest++ = current;
			}
		}

		return dest;
	}

	template <class Container>
	void decode_uri(Container &uri)
	{
		typename Container::iterator const new_end =
			decode_uri(boost::begin(uri), boost::end(uri), boost::begin(uri));
		uri.erase(new_end, boost::end(uri));
	}
}

#endif
