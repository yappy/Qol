#pragma once

#include "util.h"
#include <string>
#include <map>

namespace yappy {
namespace config {

class Config : util::noncopyable
{
public:
	using MapType = std::map<std::string, std::string>;
	using InitList = std::initializer_list<MapType::value_type>;

	static const size_t LineCharMax = 1024;
	const char *const BoolStrTrue  = "true";
	const char *const BoolStrFalse = "false";

	Config(const wchar_t *fileName, InitList keyAndDefaults);
	~Config() = default;

	void load();
	void save() const;

	const std::string &getString(const std::string &key);
	bool getBool(const std::string &key);
	int getInt(const std::string &key);

private:
	const wchar_t *const m_fileName;
	MapType m_defaults;
	MapType m_map;
};

}
}
