#include "debugger_loop.h"
#include "cmdline.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <windows.h>

struct capture_opts
{
	bool print_help;
	std::wstring sympath;
	std::wstring covinfo_fname;
	std::wstring win_cmdline;

	capture_opts()
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

struct report_opts
{
	std::vector<std::wstring> input_files;
	std::wstring sympath;
	std::wstring output_file;

	report_opts()
		: output_file(L"-")
	{
	}

	bool parse(wstring_view cmdline)
	{
		bool ignore_opts = false;
		while (!cmdline.empty())
		{
			std::wstring arg = win_split_cmdline_arg(cmdline);

			if (!ignore_opts)
			{
				if (arg == L"-y" || arg == L"--sympath")
				{
					sympath = win_split_cmdline_arg(cmdline);
					continue;
				}

				if (arg == L"-o" || arg == L"--output")
				{
					output_file = win_split_cmdline_arg(cmdline);
					continue;
				}

				if (arg == L"--")
				{
					ignore_opts = true;
					continue;
				}

				if (arg[0] == L'-')
					return false;
			}

			input_files.push_back(arg);
		}

		return !input_files.empty();
	}
};

int main()
{
	wstring_view cmdline = GetCommandLineW();
	std::wstring arg0 = split_filename(win_split_cmdline_arg(cmdline)).second;

	std::wstring mode = win_split_cmdline_arg(cmdline);

	if (mode == L"capture")
	{
		capture_opts opts;
		opts.parse(cmdline);

		if (opts.print_help || opts.covinfo_fname.empty())
		{
			std::wcerr << L"Usage: " << arg0 << L" capture -o <output> [-y <sympath>] [--] <command> [<arg> ...]\n";
			return 2;
		}

		std::ofstream fcovinfo(opts.covinfo_fname.c_str(), std::ios::binary);
		if (!fcovinfo)
		{
			std::wcerr << arg0 << L": error: cannot open the output file\n";
			return 3;
		}

		coverage_info ci = capture_coverage(opts.win_cmdline, opts.sympath);
		ci.store(fcovinfo);
		return 0;
	}
	else if (mode == L"report")
	{
		report_opts opts;
		if (!opts.parse(cmdline))
		{
			std::wcerr << L"Usage: " << arg0 << L" report [-o <output>] [-y <sympath>] <input> [...]\n";
			return 2;
		}

		coverage_info ci;
		for (std::wstring const & input: opts.input_files)
		{
			std::ifstream fin(input.c_str(), std::ios::binary);
			if (!fin)
			{
				std::wcerr << arg0 << L": error: cannot open input file: " << input << L"\n";
				return 3;
			}

			ci.merge(coverage_info::load(fin));
		}
	}
	else
	{
		std::wcerr << L"Usage: " << arg0 << L" { capture | merge | report } [...]\n";
		return 2;
	}
}
