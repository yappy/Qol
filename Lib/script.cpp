#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"
#include "include/file.h"

namespace yappy {
namespace lua {

LuaError::LuaError(const std::string &msg, lua_State *L) :
	runtime_error("")
{
	const char *str = lua_tostring(L, -1);
	if (str == nullptr) {
		m_what = msg;
	}
	else {
		m_what = msg + ": " + str;
	}
}

namespace {

// Error happens outside protected env
// This is fatal and exit application
int atpanic(lua_State *L)
{
	throw std::runtime_error("Lua panic");
	// Don't return
}

// lua_Hook which always raises an error
void alwaysErrorHook(lua_State *L, lua_Debug *ar)
{
	luaL_error(L, "Execute instructions count exceeded");
}

class LuaHook : private util::noncopyable {
public:
	explicit LuaHook(lua_State *L, lua_Hook hook, int count) : m_lua(L)
	{
		if (count != 0) {
			lua_sethook(m_lua, hook, LUA_MASKCOUNT, count);
		}
	}
	~LuaHook()
	{
		lua_sethook(m_lua, nullptr, 0, 0);
	}
private:
	lua_State *m_lua;
};

///////////////////////////////////////////////////////////////////////////////
// Copied from linit.c
///////////////////////////////////////////////////////////////////////////////
const luaL_Reg loadedlibs[] = {
	{ "_G", luaopen_base },
//	{ LUA_LOADLIBNAME, luaopen_package },
	{ LUA_COLIBNAME, luaopen_coroutine },
	{ LUA_TABLIBNAME, luaopen_table },
//	{ LUA_IOLIBNAME, luaopen_io },
//	{ LUA_OSLIBNAME, luaopen_os },
	{ LUA_STRLIBNAME, luaopen_string },
	{ LUA_MATHLIBNAME, luaopen_math },
	{ LUA_UTF8LIBNAME, luaopen_utf8 },
//	{ LUA_DBLIBNAME, luaopen_debug },
	{ NULL, NULL }
};

LUALIB_API void my_luaL_openlibs(lua_State *L) {
	const luaL_Reg *lib;
	/* "require" functions from 'loadedlibs' and set results to global table */
	for (lib = loadedlibs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
}
///////////////////////////////////////////////////////////////////////////////
// Copied from linit.c END
///////////////////////////////////////////////////////////////////////////////

}	// namespace

void Lua::LuaDeleter::operator()(lua_State *lua)
{
	::lua_close(lua);
}

Lua::Lua()
{
	lua_State *tmpLua = ::luaL_newstate();
	if (tmpLua == nullptr) {
		throw std::bad_alloc();
	}
	m_lua.reset(tmpLua);

	::lua_atpanic(m_lua.get(), atpanic);
	my_luaL_openlibs(m_lua.get());
}

lua_State *Lua::getLuaState() const {
	return m_lua.get();
}

void Lua::loadTraceLib()
{
	lua_State *L = m_lua.get();
	luaL_newlib(L, export::trace_RegList);
	lua_setglobal(L, "trace");
}

void Lua::callWithResourceLib(const char *funcName, framework::Application *app,
	int instLimit)
{
	callGlobal(funcName, instLimit,
		[app](lua_State *L) {
			// args
			// stack[1]: resource function table
			luaL_newlib(L, export::resource_RegList);
			lua_pushstring(L, export::resource_RawFieldName);
			lua_pushlightuserdata(L, app);
			lua_settable(L, -3);
		}, 1,
		[](lua_State *L) {}, 0);
}

void Lua::loadGraphLib(framework::Application *app)
{
	lua_State *L = m_lua.get();
	luaL_newlib(L, export::graph_RegList);
	lua_pushstring(L, export::graph_RawFieldName);
	lua_pushlightuserdata(L, app);
	lua_settable(L, -3);
	lua_setglobal(L, "graph");
}

void Lua::loadSoundLib(framework::Application *app)
{
	lua_State *L = m_lua.get();
	luaL_newlib(L, export::sound_RegList);
	lua_pushstring(L, export::sound_RawFieldName);
	lua_pushlightuserdata(L, app);
	lua_settable(L, -3);
	lua_setglobal(L, "sound");
}

void Lua::loadFile(const wchar_t *fileName, const char *name)
{
	lua_State *L = m_lua.get();

	file::Bytes buf = file::loadFile(fileName);

	// Use fileName if name is nullptr
	auto cvtName = util::wc2utf8(fileName);
	name = (name == nullptr) ? cvtName.get() : name;
	int ret = ::luaL_loadbufferx(L,
		reinterpret_cast<const char *>(buf.data()), buf.size(),
		name, "t");
	if (ret != LUA_OK) {
		throw LuaError("Load script failed", L);
	}

	ret = ::lua_pcall(L, 0, LUA_MULTRET, 0);
	if (ret != LUA_OK) {
		throw LuaError("Execute chunk failed", L);
	}
}

int Lua::pcallWithLimit(lua_State *L,
	int narg, int nret, int msgh, int instLimit)
{
	LuaHook hook(L, alwaysErrorHook, instLimit);
	int ret = lua_pcall(L, narg, nret, 0);
	if (ret != LUA_OK) {
		throw LuaError("Call global function failed", L);
	}
	return ret;
}

}
}
