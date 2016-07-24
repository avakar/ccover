#include "debugger_loop.h"
#include "cmdline.h"
#include <iostream>
#include <fstream>
#include <windows.h>

struct opts
{
	bool print_help;
	std::wstring sympath;
	std::wstring covinfo_fname;
	std::wstring win_cmdline;

	opts()
		: print_help(false)
	{
	}

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
			else if (arg == L"-o" || arg == L"--output")
			{
				covinfo_fname = win_split_cmdline_arg(cmdline);
			}
			else if (arg == L"--")
			{
				win_cmdline = cmdline;
				return;
			}
			else if (arg[0] == L'-')
			{
				print_help = true;
				return;
			}
			else
			{
				win_cmdline = prev_cmdline;
				return;
			}
		}

		print_help = true;
	}
};

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

int main()
{
	wstring_view cmdline = GetCommandLineW();
	std::wstring arg0 = split_filename(win_split_cmdline_arg(cmdline)).second;

	opts opts;
	opts.parse(cmdline);

	if (opts.print_help || opts.covinfo_fname.empty())
	{
		std::wcout << L"Usage: " << arg0 << L" -o output [-y sympath] [--] command [args ...]\n";
		return 2;
	}

	std::ofstream fcovinfo(opts.covinfo_fname.c_str(), std::ios::binary);
	if (!fcovinfo)
	{
		std::wcerr << arg0 << L": error: cannot open the output file\n";
		return 2;
	}

	STARTUPINFOW si = { sizeof si };
	PROCESS_INFORMATION pi;
	if (!CreateProcessW(nullptr, &opts.win_cmdline[0], nullptr, nullptr, FALSE,
		DEBUG_PROCESS, nullptr, nullptr, &si, &pi))
	{
		throw std::runtime_error("can't create process");
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	coverage_info ci = debugger_loop(opts.sympath);
	ci.store(fcovinfo);
	return 0;
}
