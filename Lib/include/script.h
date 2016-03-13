#pragma once

#include "util.h"
#include "framework.h"
#include <lua.hpp>

namespace yappy {
/// Lua scripting library.
namespace lua {

class LuaError : public std::runtime_error {
public:
	LuaError(const std::string &msg, lua_State *L) noexcept;
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
	 */
	Lua();
	/** @brief Destruct lua_State.
	 */
	~Lua() = default;

	/** @brief Returns lua_State which this object has.
	 * @return lua_State
	 */
	lua_State *getLuaState() { return m_lua.get(); }

	void loadTraceLib();
	void callWithResourceLib(const char *funcName, framework::Application *app);
	void loadGraphLib(framework::Application *app);
	void loadSoundLib(framework::Application *app);

	/** @brief Load script file and eval it.
	 * @param[in] fileName	Script file name.
	 * @param[in] name		Chunk name. It will be used by Lua runtime for debug message.
	 *						If null, fileName is used.
	 */
	void loadFile(const wchar_t *fileName, const char *name = nullptr);

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
	 * @param[in] pushParamFunc	Will be called just before lua_pcall().
	 * @param[in] narg			Args count.
	 * @param[in] getRetFunc	Will be called just after lua_pcall().
	 * @param[in] nret			Return values count.
	 */
	template <class ParamFunc, class RetFunc>
	void callGlobal(const char *funcName,
		ParamFunc pushArgFunc, int narg, RetFunc getRetFunc, int nret)
	{
		lua_State *L = m_lua.get();
		// push global function
		lua_getglobal(L, funcName);
		// push args
		pushArgFunc(L);
		// call
		int ret = lua_pcall(L, narg, nret, 0);
		if (ret != LUA_OK) {
			throw LuaError("Call global function failed", L);
		}
		// get results
		getRetFunc(L);
	}
	/** @brief Calls global function. (No args, no return values)
	 * @param[in] funcName		Function name.
	 */
	void callGlobal(const char *funcName)
	{
		callGlobal(funcName, [](lua_State *L) {}, 0, [](lua_State *L) {}, 0);
	}

	/** @brief Dump Lua stack for debug.
	 */
	void dumpStack(void);

private:
	static void luaDeleter(lua_State *lua);
	using LuaDeleterType = decltype(&luaDeleter);

	std::unique_ptr<lua_State, LuaDeleterType> m_lua;
};

///////////////////////////////////////////////////////////////////////////////
// Export Functions (implemented in script_export.cpp)
// Put into class for documentation
///////////////////////////////////////////////////////////////////////////////
namespace lua_export {

struct trace {
	static int write(lua_State *L);
	trace() = delete;
};
const luaL_Reg trace_RegList[] = {
	{ "write",	trace::write },
	{ nullptr, nullptr }
};

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

}	// lua_export

}
}
