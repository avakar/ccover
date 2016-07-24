#include "debugger_loop.h"

#include "guid.h"

#include <map>
#include <set>

#include <windows.h>

#pragma warning(push)
// 'typedef ': ignored on left of '' when no variable is declared
#pragma warning(disable:4091)
#include <dbghelp.h>
#pragma warning(pop)

struct addr_info
{
	uint8_t orig_byte;
	bool covered;
};

struct pdb_info
{
	uint32_t image_size;
	uint32_t timestamp;
	std::wstring filename;
	std::set<HANDLE> processes;
	std::map<uint64_t, addr_info> addrs;
};

struct global_address
{
	HANDLE hProcess;
	uint64_t addr;

	friend bool operator==(global_address const & lhs, global_address const & rhs)
	{
		return lhs.hProcess == rhs.hProcess && lhs.addr == rhs.addr;
	}

	friend bool operator<(global_address const & lhs, global_address const & rhs)
	{
		return lhs.hProcess < rhs.hProcess
			|| (lhs.hProcess == rhs.hProcess && lhs.addr < rhs.addr);
	}
};

struct breakpoints
{
	std::map<guid, pdb_info> pdbs;
	std::map<global_address, pdb_info *> addrs;
};

struct sym_enum_ctx
{
	pdb_info * pi;
	std::exception_ptr exc;
};

BOOL CALLBACK SymEnumLinesProc(PSRCCODEINFOW LineInfo, PVOID UserContext) noexcept
{
	sym_enum_ctx & ctx = *static_cast<sym_enum_ctx *>(UserContext);

	try
	{
		ctx.pi->addrs[LineInfo->Address];
		return TRUE;
	}
	catch (...)
	{
		ctx.exc = std::current_exception();
		return FALSE;
	}
}

#include <cassert>

coverage_info debugger_loop(std::wstring const & sympath)
{
	breakpoints bkpts;

	auto load_module = [&](HANDLE hProcess, HANDLE hFile, DWORD64 base) {
		if (!SymLoadModuleExW(hProcess, hFile, nullptr, nullptr, base, 0, 0, 0))
			throw std::runtime_error("load module error");

		IMAGEHLP_MODULEW64 im = { sizeof im };
		if (!SymGetModuleInfoW64(hProcess, base, &im))
			throw std::runtime_error("get module info error");

		if (im.SymType == SymPdb && im.LineNumbers)
		{
			guid pdb_guid;
			memcpy(pdb_guid.data, &im.PdbSig70, sizeof pdb_guid.data);
			if (pdb_guid.is_null())
				return;

			pdb_info * pi;
			bool orig_bytes_known;

			auto pi_it = bkpts.pdbs.find(pdb_guid);
			if (pi_it == bkpts.pdbs.end())
			{
				pi = &bkpts.pdbs[pdb_guid];
				orig_bytes_known = false;

				pi->image_size = im.ImageSize;
				pi->timestamp = im.TimeDateStamp;
				pi->filename = im.LoadedPdbName;

				sym_enum_ctx ctx = { pi };
				SymEnumLinesW(hProcess, base, nullptr, nullptr, &SymEnumLinesProc, &ctx);
				if (ctx.exc != nullptr)
					std::rethrow_exception(ctx.exc);
			}
			else
			{
				pi = &pi_it->second;
				orig_bytes_known = true;
			}

			assert(pi->processes.find(hProcess) == pi->processes.end());
			pi->processes.insert(hProcess);

			for (auto && kv: pi->addrs)
			{
				global_address ga = { hProcess, kv.first };
				bkpts.addrs[ga] = pi;

				if (kv.second.covered)
					continue;

				uint8_t buf;
				if (!ReadProcessMemory(hProcess, (LPCVOID)kv.first, &buf, 1, nullptr))
					throw std::runtime_error("cannot read process memory"); // XXX: maybe we can ignore?

				assert(!orig_bytes_known || buf == kv.second.orig_byte);
				kv.second.orig_byte = buf;

				if (kv.second.orig_byte != 0xcc)
				{
					buf = 0xcc;
					if (!WriteProcessMemory(hProcess, (LPVOID)kv.first, &buf, 1, nullptr))
						throw std::runtime_error("cannot write process memory"); // XXX: maybe we can ignore?
				}
			}
		}
	};

	auto process_breakpoint = [&](HANDLE hProcess, HANDLE hThread, EXCEPTION_DEBUG_INFO const & exc) {
		uint64_t exc_addr = (uint64_t)exc.ExceptionRecord.ExceptionAddress;
		global_address ga = { hProcess, exc_addr };

		auto pi_it = bkpts.addrs.find(ga);
		if (pi_it == bkpts.addrs.end())
			return DBG_EXCEPTION_NOT_HANDLED;

		pdb_info * pi = pi_it->second;

		auto & addr_info = pi->addrs[exc_addr];
		addr_info.covered = true;

		if (addr_info.orig_byte == 0xcc)
			return DBG_EXCEPTION_NOT_HANDLED;

		for (HANDLE hProcess: pi->processes)
			WriteProcessMemory(hProcess, (LPVOID)exc_addr, &addr_info.orig_byte, 1, nullptr);

		CONTEXT ctx = {};
		ctx.ContextFlags = CONTEXT_CONTROL;
		if (GetThreadContext(hThread, &ctx))
		{
			ctx.Rip -= 1;
			SetThreadContext(hThread, &ctx);
		}
		return DBG_CONTINUE;
	};

	struct process_info
	{
		HANDLE h;
		std::map<DWORD, HANDLE> threads;
	};

	std::map<DWORD, process_info> process_handles;
	for (;;)
	{
		DEBUG_EVENT de;
		WaitForDebugEvent(&de, INFINITE);

		DWORD disp = DBG_EXCEPTION_NOT_HANDLED;

		process_info * pi;
		if (de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
		{
			pi = &process_handles[de.dwProcessId];
			pi->h = de.u.CreateProcessInfo.hProcess;
		}
		else
		{
			pi = &process_handles[de.dwProcessId];
		}

		HANDLE hProcess = pi->h;

		switch (de.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
		{
			SymInitializeW(hProcess, sympath.c_str(), FALSE);
			pi->threads[de.dwThreadId] = de.u.CreateProcessInfo.hThread;
			load_module(hProcess, de.u.CreateProcessInfo.hFile, (DWORD64)de.u.CreateProcessInfo.lpBaseOfImage);
			CloseHandle(de.u.CreateProcessInfo.hFile);
			break;
		}

		case CREATE_THREAD_DEBUG_EVENT:
			pi->threads[de.dwThreadId] = de.u.CreateThread.hThread;
			break;

		case EXIT_THREAD_DEBUG_EVENT:
			pi->threads.erase(de.dwThreadId);
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			assert(pi->threads.size() == 1);
			SymCleanup(hProcess);
			process_handles.erase(de.dwProcessId);
			if (process_handles.empty())
			{
				ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);

				coverage_info ci;
				ci.pdbs.reserve(bkpts.pdbs.size());
				for (auto && pdb_kv: bkpts.pdbs)
				{
					ci.pdbs.emplace_back();
					auto & pdb_info = ci.pdbs.back();
					pdb_info.pdb_guid = pdb_kv.first;
					pdb_info.image_size = pdb_kv.second.image_size;
					pdb_info.timestamp = pdb_kv.second.timestamp;
					pdb_info.filename = pdb_kv.second.filename;
					for (auto && addr_kv: pdb_kv.second.addrs)
					{
						if (addr_kv.second.covered)
							pdb_info.addrs_covered.push_back(addr_kv.first);
						else
							pdb_info.addrs_uncovered.push_back(addr_kv.first);
					}
				}
				return ci;
			}
			break;

		case LOAD_DLL_DEBUG_EVENT:
		{
			load_module(hProcess, de.u.LoadDll.hFile, (DWORD64)de.u.LoadDll.lpBaseOfDll);
			CloseHandle(de.u.LoadDll.hFile);
			break;
		}

		case UNLOAD_DLL_DEBUG_EVENT:
			// XXX: clear bkpt state
			SymUnloadModule64(hProcess, (DWORD64)de.u.UnloadDll.lpBaseOfDll);
			break;

		case EXCEPTION_DEBUG_EVENT:
			if (de.u.Exception.dwFirstChance && de.u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
			{
				disp = process_breakpoint(hProcess, pi->threads[de.dwThreadId], de.u.Exception);
			}
			break;
		}

		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, disp);
	}
}
