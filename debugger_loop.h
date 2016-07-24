#ifndef DEBUGGER_LOOP_H
#define DEBUGGER_LOOP_H

#include "string_view.h"
#include "guid.h"
#include <vector>
#include <string>
#include <stdint.h>

struct pdb_coverage_info
{
	guid pdb_guid;
	std::wstring filename;
	uint32_t image_size;
	uint32_t timestamp;
	std::vector<uint64_t> addrs_covered;
	std::vector<uint64_t> addrs_uncovered;
};

struct coverage_info
{
	std::vector<pdb_coverage_info> pdbs;

	void store(std::ostream & out);
};

coverage_info debugger_loop(std::wstring const & sympath);


#endif // DEBUGGER_LOOP_H
