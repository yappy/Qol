#pragma once

#include "util.h"
#include <lua.hpp>

namespace yappy {
namespace lua {

class Lua {
public:
	Lua();
	~Lua() = default;
	void load(const wchar_t *fileName, const char *name = nullptr);

private:
	static void luaDeleter(lua_State *lua) { ::lua_close(lua); }
	using LuaDeleterType = decltype(&luaDeleter);

	std::unique_ptr<lua_State, LuaDeleterType> m_lua;
};

}
}
