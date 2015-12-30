#pragma once

#include "util.h"
#include "framework.h"
#include <lua.hpp>

namespace yappy {
namespace lua {

class LuaError : public std::runtime_error {
public:
	LuaError(const std::string &msg, lua_State *L) noexcept;
	const char *what() const override
	{ return m_what.c_str(); }
private:
	std::string m_what;
};

class Lua {
public:
	Lua();
	~Lua() = default;

	lua_State *getLuaState() { return m_lua.get(); }

	void loadTraceLib();
	void callWithResourceLib(const char *funcName, framework::Application *app);
	void loadGraphLib(framework::Application *app);
	void loadSoundLib(framework::Application *app);

	void loadFile(const wchar_t *fileName, const char *name = nullptr);

	template <class ParamFunc, class RetFunc>
	void callGlobal(const char *funcName,
		ParamFunc pushParamFunc, int narg, RetFunc getRetFunc, int nret)
	{
		lua_State *L = m_lua.get();
		// push global function
		lua_getglobal(L, funcName);
		// push parameters
		pushParamFunc(L);
		// call
		int ret = lua_pcall(L, narg, nret, 0);
		if (ret != LUA_OK) {
			throw LuaError("Call global function failed", L);
		}
		// get results
		getRetFunc(L);
	}
	void callGlobal(const char *funcName)
	{
		callGlobal(funcName, [](lua_State *L) {}, 0, [](lua_State *L) {}, 0);
	}

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
