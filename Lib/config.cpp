#include "stdafx.h"
#include "include/config.h"
#include <fstream>

namespace yappy {
namespace config {

Config::Config(const wchar_t *fileName, InitList keyAndDefaults) :
	m_fileName(fileName),
	m_defaults(keyAndDefaults), m_map(keyAndDefaults)
{
	
}

void Config::load()
{

}

void Config::save() const
{

}

const std::string &Config::getString(const std::string &key)
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

bool Config::getBool(const std::string &key)
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

int Config::getInt(const std::string &key)
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
