#include "utf.h"
#include <windows.h>
#include <stdexcept>

std::string utf16_to_utf8(wstring_view s)
{
	int r = WideCharToMultiByte(CP_UTF8, 0, s.data(), s.size(), nullptr, 0, nullptr, nullptr);
	if (r < 0)
		throw std::runtime_error("failed to convert to utf-8");

	std::string res;
	if (r != 0)
	{
		res.resize((size_t)r + 1);
		r = WideCharToMultiByte(CP_UTF8, 0, s.data(), s.size(), &res[0], res.size(), nullptr, nullptr);
		if (r < 0 || r > res.size() - 1)
			throw std::runtime_error("failed to convert to utf-8");
		res.resize((size_t)r);
	}
	return res;
}

