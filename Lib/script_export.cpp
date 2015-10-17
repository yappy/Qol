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

inline int getIntWithDefault(lua_State *L, int idx, int def)
{
	if (!lua_isnone(L, idx)) {
		return static_cast<int>(luaL_checkinteger(L, idx));
	}
	else {
		return def;
	}
}

inline float getFloatWithDefault(lua_State *L, int idx, float def)
{
	if (!lua_isnone(L, idx)) {
		return static_cast<float>(luaL_checknumber(L, idx));
	}
	else {
		return def;
	}
}

inline bool getBooleanWithDefault(lua_State *L, int idx, bool def)
{
	if (!lua_isnone(L, idx)) {
		return lua_toboolean(L, idx) != 0;
	}
	else {
		return def;
	}
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

	bool lrInv = getBooleanWithDefault(L, 5, false);
	bool udInv = getBooleanWithDefault(L, 6, false);
	int sx = getIntWithDefault(L, 7, 0);
	int sy = getIntWithDefault(L, 8, 0);
	int sw = getIntWithDefault(L, 9, 0);
	int sh = getIntWithDefault(L, 10, 0);
	int cx = getIntWithDefault(L, 11, 0);
	int cy = getIntWithDefault(L, 12, 0);
	float angle = getFloatWithDefault(L, 13, 0.0f);
	float scaleX = getFloatWithDefault(L, 14, 1.0f);
	float scaleY = getFloatWithDefault(L, 15, 1.0f);
	float alpha = getFloatWithDefault(L, 16, 1.0f);

	app->drawTexture(id, dx, dy, lrInv, udInv, sx, sy, sw, sh, cx, cy,
		angle, scaleX, scaleY, alpha);

	return 0;
}

/*
(string id, const wchar_t *fontName, string startChar, string endChar,
uint32_t w, uint32_t h)
*/
int graph::loadFont(lua_State *L)
{
	auto *app = getPtrFromSelf<graphics::Application>(L, graph_RawFieldName);
	const char *id = luaL_checkstring(L, 2);
	const char *fontName = luaL_checkstring(L, 3);
	const char *startCharStr = luaL_checkstring(L, 4);
	const char *endCharStr = luaL_checkstring(L, 5);
	int w = static_cast<int>(luaL_checkinteger(L, 6));
	int h = static_cast<int>(luaL_checkinteger(L, 7));

	wchar_t startChar = util::utf82wc(startCharStr)[0];
	wchar_t endChar = util::utf82wc(endCharStr)[0];
	app->loadFont(id, util::utf82wc(fontName).get(), startChar, endChar, w, h);

	return 0;
}

int graph::drawString(lua_State *L)
{
	auto *app = getPtrFromSelf<graphics::Application>(L, graph_RawFieldName);
	const char *id = luaL_checkstring(L, 2);
	const char *str = luaL_checkstring(L, 3);
	int dx = static_cast<int>(luaL_checkinteger(L, 4));
	int dy = static_cast<int>(luaL_checkinteger(L, 5));

	int color = getIntWithDefault(L, 6, 0x000000);
	int ajustX = getIntWithDefault(L, 7, 0);
	float scaleX = getFloatWithDefault(L, 8, 1.0f);
	float scaleY = getFloatWithDefault(L, 9, 1.0f);
	float alpha = getFloatWithDefault(L, 10, 1.0f);

	app->drawString(id, util::utf82wc(str).get(), dx, dy,
		color, ajustX, scaleX, scaleY, alpha);

	return 0;
}

}
}
}
