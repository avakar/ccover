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

	static guid from_string(string_view s)
	{
		auto pull_digit = [&]() -> uint8_t {

			while (!s.empty())
			{
				char ch = s.front();
				s.remove_prefix(1);

				if (ch == '-')
					continue;

				if ('0' <= ch && ch <= '9')
					return ch - '0';
				if ('a' <= ch && ch <= 'f')
					return ch - 'a' + 10;
				if ('A' <= ch && ch <= 'F')
					return ch - 'A' + 10;

				throw std::runtime_error("invalid character in a guid");
			}

			throw std::runtime_error("guid too short");
		};

		auto pull_byte = [&]() {
			uint8_t b = pull_digit() << 4;
			b |= pull_digit();
			return b;
		};

		guid res;
		std::generate(res.data, res.data + 16, pull_byte);
		return res;
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
