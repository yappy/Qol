#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"

namespace yappy {
namespace lua {
namespace export {

namespace {

template <class T>
inline T *getPtrFromUpvalue(lua_State *L, int uvInd)
{
	int idx = lua_upvalueindex(uvInd);
	ASSERT(lua_type(L, idx) == LUA_TLIGHTUSERDATA);
	void *p = lua_touserdata(L, idx);
	return static_cast<T *>(p);
}

template <class F>
inline int exceptToLuaError(lua_State *L, F proc)
{
	try {
		return proc();
	}
	catch (const std::exception &ex) {
		// push string ex.what() and throw
		// noreturn
		return luaL_error(L, "%s", ex.what());
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

inline double luanumToDouble(lua_State *L, int arg,
	lua_Number val, double min, double max)
{
	luaL_argcheck(L, val >= min, arg, "number too small or NaN");
	luaL_argcheck(L, val <= max, arg, "number too large or NaN");
	return static_cast<double>(val);
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

inline double getDouble(lua_State *L, int arg,
	double min = std::numeric_limits<double>::lowest(),
	double max = std::numeric_limits<double>::max())
{
	lua_Number val = luaL_checknumber(L, arg);
	return luanumToDouble(L, arg, val, min, max);
}

inline double getOptDouble(lua_State *L, int arg, double def,
	double min = std::numeric_limits<double>::lowest(),
	double max = std::numeric_limits<double>::max())
{
	lua_Number val = luaL_optnumber(L, arg, def);
	return luanumToDouble(L, arg, val, min, max);
}

}	// namespace

///////////////////////////////////////////////////////////////////////////////
// "trace" table
///////////////////////////////////////////////////////////////////////////////

/** @brief デバッグ出力する。
 * @details
 * 標準 print 関数は削除され、この関数で置き換えられます。
 * @code
 * function trace.write(...)
 * end
 * function print(...)
 * end
 * @endcode
 *
 * @param[in]	...	任意の型および数の出力する値
 * @return			なし
 *
 * @sa @ref yappy::debug
 */
int trace::write(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
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
		return 0;
	});
}

/** @brief メモリ上のバッファに高速なログ出力を行う。
 * @details
 * @code
 * function trace.perf(...)
 * end
 * @endcode
 *
 * @param[in]	...	任意の型および数の出力する値
 * @return			なし
 *
 * @sa @ref yappy::trace
 */
int trace::perf(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		int argc = lua_gettop(L);
		for (int i = 1; i <= argc; i++) {
			const char *str = ::lua_tostring(L, i);
			if (str != nullptr) {
				yappy::trace::write(str);
			}
			else {
				debug::writef("<%s>", luaL_typename(L, i));
			}
		}
		return 0;
	});
}

///////////////////////////////////////////////////////////////////////////////
// "sys" table
///////////////////////////////////////////////////////////////////////////////

/** @brief 別の Lua ソースファイルを実行する。
 * @details
 * @code
 * function sys.include(...)
 * end
 * @endcode
 * Lua 標準関数 dofile() とおおよそ同じです。
 * (ロード系の標準関数は全て削除されています。)
 * ファイルのロードはライブラリのローダを呼び出して行うようになっています。
 * デバッグ情報のロードも行うようになっています。
 *
 * @param[in]	...	ファイル名
 * @return			なし
 *
 * @sa @ref yappy::file
 */
int sys::include(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *lua = getPtrFromUpvalue<Lua>(L, 1);

		int argc = lua_gettop(L);
		for (int i = 1; i <= argc; i++) {
			const char *fileName = ::lua_tostring(L, i);
			luaL_argcheck(L, fileName != nullptr, i, "string needed");

			lua->loadFile(util::utf82wc(fileName).get(), false, false);
		}
		return 0;
	});
}

/** @brief ファイルを読む。
 * @details
 * @code
 * function sys.readFile(str fileName)
 * 	return ...;
 * end
 * @endcode
 * テキストファイルを読みます。
 * ファイルが存在しない場合、何も返しません(0個の値を返します)。
 * ファイル内に1行も存在しなかった場合も同様です。
 * ファイルが存在しない以外の理由で失敗した場合、エラーを発生させます。
 * 1行あたり読み取るのは1023文字までです。超過した分は捨てられます。
 * @code
 * -- 最初の3行をそれぞれ a, b, c に代入する
 * -- 足りない部分には nil が代入される
 * -- 3行あったかどうかは c が nil かを見ればよい
 * local a, b, c = sys.readFile("savedata.txt");
 * -- リストに変換する (x[1]から始まる)
 * local x = { sys.readFile("savedata.txt") };
 * -- 長さ(行数)を得る
 * local len = #x;
 * -- 全て出力する
 * for i = 1, #x do
 *	trace.write(x[i]);
 * end
 * @endcode
 *
 * @param[in] fileName	ファイル名
 * @return				各行の内容
 */
int sys::readFile(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		const char *fileName = luaL_checkstring(L, 1);

		FILE *tmpfp = nullptr;
		errno_t err = ::fopen_s(&tmpfp, fileName, "r");
		if (err == ENOENT) {
			// return;
			return 0;
		}
		else if (err != 0) {
			luaL_error(L, "File open error: %s, %d", fileName, err);
		}

		// return ...;
		int retcnt = 0;

		util::FilePtr fp(tmpfp);
		char buf[1024];
		while (std::fgets(buf, sizeof(buf), fp.get()) != nullptr) {
			// find new line
			char *pnl = std::strchr(buf, '\n');
			if (pnl != nullptr) {
				// OK, delete '\n'
				*pnl = '\0';
			}
			else {
				// line buffer over or EOF without new line
				// discard until NL is found or reach EOF
				char disc[1024];
				do {
					if (std::fgets(disc, sizeof(disc), fp.get()) == nullptr) {
						break;
					}
				} while (std::strchr(disc, '\n') == nullptr);
			}
			lua_pushstring(L, buf);
			retcnt++;
		}
		return retcnt;
	});
}

/** @brief ファイルを書く。
 * @details
 * @code
 * function sys.writeFile(str fileName, ...)
 * end
 * @endcode
 * テキストファイルを書き込みます。
 * ファイルのオープンや書き込みに失敗した場合、エラーを発生させます。
 * @code
 * -- 数値は文字列に自動で変換される
 * sys.writeFile("savedata.txt", "NoName", 1, 255);
 * local name, stage, hp = sys.readFile("savedata.txt");
 * -- リストを可変個の値に変換するには table.unpack() を使う
 * local list = { "NoName", 1, 255 };
 * sys.writeFile("savedata.txt", table.unpack(list));
 * @endcode
 *
 * @warning 文字列に含まれる改行をチェックしません。
 * @ref readFile() との整合性が崩れるので注意してください。
 *
 * @param[in] fileName	ファイル名
 * @param[in] ...		各行の内容
 * @return				なし
 */
int sys::writeFile(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		const char *fileName = luaL_checkstring(L, 1);

		FILE *tmpfp = nullptr;
		errno_t err = ::fopen_s(&tmpfp, fileName, "w");
		if (err != 0) {
			luaL_error(L, "File open error: %s, %d", fileName, err);
		}

		util::FilePtr fp(tmpfp);
		int argc = lua_gettop(L);
		for (int i = 2; i <= argc; i++) {
			// if lua error, destruct fp and throw
			const char *line = luaL_checkstring(L, i);
			std::fputs(line, fp.get());
			std::fputc('\n', fp.get());
		}
		return 0;
	});
}

///////////////////////////////////////////////////////////////////////////////
// "rand" table
///////////////////////////////////////////////////////////////////////////////

/** @brief 乱数シード用の値を生成する。
 * @details
 * @code
 * function rand.generateSeed()
 * end
 * @endcode
 *
 * @return	シード用の整数乱数
 *
 * @sa @ref framework::random::generateRandomSeed()
 */
int rand::generateSeed(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		unsigned int seed = framework::random::generateRandomSeed();
		lua_pushinteger(L, seed);
		return 1;
	});
}

/** @brief 乱数のシード値を設定する。
 * @details
 * @code
 * function rand.setSeed(int seed)
 * end
 * @endcode
 * シード値を設定しないと毎回同じ乱数が出てきてしまいます。
 * 初めに @ref generateSeed() で生成した乱数値をシードに設定してください。
 * 逆に同じシードを設定すると同じ乱数列を再現できます。
 *
 * @param[in]	seed	設定するシード値
 * @return				なし
 *
 * @sa @ref framework::random::setSeed()
 */
int rand::setSeed(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto seed = static_cast<unsigned int>(luaL_checkinteger(L, 1));
		framework::random::setSeed(seed);
		return 0;
	});
}

/** @brief 次の整数乱数を生成する。
 * @details
 * @code
 * function rand.nextInt(int a = 0, int b = 0x7fffffff)
 * end
 * @endcode
 *
 * @param[in]	a	最小値(含む)
 * @param[in]	b	最大値(含む)
 * @return		[a, b] のランダムな値
 *
 * @sa @ref framework::random::nextInt()
 */
int rand::nextInt(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		int a = getOptInt(L, 1, 0);
		int b = getOptInt(L, 2, std::numeric_limits<int>::max());
		luaL_argcheck(L, a <= b, 1, "Must be a <= b");

		int rnum = framework::random::nextInt(a, b);

		lua_pushinteger(L, rnum);
		return 1;
	});
}

/** @brief 次の浮動小数点乱数を生成する。
 * @details
 * @code
 * function rand.nextDouble(double a = 0.0, double b = 1.0)
 * end
 * @endcode
 *
 * @param[in]	a	最小値(含む)
 * @param[in]	b	最大値(含まない)
 * @return		[a, b) のランダムな値
 *
 * @sa @ref framework::random::nextInt()
 */
int rand::nextDouble(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		double a = getOptDouble(L, 1, 0.0);
		double b = getOptDouble(L, 2, 1.0);
		luaL_argcheck(L, a <= b, 1, "Must be a <= b");

		double rnum = framework::random::nextDouble(a, b);

		lua_pushnumber(L, rnum);
		return 1;
	});
}

///////////////////////////////////////////////////////////////////////////////
// "resource" table
///////////////////////////////////////////////////////////////////////////////

/** @brief テクスチャリソースを登録する。
 * @details
 * @code
 * function resource.addTexture(int setId, str resId, str path)
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
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);
		const char *path = luaL_checkstring(L, 3);

		app->addTextureResource(setId, resId, util::utf82wc(path).get());
		return 0;
	});
}

/** @brief フォントリソースを登録する。
 * @details
 * @code
 * function resource.addFont(int setId, str resId, str path)
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
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);
		const char *fontName = luaL_checkstring(L, 3);
		const char *startCharStr = luaL_checkstring(L, 4);
		luaL_argcheck(L, *startCharStr != '\0', 4, "empty string is NG");
		const char *endCharStr = luaL_checkstring(L, 4);
		luaL_argcheck(L, *endCharStr != '\0', 5, "empty string is NG");
		int w = getInt(L, 7, 0);
		int h = getInt(L, 8, 0);

		wchar_t startChar = util::utf82wc(startCharStr)[0];
		wchar_t endChar = util::utf82wc(endCharStr)[0];

		app->addFontResource(setId, resId, util::utf82wc(fontName).get(),
			startChar, endChar, w, h);
		return 0;
	});
}

/** @brief 効果音リソースを登録する。
 * @details
 * @code
 * function resource.addSe(int setId, str resId, str path)
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
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);
		const char *path = luaL_checkstring(L, 3);

		app->addSeResource(setId, resId, util::utf82wc(path).get());
		return 0;
	});
}

/** @brief BGMリソースを登録する。
 * @details
 * @code
 * function resource.addBgm(int setId, str resId, str path)
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
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);
		const char *path = luaL_checkstring(L, 3);

		app->addBgmResource(setId, resId, util::utf82wc(path).get());
		return 0;
	});
}

///////////////////////////////////////////////////////////////////////////////
// "graph" table
///////////////////////////////////////////////////////////////////////////////

/** @brief テクスチャのサイズを得る。
 * @details
 * @code
 * function graph.getParam()
 * 	return int w, int h, int refreshRate, bool vsync;
 * end
 * @endcode
 *
 * @retval	1	ディスプレイサイズ横
 * @retval	2	ディスプレイサイズ縦
 * @retval	3	リフレッシュレート(Hz)
 * @retval	4	vsync 待ちが有効なら true
 */
int graph::getParam(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);

		graphics::GraphicsParam param;
		app->getGraphicsParam(&param);

		lua_pushinteger(L, param.w);
		lua_pushinteger(L, param.h);
		lua_pushinteger(L, param.refreshRate);
		lua_pushboolean(L, param.vsync);
		return 4;
	});
}

/** @brief テクスチャのサイズを得る。
 * @details
 * @code
 * function graph.getTextureSize(int setId, str resId)
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
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);

		const auto &pTex = app->getTexture(setId, resId);

		lua_pushinteger(L, pTex->w);
		lua_pushinteger(L, pTex->h);
		return 2;
	});
}

/** @brief テクスチャを描画する。
 * @details
 * @code
 * function graph.drawTexture(int setId, str resId,
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
 * @sa @ref yappy::graphics::DGraphics::drawTexture()
 */
int graph::drawTexture(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);
		int dx = getInt(L, 3);
		int dy = getInt(L, 4);

		bool lrInv = lua_toboolean(L, 5) != 0;
		bool udInv = lua_toboolean(L, 6) != 0;
		int sx = getOptInt(L, 7, 0);
		int sy = getOptInt(L, 8, 0);
		int sw = getOptInt(L, 9, -1);
		int sh = getOptInt(L, 10, -1);
		int cx = getOptInt(L, 11, 0);
		int cy = getOptInt(L, 12, 0);
		float angle = getOptFloat(L, 13, 0.0f);
		float scaleX = getOptFloat(L, 14, 1.0f);
		float scaleY = getOptFloat(L, 15, 1.0f);
		float alpha = getOptFloat(L, 16, 1.0f);

		const auto &pTex = app->getTexture(setId, resId);
		app->graph().drawTexture(pTex, dx, dy, lrInv, udInv, sx, sy, sw, sh, cx, cy,
			angle, scaleX, scaleY, alpha);
		return 0;
	});
}

/** @brief 文字列を描画する。
 * @details
 * @code
 * function graph.drawString(int setId, str resId, str str, int dx, int dy,
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
 * @sa @ref yappy::graphics::DGraphics::drawString()
 */
int graph::drawString(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);
		const char *str = luaL_checkstring(L, 3);
		int dx = getInt(L, 4);
		int dy = getInt(L, 5);

		int color = getOptInt(L, 6, 0x000000);
		int ajustX = getOptInt(L, 7, 0);
		float scaleX = getOptFloat(L, 8, 1.0f);
		float scaleY = getOptFloat(L, 9, 1.0f);
		float alpha = getOptFloat(L, 10, 1.0f);

		const auto &pFont = app->getFont(setId, resId);
		app->graph().drawString(pFont, util::utf82wc(str).get(), dx, dy,
			color, ajustX, scaleX, scaleY, alpha);
		return 0;
	});
}

///////////////////////////////////////////////////////////////////////////////
// "sound" table
///////////////////////////////////////////////////////////////////////////////

/** @brief 効果音再生を開始する。
 * @details
 * @code
 * function sound.playSe(int setId, str resId)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @return				なし
 */
int sound::playSe(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);

		const auto &pSoundEffect = app->getSoundEffect(setId, resId);
		app->sound().playSoundEffect(pSoundEffect);
		return 0;
	});
}

/** @brief BGM 再生を開始する。
 * @details
 * @code
 * function sound.playBgm(int setId, str resId)
 * end
 * @endcode
 *
 * @param[in]	setId	リソースセットID(整数値)
 * @param[in]	resId	リソースID(文字列)
 * @return				なし
 */
int sound::playBgm(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);
		int setId = getInt(L, 1, 0);
		const char *resId = luaL_checkstring(L, 2);

		auto &pBgm = app->getBgm(setId, resId);
		app->sound().playBgm(pBgm);
		return 0;
	});
}

/** @brief BGM 再生を停止する。
 * @details
 * @code
 * function sound.stopBgm()
 * end
 * @endcode
 *
 * @return				なし
 */
int sound::stopBgm(lua_State *L)
{
	return exceptToLuaError(L, [L]() {
		auto *app = getPtrFromUpvalue<framework::Application>(L, 1);

		app->sound().stopBgm();
		return 0;
	});
}

}
}
}
