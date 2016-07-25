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
	SymInitializeW(hp, sympath.c_str(), FALSE);

	report_ctx ctx;
	for (auto && kv: ci.pdbs)
	{
		std::vector<uint8_t> buf;
		buf.resize(sizeof(MODLOAD_CVMISC) + kv.second.cv.size());

		MODLOAD_CVMISC * cvmisc = (MODLOAD_CVMISC *)buf.data();
		cvmisc->oCV = sizeof(MODLOAD_CVMISC);
		cvmisc->cCV = kv.second.cv.size();
		cvmisc->oMisc = 0;
		cvmisc->cMisc = 0;
		cvmisc->dtImage = kv.second.timestamp;
		cvmisc->cImage = kv.second.image_size;
		std::copy(kv.second.cv.begin(), kv.second.cv.end(), buf.data() + sizeof(MODLOAD_CVMISC));

		MODLOAD_DATA md = {};
		md.ssize = sizeof md;
		md.ssig = DBHHEADER_CVMISC;
		md.data = buf.data();
		md.size = buf.size();

		uint64_t base = SymLoadModuleExW(hp, 0, L"kkk", nullptr, 0x10000, kv.second.image_size, &md, 0);
		if (base == 0)
			throw std::runtime_error("failed to load symbols");

		ctx.ci = &kv.second;
		ctx.base = base;
		SymEnumLinesW(hp, base, nullptr, nullptr, &SymEnumLinesProc, &ctx);
		if (ctx.exc != nullptr)
			std::rethrow_exception(ctx.exc);

		SymUnloadModule64(hp, base);
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
