#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"

namespace yappy {
namespace lua {
namespace export {

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

template <class F>
inline void exceptToLuaError(lua_State *L, F proc)
{
	try {
		proc();
	}
	catch (const std::exception &ex) {
		// push string ex.what() and throw
		luaL_error(L, "%s", ex.what());
	}
}

template <class T>
using lim = std::numeric_limits<T>;

static_assert(lim<lua_Integer>::min() <= lim<int>::min(),	"Lua type check");
static_assert(lim<lua_Integer>::max() >= lim<int>::max(),	"Lua type check");
static_assert(lim<lua_Number>::max() >= lim<float>::max(),	"Lua type check");

inline int luaintToInt(lua_State *L, int arg,
	lua_Integer val, int min, int max)
{
	luaL_argcheck(L, val >= min, arg, "number too small");
	luaL_argcheck(L, val <= max, arg, "number too large");
	return static_cast<int>(val);
}

inline int getInt(lua_State *L, int arg,
	int min = std::numeric_limits<int>::min(),
	int max = std::numeric_limits<int>::max())
{
	lua_Integer val = luaL_checkinteger(L, arg);
	return luaintToInt(L, arg, val, min, max);
}

inline int getOptInt(lua_State *L, int arg, int def,
	int min = std::numeric_limits<int>::min(),
	int max = std::numeric_limits<int>::max())
{
	lua_Integer val = luaL_optinteger(L, arg, def);
	return luaintToInt(L, arg, val, min, max);
}

inline float luanumToFloat(lua_State *L, int arg,
	lua_Number val, float min, float max)
{
	luaL_argcheck(L, val >= min, arg, "number too small or NaN");
	luaL_argcheck(L, val <= max, arg, "number too large or NaN");
	return static_cast<float>(val);
}

inline float getFloat(lua_State *L, int arg,
	float min = std::numeric_limits<float>::lowest(),
	float max = std::numeric_limits<float>::max())
{
	lua_Number val = luaL_checknumber(L, arg);
	return luanumToFloat(L, arg, val, min, max);
}

inline float getOptFloat(lua_State *L, int arg, float def,
	float min = std::numeric_limits<float>::lowest(),
	float max = std::numeric_limits<float>::max())
{
	lua_Number val = luaL_optnumber(L, arg, def);
	return luanumToFloat(L, arg, val, min, max);
}

}	// namespace

///////////////////////////////////////////////////////////////////////////////
// "trace" table
///////////////////////////////////////////////////////////////////////////////

/** @brief デバッグ出力する。
 * @details
 * @code
 * function trace.write(...)
 * end
 * @endcode
 *
 * @param[in]	...	任意の型および数の出力する値
 * @return			なし
 */
int trace::write(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		int argc = lua_gettop(L);
		for (int i = 1; i <= argc; i++) {
			const char *str = ::lua_tostring(L, i);
			if (str != nullptr) {
				debug::writeLine(str);
			}
			else {
				debug::writef("<%s>", luaL_typename(L, i));
			}
		}
	});
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// "resource" table
///////////////////////////////////////////////////////////////////////////////

/** @brief テクスチャリソースを登録する。
 * @details
 * @code
 * function resource:addTexture(int setId, str resId, str path)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @param[in]	path	ファイルパス
 * @return				なし
 */
int resource::addTexture(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);
		const char *path = luaL_checkstring(L, 4);

		app->addTextureResource(setId, resId, util::utf82wc(path).get());
	});
	return 0;
}

/** @brief フォントリソースを登録する。
 * @details
 * @code
 * function resource:addFont(int setId, str resId, str path)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @param[in]	path	ファイルパス
 * @return				なし
 */
int resource::addFont(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);
		const char *fontName = luaL_checkstring(L, 4);
		const char *startCharStr = luaL_checkstring(L, 5);
		luaL_argcheck(L, *startCharStr != '\0', 5, "empty string is NG");
		const char *endCharStr = luaL_checkstring(L, 6);
		luaL_argcheck(L, *endCharStr != '\0', 6, "empty string is NG");
		int w = getInt(L, 7, 0);
		int h = getInt(L, 8, 0);

		wchar_t startChar = util::utf82wc(startCharStr)[0];
		wchar_t endChar = util::utf82wc(endCharStr)[0];

		app->addFontResource(setId, resId, util::utf82wc(fontName).get(),
			startChar, endChar, w, h);
	});
	return 0;
}

/** @brief 効果音リソースを登録する。
 * @details
 * @code
 * function resource:addSe(int setId, str resId, str path)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @param[in]	path	ファイルパス
 * @return				なし
 */
int resource::addSe(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);
		const char *path = luaL_checkstring(L, 4);

		app->addSeResource(setId, resId, util::utf82wc(path).get());
	});
	return 0;
}

/** @brief BGMリソースを登録する。
 * @details
 * @code
 * function resource:addBgm(int setId, str resId, str path)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @param[in]	path	ファイルパス
 * @return				なし
 */
int resource::addBgm(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, resource_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);
		const char *path = luaL_checkstring(L, 4);

		app->addBgmResource(setId, resId, util::utf82wc(path).get());
	});
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// "graph" table
///////////////////////////////////////////////////////////////////////////////

/** @brief テクスチャのサイズを得る。
 * @details
 * @code
 * function graph:getTextureSize(int setId, str resId)
 * 	return w, h;
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @retval		1		横の長さ
 * @retval		2		縦の長さ
 */
int graph::getTextureSize(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, graph_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);

		const auto &pTex = app->getTexture(setId, resId);

		lua_pushinteger(L, pTex->w);
		lua_pushinteger(L, pTex->h);
	});
	return 2;
}

/** @brief テクスチャを描画する。
 * @details
 * @code
 * function graph:drawTexture(int setId, str resId,
 * 	int dx, int dy, bool lrInv = false, bool udInv = false,
 * 	int sx = 0, int sy = 0, int sw = -1, int sh = -1,
 * 	int cx = 0, int cy = 0, float angle = 0.0f,
 * 	float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f)
 * end
 * @endcode
 * スクリーン座標 (dx, dy) に (cx, cy) が一致するように描画されます。
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @param[in]	dx		描画先中心座標X
 * @param[in]	dy		描画先中心座標Y
 * @param[in]	lrInv	左右反転フラグ
 * @param[in]	udInv	上下反転フラグ
 * @param[in]	sx		テクスチャから切り出す際の左上座標X
 * @param[in]	sy		テクスチャから切り出す際の左上座標Y
 * @param[in]	sw		切り出しサイズX(-1でテクスチャサイズ)
 * @param[in]	sh		切り出しサイズY(-1でテクスチャサイズ)
 * @param[in]	cx		(sx, sy)基準の描画元中心座標X
 * @param[in]	cy		(sx, sy)基準の描画元中心座標Y
 * @param[in]	angle	回転角(rad)
 * @param[in]	scaleX	拡大率X
 * @param[in]	scaleY	拡大率Y
 * @param[in]	alpha	透明度(0.0 - 1.0)
 * @return				なし
 *
 * @sa graphics::DGraphics::drawTexture()
 */
int graph::drawTexture(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, graph_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);
		int dx = getInt(L, 4);
		int dy = getInt(L, 5);

		bool lrInv = lua_toboolean(L, 6) != 0;
		bool udInv = lua_toboolean(L, 7) != 0;
		int sx = getOptInt(L, 8, 0);
		int sy = getOptInt(L, 9, 0);
		int sw = getOptInt(L, 10, -1);
		int sh = getOptInt(L, 11, -1);
		int cx = getOptInt(L, 12, 0);
		int cy = getOptInt(L, 13, 0);
		float angle = getOptFloat(L, 14, 0.0f);
		float scaleX = getOptFloat(L, 15, 1.0f);
		float scaleY = getOptFloat(L, 16, 1.0f);
		float alpha = getOptFloat(L, 17, 1.0f);

		const auto &pTex = app->getTexture(setId, resId);
		app->graph().drawTexture(pTex, dx, dy, lrInv, udInv, sx, sy, sw, sh, cx, cy,
			angle, scaleX, scaleY, alpha);
	});
	return 0;
}

/** @brief 文字列を描画する。
 * @details
 * @code
 * function graph:drawString(int setId, str resId, str str, int dx, int dy,
 * 	int color = 0x000000, int ajustX = 0, 
 * 	float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @param[in]	str		描画する文字列
 * @param[in]	dx		描画先座標X
 * @param[in]	dy		描画先座標Y
 * @param[in]	color	文字色 (0xRRGGBB)
 * @param[in]	ajustX	文字間隔調整用X移動量
 * @param[in]	scaleX	拡大率X
 * @param[in]	scaleY	拡大率Y
 * @param[in]	alpha	透明度(0.0 - 1.0)
 * @return				なし
 *
 * @sa graphics::DGraphics::drawString()
 */
int graph::drawString(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, graph_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);
		const char *str = luaL_checkstring(L, 4);
		int dx = getInt(L, 5);
		int dy = getInt(L, 6);

		int color = getOptInt(L, 7, 0x000000);
		int ajustX = getOptInt(L, 8, 0);
		float scaleX = getOptFloat(L, 9, 1.0f);
		float scaleY = getOptFloat(L, 10, 1.0f);
		float alpha = getOptFloat(L, 11, 1.0f);

		const auto &pFont = app->getFont(setId, resId);
		app->graph().drawString(pFont, util::utf82wc(str).get(), dx, dy,
			color, ajustX, scaleX, scaleY, alpha);
	});
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// "sound" table
///////////////////////////////////////////////////////////////////////////////

/** @brief 効果音再生を開始する。
 * @details
 * @code
 * function sound:playSe(int setId, str resId)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @return				なし
 */
int sound::playSe(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, sound_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);

		const auto &pSoundEffect = app->getSoundEffect(setId, resId);
		app->sound().playSoundEffect(pSoundEffect);
	});
	return 0;
}

/** @brief BGM 再生を開始する。
 * @details
 * @code
 * function sound:playBgm(int setId, str resId)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @return				なし
 */
int sound::playBgm(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, sound_RawFieldName);
		int setId = getInt(L, 2, 0);
		const char *resId = luaL_checkstring(L, 3);

		auto &pBgm = app->getBgm(setId, resId);
		app->sound().playBgm(pBgm);
	});
	return 0;
}

/** @brief BGM 再生を停止する。
 * @details
 * @code
 * function sound:stopBgm()
 * end
 * @endcode
 *
 * @return				なし
 */
int sound::stopBgm(lua_State *L)
{
	exceptToLuaError(L, [L]() {
		auto *app = getPtrFromSelf<framework::Application>(L, sound_RawFieldName);

		app->sound().stopBgm();
	});
	return 0;
}

}
}
}
