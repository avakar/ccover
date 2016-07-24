#include "json.h"
#include "debugger_loop.h"
#include <windows.h>

#pragma warning(push)
// 'typedef ': ignored on left of '' when no variable is declared
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

namespace {

struct report_ctx
{
	pdb_coverage_info const * ci;
	uint64_t base;
	std::map<std::wstring, std::map<uint64_t, std::pair<uint64_t, uint64_t>>> rep;
	std::exception_ptr exc;
};

}

static BOOL CALLBACK SymEnumLinesProc(PSRCCODEINFOW LineInfo, PVOID UserContext) noexcept
{
	report_ctx & ctx = *static_cast<report_ctx *>(UserContext);

	try
	{
		bool covered = std::binary_search(ctx.ci->addrs_covered.begin(), ctx.ci->addrs_covered.end(), LineInfo->Address - ctx.base);

		auto & tot_cov = ctx.rep[LineInfo->FileName][LineInfo->LineNumber];
		++tot_cov.first;
		if (covered)
			++tot_cov.second;
		return TRUE;
	}
	catch (...)
	{
		ctx.exc = std::current_exception();
		return FALSE;
	}
}

coverage_report report(coverage_info const & ci, std::wstring const & sympath)
{
	HANDLE hp = (HANDLE)4;
	uint64_t base = 0x10000;
	SymInitializeW(hp, sympath.c_str(), FALSE);

	report_ctx ctx;
	ctx.base = base;
	for (auto && kv: ci.pdbs)
	{
		wchar_t path[MAX_PATH+1];
		if (SymFindFileInPathW(hp, nullptr, kv.second.filename.c_str(), (PVOID)&kv.first.data, 1, 0, SSRVOPT_GUIDPTR, path, nullptr, nullptr))
			SymLoadModuleExW(hp, 0, path, nullptr, base, kv.second.image_size, nullptr, 0);
		else
			SymLoadModuleExW(hp, 0, kv.second.filename.c_str(), nullptr, base, kv.second.image_size, nullptr, 0);

		ctx.ci = &kv.second;
		SymEnumLinesW(hp, base, nullptr, nullptr, &SymEnumLinesProc, &ctx);
		if (ctx.exc != nullptr)
			std::rethrow_exception(ctx.exc);
	}

	coverage_report cr;
	for (auto & kv: ctx.rep)
	{
		coverage_file_report fr;
		fr.filename = kv.first;

		for (auto & line_kv: kv.second)
		{
			coverage_line_info li;
			li.line = line_kv.first;
			li.total_addresses = line_kv.second.first;
			li.covered = line_kv.second.second;
			fr.lines.push_back(li);
		}

		cr.files.push_back(std::move(fr));
	}

	return cr;
}

void coverage_report::store(std::ostream & out)
{
	json_writer w(out);

	w.open_object();
	for (auto & file: files)
	{
		w.write_key(file.filename);
		w.open_array();
		for (auto & line: file.lines)
		{
			w.open_array();
			w.write_num(line.line);
			w.write_num(line.total_addresses);
			w.write_num(line.covered);
			w.close_array();
		}
		w.close_array();
	}
	w.close_object();
}
