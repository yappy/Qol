#pragma once

#include "util.h"

struct lua_State;

namespace yappy {
namespace lua {

class LuaError : public std::runtime_error {
public:
	LuaError(const std::string &msg, lua_State *L) noexcept;
	const char *what() const override
	{ return m_what.c_str(); }
private:
	std::string m_what;
};

class Lua {
public:
	Lua();
	~Lua() = default;
	void load(const wchar_t *fileName, const char *name = nullptr);
	void dumpStack(void);

private:
	static void luaDeleter(lua_State *lua);
	using LuaDeleterType = decltype(&luaDeleter);

	std::unique_ptr<lua_State, LuaDeleterType> m_lua;
};

}
}
