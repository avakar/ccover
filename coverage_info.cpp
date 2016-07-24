#include "debugger_loop.h"
#include "utf.h"
#include <cassert>

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
				// XXX proper escape
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

struct json_reader
{
	json_reader(std::istream & in)
		: m_in(in), m_exc_state(m_in.exceptions()), m_next(nx_unknown)
	{
		m_in.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
		this->detect_next();
	}

	~json_reader()
	{
		m_in.exceptions(m_exc_state);
	}

	template <typename F>
	void read_array(F && f)
	{
		this->consume('[');
		while (!this->try_consume(']'))
		{
			this->detect_next();
			f();
			this->skip();
		}

		m_next = nx_comma;
	}

	template <typename F>
	void read_object(F && f)
	{
		this->consume('{');
		while (!this->try_consume('}'))
		{
			if (m_next == nx_comma)
				this->consume(',');

			std::string key = this->read_str();
			this->consume(':');

			this->detect_next();
			f(std::move(key));
			this->skip();
		}

		m_next = nx_comma;
	}

	std::string read_str()
	{
		this->consume('"');

		std::string res;
		for (;;)
		{
			char ch;
			m_in.get(ch);

			if (ch == '"')
			{
				m_next = nx_comma;
				return res;
			}

			if (ch == '\\')
			{
				m_in.get(ch);
				// XXX: other
			}

			res.push_back(ch);
		}
	}

	std::wstring read_wstr()
	{
		return utf8_to_utf16(this->read_str());
	}

	template <typename Num>
	Num read_num()
	{
		Num res = 0;

		char ch;
		for (;;)
		{
			m_in.get(ch);
			if (!is_ws(ch))
				break;
		}

		bool has_none = true;
		for (;;)
		{
			if ('0' <= ch && ch <= '9')
			{
				res = res * 10 + (ch - '0');
				has_none = false;
				m_in.get(ch);
			}
			else
			{
				m_in.unget();
				if (has_none)
					throw std::runtime_error("expected number");
				m_next = nx_comma;
				return res;
			}
		}
	}

private:
	static bool is_ws(char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }

	bool try_consume(char exp)
	{
		for (;;)
		{
			char ch;
			m_in.get(ch);
			if (!is_ws(ch))
			{
				if (ch != exp)
				{
					m_in.unget();
					return false;
				}

				m_next = nx_unknown;
				return true;
			}
		}
	}

	void consume(char exp)
	{
		if (!this->try_consume(exp))
			throw std::runtime_error("expected array");

	}

	void detect_next()
	{
		if (m_next == nx_comma)
			this->consume(',');

		char ch;
		do
		{
			m_in.get(ch);
		}
		while (is_ws(ch));

		if (ch == '{')
			m_next = nx_object;
		else if (ch == '[')
			m_next = nx_array;
		else if (ch == '"')
			m_next = nx_str;
		else if (ch == '-' || ('0' <= ch && ch <= '9'))
			m_next = nx_num;
		else
			throw std::runtime_error("unexpected character");

		m_in.unget();
	}

	void skip()
	{
		switch (m_next)
		{
		case nx_object:
			this->read_object([](string_view) {});
			break;
		case nx_array:
			this->read_array([]() {});
			break;
		case nx_num:
			this->read_num<int>();
			break;
		case nx_str:
			this->read_str();
			break;
		}

		assert(m_next == nx_comma);
	}

	std::istream & m_in;
	int m_exc_state;
	enum { nx_unknown, nx_object, nx_array, nx_num, nx_str, nx_comma } m_next;
};

coverage_info coverage_info::load(std::istream & in)
{
	coverage_info res;

	json_reader reader(in);
	reader.read_array([&]() {
		pdb_coverage_info pdb_info;
		reader.read_object([&](string_view key) {
			if (key == "filename")
				pdb_info.filename = reader.read_wstr();
			else if (key == "timestamp")
				pdb_info.timestamp = reader.read_num<uint32_t>();
			else if (key == "image_size")
				pdb_info.image_size = reader.read_num<uint32_t>();
			else if (key == "pdb_guid")
				pdb_info.pdb_guid = guid::from_string(reader.read_str());
			else if (key == "covered")
				reader.read_array([&]() {
					pdb_info.addrs_covered.push_back(reader.read_num<uint64_t>());
				});
		});

		res.pdbs.push_back(pdb_info);
	});

	return res;
}

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

		j.close_object();
	}
	j.close_array();
}

