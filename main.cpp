#include "debugger_loop.h"
#include "cmdline.h"
#include <iostream>
#include <windows.h>

struct opts
{
	bool print_help;
	std::wstring sympath;
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


int main()
{
	wstring_view cmdline = GetCommandLineW();
	std::wstring arg0 = win_split_cmdline_arg(cmdline);

	opts opts;
	opts.parse(cmdline);

	if (opts.print_help)
	{
		std::wcout << L"Usage: " << arg0 << L" [-y sympath] [--] command [args ...]\n";
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

	debugger_loop(opts.sympath);
}
