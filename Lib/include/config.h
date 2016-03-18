#pragma once

#include "util.h"
#include <string>
#include <map>

namespace yappy {
namespace config {

class ConfigFile : private util::noncopyable
{
public:
	using MapType = std::map<std::string, std::string>;
	using InitList = std::initializer_list<MapType::value_type>;

	static const size_t LineCharMax = 1024;
	const char *const BoolStrTrue  = "true";
	const char *const BoolStrFalse = "false";

	ConfigFile(const wchar_t *fileName, InitList keyAndDefaults);
	~ConfigFile();

	void setString(const std::string &key, const std::string &value);
	const std::string &getString(const std::string &key) const;
	bool getBool(const std::string &key);
	int getInt(const std::string &key);

private:
	const wchar_t *const m_fileName;
	MapType m_defaults;
	MapType m_map;

	void load();
	void save();
};

}
}
