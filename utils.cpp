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
