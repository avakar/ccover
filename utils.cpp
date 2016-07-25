#include "utils.h"

std::pair<wstring_view, wstring_view> split_filename(wstring_view fname)
{
	auto first = fname.begin();
	auto last = fname.end();

	auto is_sep = [](wchar_t ch) { return ch == '/' || ch == '\\'; };

	while (last != first && is_sep(last[-1]))
		--last;

	for (auto cur = last; cur != first; --cur)
	{
		if (is_sep(cur[-1]))
			return std::make_pair(wstring_view(first, cur - 1), wstring_view(cur, last));
	}

	return std::make_pair(wstring_view(), wstring_view(first, last));
}

std::string to_base64(uint8_t const * p, size_t size)
{
	static char const digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string res;
	res.reserve(size / 3 * 4 + 4);

	while (size >= 3)
	{
		res.push_back(digits[p[0] >> 2]);
		res.push_back(digits[((p[0] << 4) & 0x30) | (p[1] >> 4)]);
		res.push_back(digits[((p[1] << 2) & 0x3c) | (p[2] >> 6)]);
		res.push_back(digits[p[2] & 0x3f]);
		p += 3;
		size -= 3;
	}

	switch (size)
	{
	case 1:
		res.push_back(digits[p[0] >> 2]);
		res.push_back(digits[(p[0] << 4) & 0x30]);
		res.append("==");
		break;
	case 2:
		res.push_back(digits[p[0] >> 2]);
		res.push_back(digits[((p[0] << 4) & 0x30) | (p[1] >> 4)]);
		res.push_back(digits[(p[1] << 2) & 0x3c]);
		res.push_back('=');
		break;
	}

	return res;
}

std::vector<uint8_t> from_base64(string_view s)
{
	if (s.size() % 4 != 0)
		throw std::invalid_argument("base64 strings must be a multiple of 4 characters long");

	auto extract_digit = [](char ch) -> uint8_t {
		if ('A' <= ch && ch <= 'Z')
			return ch - 'A';
		if ('a' <= ch && ch <= 'z')
			return ch - 'a' + 26;
		if ('0' <= ch && ch <= '9')
			return ch - '0' + 52;
		if (ch == '+')
			return 62;
		if (ch == '/')
			return 63;
		if (ch == '=')
			return 0;
		throw std::invalid_argument("unexpected characters");
	};

	char const * first = s.begin();
	char const * last = s.end();

	size_t eq_suffix = 0;
	if (first != last && last[-1] == '=')
	{
		if (last[-2] == '=')
			eq_suffix = 2;
		else
			eq_suffix = 1;
	}

	if (std::find(first, last - eq_suffix, '=') != last - eq_suffix)
		throw std::invalid_argument("stray equal signs");

	std::vector<uint8_t> res;
	res.reserve(s.size() / 4 * 3);

	while (!s.empty())
	{
		uint32_t v = 0;
		for (int i = 0; i < 4; ++i)
			v = (v << 6) | extract_digit(s[i]);
		s.remove_prefix(4);

		res.push_back(v >> 16);
		res.push_back(v >> 8);
		res.push_back(v);
	}

	res.erase(res.end() - eq_suffix, res.end());
	return res;
}
