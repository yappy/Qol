#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"
#include <lua.hpp>

namespace yappy {
namespace lua {
namespace lua_export {

namespace {

template <class T>
inline T *getPtrFromSelf(lua_State *L, const char *fieldName)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L, 1, fieldName);
	luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);
	void *p = lua_touserdata(L, -1);
	T *result = reinterpret_cast<T *>(p);
	lua_pop(L, 1);
	return result;
}

}

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

int graph::loadTexture(lua_State *L)
{
	auto *app = getPtrFromSelf<graphics::Application>(L, graph_RawFieldName);
	const char *id = luaL_checkstring(L, 2);
	const char *path = luaL_checkstring(L, 3);

	app->loadTexture(id, util::utf82wc(path).get());

	return 0;
}

int graph::getTextureSize(lua_State *L)
{
	auto *app = getPtrFromSelf<graphics::Application>(L, graph_RawFieldName);
	const char *id = luaL_checkstring(L, 2);

	uint32_t w = 0;
	uint32_t h = 0;
	app->getTextureSize(id, &w, &h);

	lua_pushinteger(L, w);
	lua_pushinteger(L, h);

	return 2;
}

/**
 * (string id, int dx, int dy, bool lrInv = false, bool udInv = false,
 * int sx = 0, int sy = 0, int sw = -1, int sh = -1,
 * int cx = 0, int cy = 0, float angle = 0.0f,
 * float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);
 */
int graph::drawTexture(lua_State *L)
{
	auto *app = getPtrFromSelf<graphics::Application>(L, graph_RawFieldName);
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
