#include "cmdline.h"
#include <stdexcept>

std::wstring win_split_cmdline_arg(wstring_view & cmdline)
{
	auto is_ws = [](wchar_t ch) { return ch == ' ' || ch == '\t'; };

	std::wstring res;

	auto first = cmdline.begin();
	auto last = cmdline.end();

	enum { st_text, st_quotes, st_escape } state = st_text;
	for (auto cur = first; cur != last; ++cur)
	{
		wchar_t ch = *cur;

		if (state == st_text)
		{
			if (is_ws(ch))
			{
				res.append(first, cur);
				do
				{
					++cur;
				}
				while (cur != last && is_ws(*cur));

				cmdline = wstring_view(cur, last - cur);
				return res;
			}

			if (ch == '^')
			{
				res.append(first, cur);
				first = cur + 2;
				state = st_escape;
			}
			else if (ch == '"')
			{
				res.append(first, cur);
				first = std::next(cur);
				state = st_quotes;
			}
		}
		else if (state == st_quotes)
		{
			if (ch == '"')
			{
				// XXX: consider backslashes at the end of the quoted part
				res.append(first, cur);
				first = std::next(cur);
				state = st_text;
			}
		}
		else if (state == st_escape)
		{
			res.push_back(ch);
			state = st_text;
		}
	}

	res.append(first, last);
	cmdline = wstring_view();
	return res;
}
