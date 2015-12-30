#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"
#include <lua.hpp>

namespace yappy {
namespace lua {
namespace lua_export {

namespace {

// Get reinterpret_cast<T *>(self.fieldName)
// self is the first argument (stack[1])
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
// "resource" table
///////////////////////////////////////////////////////////////////////////////

/**
* (self, int setId, string resId, string path)
*/
int resource::addTexture(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
	int setId = static_cast<int>(luaL_checkinteger(L, 2));
	const char *resId = luaL_checkstring(L, 3);
	const char *path = luaL_checkstring(L, 4);

	app->addTextureResource(setId, resId, util::utf82wc(path).get());

	return 0;
}

/**
* (self, int setId, string resId, string path)
*/
int resource::addFont(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
	int setId = static_cast<int>(luaL_checkinteger(L, 2));
	const char *resId = luaL_checkstring(L, 3);
	const char *fontName = luaL_checkstring(L, 4);
	const char *startCharStr = luaL_checkstring(L, 5);
	const char *endCharStr = luaL_checkstring(L, 6);
	int w = static_cast<int>(luaL_checkinteger(L, 7));
	int h = static_cast<int>(luaL_checkinteger(L, 8));

	wchar_t startChar = util::utf82wc(startCharStr)[0];
	wchar_t endChar = util::utf82wc(endCharStr)[0];

	app->addFontResource(setId, resId, util::utf82wc(fontName).get(),
		startChar, endChar, w, h);

	return 0;
}

/**
* (self, int setId, string resId, string path)
*/
int resource::addSe(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
	int setId = static_cast<int>(luaL_checkinteger(L, 2));
	const char *resId = luaL_checkstring(L, 3);
	const char *path = luaL_checkstring(L, 4);

	app->addSeResource(setId, resId, util::utf82wc(path).get());

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// "graph" table
///////////////////////////////////////////////////////////////////////////////

/**
 * (self, int setId, string resId)
 */
int graph::getTextureSize(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, graph_RawFieldName);
	int setId = static_cast<int>(luaL_checkinteger(L, 2));
	const char *resId = luaL_checkstring(L, 3);

	const auto &pTex = app->getTexture(setId, resId);

	lua_pushinteger(L, pTex->w);
	lua_pushinteger(L, pTex->h);

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
	auto *app = getPtrFromSelf<framework::Application>(L, graph_RawFieldName);
	int setId = static_cast<int>(luaL_checkinteger(L, 2));
	const char *resId = luaL_checkstring(L, 3);
	int dx = static_cast<int>(luaL_checkinteger(L, 4));
	int dy = static_cast<int>(luaL_checkinteger(L, 5));

	bool lrInv = getBooleanWithDefault(L, 6, false);
	bool udInv = getBooleanWithDefault(L, 7, false);
	int sx = getIntWithDefault(L, 8, 0);
	int sy = getIntWithDefault(L, 9, 0);
	int sw = getIntWithDefault(L, 10, 0);
	int sh = getIntWithDefault(L, 11, 0);
	int cx = getIntWithDefault(L, 12, 0);
	int cy = getIntWithDefault(L, 13, 0);
	float angle = getFloatWithDefault(L, 14, 0.0f);
	float scaleX = getFloatWithDefault(L, 15, 1.0f);
	float scaleY = getFloatWithDefault(L, 16, 1.0f);
	float alpha = getFloatWithDefault(L, 17, 1.0f);

	const auto &pTex = app->getTexture(setId, resId);
	app->graph().drawTexture(pTex, dx, dy, lrInv, udInv, sx, sy, sw, sh, cx, cy,
		angle, scaleX, scaleY, alpha);

	return 0;
}

int graph::drawString(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, graph_RawFieldName);
	int setId = static_cast<int>(luaL_checkinteger(L, 2));
	const char *resId = luaL_checkstring(L, 3);
	const char *str = luaL_checkstring(L, 4);
	int dx = static_cast<int>(luaL_checkinteger(L, 5));
	int dy = static_cast<int>(luaL_checkinteger(L, 6));

	int color = getIntWithDefault(L, 7, 0x000000);
	int ajustX = getIntWithDefault(L, 8, 0);
	float scaleX = getFloatWithDefault(L, 9, 1.0f);
	float scaleY = getFloatWithDefault(L, 10, 1.0f);
	float alpha = getFloatWithDefault(L, 11, 1.0f);

	const auto &pFont = app->getFont(setId, resId);
	app->graph().drawString(pFont, util::utf82wc(str).get(), dx, dy,
		color, ajustX, scaleX, scaleY, alpha);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// "sound" table
///////////////////////////////////////////////////////////////////////////////

int sound::playBgm(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, sound_RawFieldName);
	const char *path = luaL_checkstring(L, 2);

	app->sound().playBgm(util::utf82wc(path).get());

	return 0;
}

int sound::stopBgm(lua_State *L)
{
	auto *app = getPtrFromSelf<framework::Application>(L, sound_RawFieldName);

	app->sound().stopBgm();

	return 0;
}

}
}
}
