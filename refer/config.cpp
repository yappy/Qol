#include "stdafx.h"
#include "config.hpp"
#include "debug.hpp"

namespace dx9lib {

	ConfigFile::ConfigFile(const tstring &filename, bool storeOnDestruct) :
		m_fileName(filename), m_storeOnDestruct(storeOnDestruct)
	{
		try{
			load();
		} catch (IOError &) {
			debug.println(_T("Warning: Opening file failed: ") + filename);
		}
	}

	ConfigFile::~ConfigFile()
	{
		if (m_storeOnDestruct) {
			store();
		}
	}

	void ConfigFile::load()
	{
		tifstream ifs(m_fileName.c_str(), std::ios::in);
		if (!ifs)
			throw IOError(_T("Opening file failed: ") + m_fileName);

		array<TCHAR, LINE_BUF_SIZE> buf;
		while (!ifs.eof()) {
			ifs.getline(&buf[0], LINE_BUF_SIZE);
			// empty line | comment line
			if (buf[0] == L'\0' || buf[0] == L'#') {
				continue;
			}
			tstring str(&buf[0]);
			tstring::size_type ind = str.find(L'=');
			if (ind == tstring::npos)
				throw IOError(_T("Invalid line"));
			tstring key = str.substr(0, ind);
			tstring val = str.substr(ind + 1);
			if (!datamap.insert(std::make_pair(key, val)).second) {
				debug.println(_T("Warning: multiple key '" + key + _T("'")));
			}
		}
	}

	void ConfigFile::store()
	{
		tofstream ofs(m_fileName.c_str(), std::ios::out);
		if (!ofs)
			throw IOError(_T("Opening file failed:") + m_fileName);
		for (map_type::iterator it=datamap.begin(); it!=datamap.end(); ++it) {
			ofs << it->first << _T('=') << it->second << std::endl;
		}
	}

	const tstring ConfigFile::getString(const tstring &key, const tstring &def)
	{
		map_type::iterator it = datamap.find(key);
		if (it == datamap.end()) {
			setString(key, def);
			return def;
		}
		return it->second;
	}

	int ConfigFile::getInt(const tstring &key, int def)
	{
		map_type::iterator it = datamap.find(key);
		if (it == datamap.end()) {
			setInt(key, def);
			return def;
		}
		return to_i(it->second);
	}

	void ConfigFile::setString(const tstring &key, const tstring &value)
	{
		datamap.insert(std::make_pair(key, value));
	}

	void ConfigFile::setInt(const tstring &key, int value)
	{
		datamap.insert(std::make_pair(key, to_s(value)));
	}

}
