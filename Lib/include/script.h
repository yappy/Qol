﻿#pragma once

#include "util.h"
#include "framework.h"
#include <lua.hpp>

namespace yappy {
/// Lua scripting library.
namespace lua {

class LuaError : public std::runtime_error {
public:
	LuaError(const std::string &msg, lua_State *L);
	const char *what() const override
	{ return m_what.c_str(); }
private:
	std::string m_what;
};

/** @brief Lua state manager.
 * @details Each Lua object manages one lua_State.
 */
class Lua : private util::noncopyable {
public:
	/** @brief Create new lua_State and open standard libs.
	 * @param[in]	maxHeapSize		Max memory usage
	 *								(only virtual address range will be reserved at first)
	 * @param[in]	initHeapSize	Initial commit size (physical memory mapped)
	 */
	explicit Lua(size_t maxHeapSize, size_t initHeapSize = 1024 * 1024);
	/** @brief Destruct lua_State.
	 */
	~Lua();

	/** @brief Returns lua_State which this object has.
	 * @return lua_State
	 */
	lua_State *getLuaState() const;

	void loadTraceLib();
	void callWithResourceLib(const char *funcName, framework::Application *app,
		int instLimit = 0);
	void loadGraphLib(framework::Application *app);
	void loadSoundLib(framework::Application *app);

	/** @brief Load script file and eval it.
	 * @param[in] fileName	Script file name.
	 * @param[in] name		Chunk name. It will be used by Lua runtime for debug message.
	 *						If null, fileName is used.
	 */
	void loadFile(const wchar_t *fileName, const char *name = nullptr);

	struct doNothing {
		void operator ()(lua_State *L) {}
	};

	/** @brief Calls global function.
	 * @details pushParamFunc and getRetFunc must be able to be called by:
	 * @code
	 * pushParamFunc(lua_State *);
	 * getRetFunc(lua_State *);
	 * @endcode
	 * (lambda expr is recommended)
	 * @code
	 * [](lua_State *L) {
	 *     lua_pushXXX(L, ...);
	 * }
	 * @endcode
	 *
	 * @param[in] funcName		Function name.
	 * @param[in] instLimit		Instruction count limit for prevent inf loop. (no limit if 0)
	 * @param[in] pushParamFunc	Will be called just before lua_pcall().
	 * @param[in] narg			Args count.
	 * @param[in] getRetFunc	Will be called just after lua_pcall().
	 * @param[in] nret			Return values count.
	 */
	template <class ParamFunc = doNothing, class RetFunc = doNothing>
	void callGlobal(const char *funcName, int instLimit = 0,
		ParamFunc pushArgFunc = doNothing(), int narg = 0,
		RetFunc getRetFunc = doNothing(), int nret = 0)
	{
		lua_State *L = m_lua.get();
		lua_getglobal(L, funcName);
		// push args
		pushArgFunc(L);
		// call
		pcallWithLimit(L, narg, nret, 0, instLimit);
		// get results
		getRetFunc(L);
	}

private:
	static const DWORD HeapOption = HEAP_NO_SERIALIZE;

	struct LuaDeleter {
		void operator()(lua_State *L);
	};

	util::HeapPtr m_heap;
	std::unique_ptr<lua_State, LuaDeleter> m_lua;

	static void *luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize);
	static int pcallWithLimit(lua_State *L,
		int narg, int nret, int msgh, int instLimit);
};

///////////////////////////////////////////////////////////////////////////////
// Export Functions (implemented in script_export.cpp)
// Put into class for documentation
// Function description is written in script_export.cpp
///////////////////////////////////////////////////////////////////////////////

/** @brief C++ から Lua へ公開する関数。(Lua関数仕様)
 * @sa lua::Lua
 */
namespace export {

/** @brief デバッグ出力関数。<b>trace</b>グローバルテーブルに提供。
 * @details
 * @code
 * trace = {};
 * @endcode
 * デバッグ出力を本体側のロギングシステムに転送します。
 *
 * @sa debug
 */
struct trace {
	static int write(lua_State *L);
	trace() = delete;
};
const luaL_Reg trace_RegList[] = {
	{ "write",	trace::write },
	{ nullptr, nullptr }
};

/** @brief 使用リソース登録関数。
 * @details
 * 各関数は最初の引数に self オブジェクトが必要です。
 * リソースはリソースセットID(整数)とリソースID(文字列)で識別されます。
 * 何らかの初期化関数の引数としてテーブルが渡されます(今のところ)。
 * その中でリソースを登録した後、C++側で
 * framework::Application::loadResourceSet()
 * を呼ぶとリソースが使用可能になります。
 * @sa framework::Application
 * @sa framework::ResourceManager
 */
struct resource {
	static int addTexture(lua_State *L);
	static int addFont(lua_State *L);
	static int addSe(lua_State *L);
};
const luaL_Reg resource_RegList[] = {
	{ "addTexture",	resource::addTexture },
	{ "addFont",	resource::addFont },
	{ "addSe",		resource::addSe },
	{ nullptr, nullptr }
};
const char *const resource_RawFieldName = "_rawptr";

/** @brief グラフィックス描画関連関数。<b>graph</b>グローバルテーブルに提供。
 * @details
 * @code
 * graph = {};
 * @endcode
 * 各関数は最初の引数に self オブジェクトが必要です。
 * 描画するテクスチャリソースはリソースセットID(整数)とリソースID(文字列)で
 * 指定します。
 * @sa lua::export::resource
 * @sa graphics::DGraphics
 */
struct graph {
	static int getTextureSize(lua_State *L);
	static int drawTexture(lua_State *L);
	static int drawString(lua_State *L);
	graph() = delete;
};
const luaL_Reg graph_RegList[] = {
	{ "getTextureSize",	graph::getTextureSize },
	{ "drawTexture",	graph::drawTexture },
	{ "drawString",		graph::drawString },
	{ nullptr, nullptr }
};
const char *const graph_RawFieldName = "_rawptr";

/** @brief 音声再生関連関数。<b>sound</b>グローバルテーブルに提供。
 * @details
 * @code
 * sound = {};
 * @endcode
 * 各関数は最初の引数に self オブジェクトが必要です。
 * @sa lua::export::resource
 * @sa sound::XAudio2
 *
 * @todo 効果音再生がまだ。
 */
struct sound {
	static int playBgm(lua_State *L);
	static int stopBgm(lua_State *L);
	sound() = delete;
};
const luaL_Reg sound_RegList[] = {
	{ "playBgm",	sound::playBgm },
	{ "stopBgm",	sound::stopBgm },
	{ nullptr, nullptr }
};
const char *const sound_RawFieldName = "_rawptr";

}	// export

}
}
