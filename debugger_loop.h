#ifndef DEBUGGER_LOOP_H
#define DEBUGGER_LOOP_H

#include "string_view.h"
#include "guid.h"
#include <vector>
#include <map>
#include <string>
#include <stdint.h>

struct pdb_coverage_info
{
	std::wstring filename;
	uint32_t image_size;
	uint32_t timestamp;
	std::vector<uint64_t> addrs_covered;
};

struct coverage_info
{
	std::map<guid, pdb_coverage_info> pdbs;

	void merge(coverage_info && ci);

	static coverage_info load(std::istream & in);
	void store(std::ostream & out);
};

coverage_info capture_coverage(std::wstring cmdline, std::wstring const & sympath);


#endif // DEBUGGER_LOOP_H
