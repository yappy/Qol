#include "stdafx.h"
#include "include/config.h"
#include "include/debug.h"
#include <cstdio>

namespace yappy {
namespace config {

ConfigFile::ConfigFile(const wchar_t *fileName, InitList keyAndDefaults) :
	m_fileName(fileName),
	m_defaults(keyAndDefaults),
	m_map()
{}

ConfigFile::~ConfigFile()
{}

void ConfigFile::load()
{
	debug::writeLine(L"Load config");

	std::regex re(R"(\s*(\S+?)\s*=\s*(\S*)\s*)");
	std::cmatch match;

	FILE *tmpfp = nullptr;
	if (::_wfopen_s(&tmpfp, m_fileName, L"r") != 0) {
		save();
		if (::_wfopen_s(&tmpfp, m_fileName, L"r") != 0) {
			throw std::runtime_error("Load config file failed");
		}
	}
	util::FilePtr fp(tmpfp);

	char line[LineCharMax];
	while (::fgets(line, LineCharMax, fp.get()) != nullptr) {
		size_t len = ::strlen(line);
		if (line[len - 1] == '\n') {
			// legal line
			if (std::regex_match(line, match, re)) {
				const std::string key = match.str(1);
				const std::string value = match.str(2);
				debug::writef(L"key: %s, value: %s",
					util::utf82wc(key.c_str()).get(),
					util::utf82wc(value.c_str()).get());
				m_map[key] = value;
			}
		}
		else {
			// illegal line
			while (::fgets(line, LineCharMax, fp.get()) != nullptr) {
				// ignore to next LF
				size_t len = ::strlen(line);
				if (line[len - 1] == '\n') {
					break;
				}
			}
		}
	}
	fp.reset();
}

void ConfigFile::save()
{
	debug::writeLine(L"Save config");

	FILE *tmpfp = nullptr;
	if (::_wfopen_s(&tmpfp, m_fileName, L"w") != 0) {
		throw std::runtime_error("Save config file failed");
	}
	util::FilePtr fp(tmpfp);

	for (const auto &pair : m_defaults) {
		const std::string &key = pair.first;
		const std::string &value = getString(key);
		::fprintf_s(fp.get(), "%s=%s\n", key.c_str(), value.c_str());
	}
}

void ConfigFile::setString(const std::string &key, const std::string &value)
{
	if (m_defaults.find(key) == m_defaults.end()) {
		throw std::invalid_argument("Key not found: " + key);
	}
	m_map[key] = value;
}

const std::string &ConfigFile::getString(const std::string &key) const
{
	auto res = m_map.find(key);
	if (res == m_map.end()) {
		res = m_defaults.find(key);
		if (res == m_defaults.end()) {
			throw std::invalid_argument("Key not found: " + key);
		}
	}
	return res->second;
}

bool ConfigFile::getBool(const std::string &key)
{
	{
		const std::string &value = getString(key);
		if (value == BoolStrTrue) {
			return true;
		}
		else if (value == BoolStrFalse) {
			return false;
		}
	}
	m_map.erase(key);
	{
		const std::string &value = getString(key);
		if (value == BoolStrTrue) {
			return true;
		}
		else if (value == BoolStrFalse) {
			return false;
		}
		else {
			throw std::logic_error("Invalid default bool: " + value);
		}
	}
	
}

int ConfigFile::getInt(const std::string &key)
{
	{
		const std::string &value = getString(key);
		try {
			size_t idx = 0;
			int ivalue = std::stoi(value, &idx, 0);
			if (idx != value.size()) {
				throw std::invalid_argument(value);
			}
			return ivalue;
		}
		catch (const std::logic_error &) {
			// go through
		}
	}
	m_map.erase(key);
	{
		const std::string &value = getString(key);
		try {
			size_t idx = 0;
			int ivalue = std::stoi(value, &idx, 0);
			if (idx != value.size()) {
				throw std::invalid_argument(value);
			}
			return ivalue;
		}
		catch (const std::logic_error &) {
			throw std::logic_error("Invalid default int: " + value);
		}
	}
}

}
}
