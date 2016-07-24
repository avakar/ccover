#ifndef GUID_H
#define GUID_H

#include <algorithm>
#include <string>
#include <stdint.h>

struct guid
{
	uint8_t data[16];

	bool is_null() const
	{
		for (uint8_t b: data)
		{
			if (b != 0)
				return false;
		}

		return true;
	}

	std::string to_string() const
	{
		static char const digits[] = "0123456789abcdef";
		static size_t const hyphen_pos[] = { 8, 13, 18, 23, 37 };

		char buf[36];
		size_t buf_ptr = 0;

		size_t const * hyptr = hyphen_pos;
		for (size_t i = 0; i < 16; ++i)
		{
			uint8_t b = data[i];
			buf[buf_ptr++] = digits[b >> 4];
			buf[buf_ptr++] = digits[b & 0xf];
			if (buf_ptr == *hyptr)
			{
				buf[buf_ptr++] = '-';
				++hyptr;
			}
		}

		return std::string(buf, 36);
	}

	friend bool operator==(guid const & lhs, guid const & rhs)
	{
		return std::equal(lhs.data, lhs.data + sizeof lhs.data, rhs.data);
	}

	friend bool operator!=(guid const & lhs, guid const & rhs)
	{
		return !(lhs == rhs);
	}

	friend bool operator<(guid const & lhs, guid const & rhs)
	{
		return std::lexicographical_compare(lhs.data, lhs.data + sizeof lhs.data, rhs.data, rhs.data + sizeof rhs.data);
	}

	friend bool operator>(guid const & lhs, guid const & rhs)
	{
		return rhs < lhs;
	}

	friend bool operator<=(guid const & lhs, guid const & rhs)
	{
		return !(rhs < lhs);
	}

	friend bool operator>=(guid const & lhs, guid const & rhs)
	{
		return !(lhs < rhs);
	}
};



#endif // GUID_H
