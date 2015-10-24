#pragma once

#include "util.h"
#include "graphics.h"
#include "sound.h"
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
	void loadGraphLib(graphics::Application *app);
	void loadSoundLib(sound::XAudio2 *sound);

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
	{ "write", trace::write },
	{ nullptr, nullptr }
};

struct graph {
	static int loadTexture(lua_State *L);
	static int getTextureSize(lua_State *L);
	static int drawTexture(lua_State *L);
	static int loadFont(lua_State *L);
	static int drawString(lua_State *L);
	graph() = delete;
};
const luaL_Reg graph_RegList[] = {
	{ "loadTexture", graph::loadTexture },
	{ "getTextureSize", graph::getTextureSize },
	{ "drawTexture", graph::drawTexture },
	{ "loadFont", graph::loadFont },
	{ "drawString", graph::drawString },
	{ nullptr, nullptr }
};
const char *const graph_RawFieldName = "_rawptr";

struct sound {
	static int playBgm(lua_State *L);
	static int stopBgm(lua_State *L);
	sound() = delete;
};
const luaL_Reg sound_RegList[] = {
	{ "playBgm", sound::playBgm },
	{ "stopBgm", sound::stopBgm },
	{ nullptr, nullptr }
};
const char *const sound_RawFieldName = "_rawptr";

}	// lua_export

}
}