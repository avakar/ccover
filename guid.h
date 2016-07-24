#ifndef GUID_H
#define GUID_H

#include <algorithm>
#include <stdint.h>

struct guid
{
	uint8_t data[16];

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
