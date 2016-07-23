#include <windows.h>

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

	basic_string_view(T const * s, size_type size) noexcept
		: m_first(s), m_last(s + size)
	{
	}

	basic_string_view(T const * s)
		: m_first(s), m_last(s + Traits::length(s))
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

//#include "string_view.h"
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

struct opts
{
	std::wstring sympath;
	std::wstring win_cmdline;

	void parse(wstring_view cmdline)
	{
		while (!cmdline.empty())
		{
			wstring_view prev_cmdline = cmdline;
			std::wstring arg = win_split_cmdline_arg(cmdline);

			if (arg == L"-y" || arg == L"--sympath")
			{
				sympath = win_split_cmdline_arg(cmdline);
			}
			else if (arg == L"--")
			{
				win_cmdline = cmdline;
				return;
			}
			else if (arg[0] == L'-')
			{
				throw std::invalid_argument("unknown argument");
			}
			else
			{
				win_cmdline = prev_cmdline;
				return;
			}
		}

		throw std::invalid_argument("unknown argument");
	}
};

#include <map>

#pragma warning(push)
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

void debug_loop(opts const & opts)
{
	std::map<DWORD, HANDLE> process_handles;
	for (;;)
	{
		DEBUG_EVENT de;
		WaitForDebugEvent(&de, INFINITE);

		HANDLE hProcess;
		if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
		{
			hProcess = de.u.CreateProcessInfo.hProcess;
			process_handles[de.dwProcessId] = hProcess;
		}
		else
		{
			hProcess = process_handles[de.dwProcessId];
		}

		switch (de.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
		{
			SymInitializeW(hProcess, opts.sympath.c_str(), FALSE);
			DWORD64 base = SymLoadModuleExW(hProcess, de.u.CreateProcessInfo.hFile, nullptr, nullptr, (DWORD64)de.u.CreateProcessInfo.lpBaseOfImage, 0, 0, 0);
			IMAGEHLP_MODULEW64 im = { sizeof im };
			SymGetModuleInfoW64(hProcess, base, &im);
			CloseHandle(de.u.CreateProcessInfo.hFile);
			break;
		}

		case EXIT_PROCESS_DEBUG_EVENT:
			process_handles.erase(de.dwProcessId);
			if (process_handles.empty())
			{
				ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
				return;
			}
			break;

		case LOAD_DLL_DEBUG_EVENT:
		{
			DWORD64 base = SymLoadModuleExW(hProcess, de.u.LoadDll.hFile, nullptr, nullptr, (DWORD64)de.u.LoadDll.lpBaseOfDll, 0, 0, 0);
			IMAGEHLP_MODULEW64 im = { sizeof im };
			SymGetModuleInfoW64(hProcess, base, &im);
			CloseHandle(de.u.LoadDll.hFile);
			break;
		}

		case UNLOAD_DLL_DEBUG_EVENT:
			SymUnloadModule64(hProcess, (DWORD64)de.u.UnloadDll.lpBaseOfDll);
			break;
		}

		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
	}
}

int main()
{
	wstring_view cmdline = GetCommandLineW();
	win_split_cmdline_arg(cmdline);

	opts opts;
	opts.parse(cmdline);

	STARTUPINFOW si = { sizeof si };
	PROCESS_INFORMATION pi;
	if (!CreateProcessW(nullptr, &opts.win_cmdline[0], nullptr, nullptr, FALSE,
		DEBUG_PROCESS, nullptr, nullptr, &si, &pi))
	{
		throw std::runtime_error("can't create process");
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	debug_loop(opts);
}
