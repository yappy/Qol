#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"
#include <lua.hpp>

namespace yappy {
namespace lua {

///////////////////////////////////////////////////////////////////////////////
// "trace" interface
///////////////////////////////////////////////////////////////////////////////
namespace trace {

int write(lua_State *L)
{
	int argc = lua_gettop(L);
	for (int i = 1; i <= argc; i++) {
		const char *str = ::lua_tostring(L, i);
		str = (str == nullptr) ? "<?>" : str;
		debug::writeLine(str);
	}
	return 0;
}

}

///////////////////////////////////////////////////////////////////////////////
// "graph" interface
///////////////////////////////////////////////////////////////////////////////
namespace graph {

int draw(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, "ptr");

	luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);
	void *p = lua_touserdata(L, -1);
	auto *app = reinterpret_cast<graphics::Application *>(p);

	// TODO

	return 0;
}

}

}
}
