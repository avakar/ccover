#include "debugger_loop.h"
#include "utf.h"

struct json_writer
{
	json_writer(std::ostream & out)
		: m_out(out), m_comma(false)
	{
	}

	void open_object()
	{
		this->comma();
		this->write_raw("{");
		m_comma = false;
	}

	void close_object()
	{
		this->write_raw("}");
		m_comma = true;
	}

	void write_key(string_view s)
	{
		this->comma();
		this->write_raw_str(s);
		this->write_raw(":");
		m_comma = false;
	}

	void open_array()
	{
		this->comma();
		this->write_raw("[");
		m_comma = false;
	}

	void close_array()
	{
		this->write_raw("]");
		m_comma = true;
	}

	template <typename Num>
	void write_num(Num num)
	{
		this->comma();
		this->write_raw(std::to_string(num));
		m_comma = true;
	}

	void write_str(string_view s)
	{
		this->comma();
		this->write_raw_str(s);
		m_comma = true;
	}

	void write_str(wstring_view s)
	{
		this->write_str(utf16_to_utf8(s));
	}

private:
	void comma()
	{
		if (m_comma)
			this->write_raw(",");
	}

	void write_raw_str(string_view s)
	{
		this->write_raw("\"");

		char const * first = s.begin();
		char const * last = s.end();

		auto needs_escape = [](char ch) { return ch == '\\' || ch == '\n' || ch == '"'; };

		for (char const * cur = first; cur != last; ++cur)
		{
			if (needs_escape(*cur))
			{
				this->write_raw(string_view(first, cur));
				this->write_raw("\\");
				first = cur;
			}
		}

		this->write_raw(string_view(first, last));
		this->write_raw("\"");
	}

	void write_raw(string_view s)
	{
		m_out.write(s.data(), s.size());
	}

	std::ostream & m_out;
	bool m_comma;
};

void coverage_info::store(std::ostream & out)
{
	json_writer j(out);

	j.open_array();
	for (auto & pdb_info: pdbs)
	{
		j.open_object();

		j.write_key("filename");
		j.write_str(pdb_info.filename);

		j.write_key("timestamp");
		j.write_num(pdb_info.timestamp);

		j.write_key("image_size");
		j.write_num(pdb_info.image_size);

		j.write_key("pdb_guid");
		j.write_str(pdb_info.pdb_guid.to_string());

		j.write_key("covered");
		j.open_array();
		for (uint64_t addr: pdb_info.addrs_covered)
			j.write_num(addr);
		j.close_array();

		j.write_key("uncovered");
		j.open_array();
		for (uint64_t addr: pdb_info.addrs_uncovered)
			j.write_num(addr);
		j.close_array();


		j.close_object();
	}
	j.close_array();
}

