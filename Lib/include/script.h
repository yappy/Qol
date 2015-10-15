#pragma once

#include "util.h"
#include "graphics.h"
#include <lua.hpp>

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

	void loadTraceLib();
	void loadGraphLib(graphics::Application *app);
	void load(const wchar_t *fileName, const char *name = nullptr);
	void dumpStack(void);

private:
	static void luaDeleter(lua_State *lua);
	using LuaDeleterType = decltype(&luaDeleter);

	std::unique_ptr<lua_State, LuaDeleterType> m_lua;
};

///////////////////////////////////////////////////////////////////////////////
// Export Functions
///////////////////////////////////////////////////////////////////////////////

namespace trace {

int write(lua_State *L);
const luaL_Reg TraceLib[] =
{
	{ "write", write },
	{ nullptr, nullptr }
};

}	// namespace trace

namespace graph {

int draw(lua_State *L);
const luaL_Reg GraphLib[] =
{
	{ "draw", draw },
	{ nullptr, nullptr }
};

}	// namespace graph

}
}
