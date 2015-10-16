#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"
#include <lua.hpp>

namespace yappy {
namespace lua {
namespace lua_export {

///////////////////////////////////////////////////////////////////////////////
// "trace" table
///////////////////////////////////////////////////////////////////////////////

int trace::write(lua_State *L)
{
	int argc = lua_gettop(L);
	for (int i = 1; i <= argc; i++) {
		const char *str = ::lua_tostring(L, i);
		str = (str == nullptr) ? "<?>" : str;
		debug::writeLine(str);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// "graph" table
///////////////////////////////////////////////////////////////////////////////

/**
 * (string id, int dx, int dy, bool lrInv = false, bool udInv = false,
 * int sx = 0, int sy = 0, int sw = -1, int sh = -1,
 * int cx = 0, int cy = 0, float angle = 0.0f,
 * float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);
 */
int graph::drawTexture(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, graph_RawFieldName);

	luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);
	void *p = lua_touserdata(L, -1);
	auto *app = reinterpret_cast<graphics::Application *>(p);
	lua_pop(L, 1);

	const char *id = luaL_checkstring(L, 2);
	int dx = static_cast<int>(luaL_checkinteger(L, 3));
	int dy = static_cast<int>(luaL_checkinteger(L, 4));

	// TODO
	app->drawTexture(id, dx, dy);

	return 0;
}

}
}
}
