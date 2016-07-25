#include "json.h"
#include "debugger_loop.h"
#include "utf.h"
#include "utils.h"
#include <cassert>

coverage_info coverage_info::load(std::istream & in)
{
	coverage_info res;

	json_reader reader(in);
	reader.read_array([&]() {
		guid pdb_guid;
		pdb_coverage_info pdb_info;
		reader.read_object([&](string_view key) {
			if (key == "filename")
				pdb_info.filename = reader.read_wstr();
			else if (key == "timestamp")
				pdb_info.timestamp = reader.read_num<uint32_t>();
			else if (key == "image_size")
				pdb_info.image_size = reader.read_num<uint32_t>();
			else if (key == "cv_record")
				pdb_info.cv = from_base64(reader.read_str());
			else if (key == "pdb_guid")
				pdb_guid = guid::from_string(reader.read_str());
			else if (key == "covered")
				reader.read_array([&]() {
					pdb_info.addrs_covered.push_back(reader.read_num<uint64_t>());
				});
		});

		if (pdb_guid.is_null())
			throw std::runtime_error("missing pdb_guid entry");
		res.pdbs[pdb_guid] = pdb_info;
	});

	return res;
}

void coverage_info::store(std::ostream & out)
{
	json_writer j(out);

	j.open_array();
	for (auto & kv: pdbs)
	{
		j.open_object();

		j.write_key("filename");
		j.write_str(kv.second.filename);

		j.write_key("timestamp");
		j.write_num(kv.second.timestamp);

		j.write_key("image_size");
		j.write_num(kv.second.image_size);

		j.write_key("cv_record");
		j.write_str(to_base64(kv.second.cv.data(), kv.second.cv.size()));

		j.write_key("pdb_guid");
		j.write_str(kv.first.to_string());

		j.write_key("covered");
		j.open_array();
		for (uint64_t addr: kv.second.addrs_covered)
			j.write_num(addr);
		j.close_array();

		j.close_object();
	}
	j.close_array();
}

void coverage_info::merge(coverage_info && ci)
{
	for (auto && kv: ci.pdbs)
	{
		auto it = pdbs.find(kv.first);
		if (it == pdbs.end())
		{
			pdbs[kv.first] = std::move(kv.second);
			continue;
		}

		if (it->second.timestamp != kv.second.timestamp || it->second.image_size != kv.second.image_size)
			throw std::runtime_error("inconsistent");

		std::vector<uint64_t> merged;
		std::set_union(it->second.addrs_covered.begin(), it->second.addrs_covered.end(), kv.second.addrs_covered.begin(),kv.second.addrs_covered.end(), std::back_inserter(merged));
		kv.second.addrs_covered = std::move(merged);
	}
}
