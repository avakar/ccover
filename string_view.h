#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <string>
#include <iterator>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <utility>

template <typename T, typename Traits = std::char_traits<T>>
struct basic_string_view
{
	using traits_type = Traits;
	using value_type = T;
	using pointer = T *;
	using const_pointer = T const *;
	using reference = T &;
	using const_reference = T const &;
	using const_iterator = T const *;
	using iterator = const_iterator;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using reverse_iterator = const_reverse_iterator;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	static size_type const npos = (size_type)-1;

	basic_string_view() noexcept
		: m_first(nullptr), m_last(nullptr)
	{
	}

	basic_string_view(basic_string_view const & o) noexcept
		: m_first(o.m_first), m_last(o.m_last)
	{
	}

	basic_string_view(T const * first, T const * last) noexcept
		: m_first(first), m_last(last)
	{
	}

	basic_string_view(T const * s, size_type size) noexcept
		: m_first(s), m_last(s + size)
	{
	}

	basic_string_view(T const * s)
		: m_first(s), m_last(s + Traits::length(s))
	{
	}

	template <typename Alloc>
	basic_string_view(std::basic_string<T, Traits, Alloc> const & s)
		: m_first(s.data()), m_last(s.data() + s.size())
	{
	}

	basic_string_view & operator=(basic_string_view const & o) noexcept = default;

	const_iterator begin() const noexcept { return m_first; }
	const_iterator cbegin() const noexcept { return m_first; }
	const_iterator end() const noexcept { return m_last; }
	const_iterator cend() const noexcept { return m_last; }

	const_reverse_iterator rbegin() const noexcept { return m_last; };
	const_reverse_iterator crbegin() const noexcept { return m_last; };
	const_reverse_iterator rend() const noexcept { return m_first; };
	const_reverse_iterator crend() const noexcept { return m_first; };

	const_reference operator[](size_type pos) const noexcept { return m_first[pos]; }

	const_reference at(size_type pos) const
	{
		if (pos > this->size())
			throw std::out_of_range("index out of range");
		return m_first[pos];
	}

	const_reference front() const noexcept { return m_first[0]; }
	const_reference back() const noexcept { return m_last[-1]; }
	const_pointer data() const noexcept { return m_first; }

	size_type size() const noexcept { return m_last - m_first; }
	size_type length() const noexcept { return this->size(); }
	bool empty() const noexcept { return m_first == m_last; }

	void remove_prefix(size_type n) noexcept { m_first += n; }
	void remove_suffix(size_type n) noexcept { m_last -= n; }

	void swap(basic_string_view & o) noexcept
	{
		using std::swap;
		swap(m_first, o.m_first);
		swap(m_last, o.m_last);
	}

	size_type copy(T * dest, size_type count, size_type pos = 0)
	{
		auto ss = this->substr(pos, count);
		std::copy(ss.cbegin(), ss.cend(), dest);
		return ss.size();
	}

	basic_string_view substr(size_type pos = 0, size_type count = npos) const
	{
		size_type size = this->size();
		if (pos >= size)
			throw std::out_of_range("pos out of range");
		count = (std::min)(count, size - pos);
		return basic_string_view(m_first + pos, m_first + pos + count);
	}

	int compare(basic_string_view o) const noexcept
	{
		size_type prefix_size = (std::min)(this->size(), o.size());
		int r = Traits::compare(m_first, o.m_first, prefix_size);
		if (r != 0)
			return r;
		if (this->size() < o.size())
			return -1;
		if (this->size() > o.size())
			return 1;
		return 0;
	}

	friend bool operator==(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.compare(rhs) == 0;
	}

	friend bool operator!=(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.compare(rhs) != 0;
	}

	friend bool operator<(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	friend bool operator<=(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.compare(rhs) <= 0;
	}

	friend bool operator>(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.compare(rhs) > 0;
	}

	friend bool operator>=(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.compare(rhs) >= 0;
	}

	template <typename Alloc>
	operator std::basic_string<T, Traits, Alloc>() const
	{
		return std::basic_string<T, Traits, Alloc>(m_first, m_last);
	}

private:
	T const * m_first;
	T const * m_last;
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;

#endif // STRING_VIEW_H
