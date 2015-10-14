#include "stdafx.h"
#include "include/script.h"
#include "include/debug.h"
#include "include/file.h"
#include <lua.hpp>

namespace yappy {
namespace lua {

LuaError::LuaError(const std::string &msg, lua_State *L) noexcept :
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
	{ LUA_DBLIBNAME, luaopen_debug },
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

namespace {

///////////////////////////////////////////////////////////////////////////////
// "trace" interface
///////////////////////////////////////////////////////////////////////////////
int trace_write(lua_State *L)
{
	int argc = lua_gettop(L);
	for (int i = 1; i <= argc; i++) {
		const char *str = ::lua_tostring(L, i);
		str = (str == nullptr) ? "<?>" : str;
		debug::writeLine(str);
	}
	return 0;
}

const luaL_Reg TraceLib[] =
{
	{ "write", trace_write },
	{ nullptr, nullptr }
};
///////////////////////////////////////////////////////////////////////////////
// "trace" interface END
///////////////////////////////////////////////////////////////////////////////

}	// namespace

void Lua::luaDeleter(lua_State *lua)
{
	::lua_close(lua);
}

Lua::Lua() : m_lua(nullptr, luaDeleter)
{
	lua_State *tmpLua = ::luaL_newstate();
	if (tmpLua == nullptr) {
		throw std::bad_alloc();
	}
	m_lua.reset(tmpLua);

	::lua_atpanic(m_lua.get(), atpanic);
	my_luaL_openlibs(m_lua.get());
}

void Lua::loadTraceLib()
{
	lua_State *L = m_lua.get();
	luaL_newlib(L, TraceLib);
	lua_setglobal(L, "trace");
}

void Lua::load(const wchar_t *fileName, const char *name)
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

void Lua::dumpStack()
{
	lua_State *L = m_lua.get();

	debug::writeLine("[Stack]----------------------");
	const int num = lua_gettop(L);
	if (num == 0) {
		debug::writeLine("No stack.");
		return;
	}
	for (int i = num; i >= 1; i--) {
		debug::writef(L"%03d(%04d): ", i, -num + i - 1);
		int type = lua_type(L, i);
		switch (type) {
		case LUA_TNIL:
			debug::writeLine(L"NIL");
			break;
		case LUA_TBOOLEAN:
			debug::writef(L"BOOLEAN %s", lua_toboolean(L, i) ? L"true" : L"false");
			break;
		case LUA_TLIGHTUSERDATA:
			debug::writeLine(L"LIGHTUSERDATA");
			break;
		case LUA_TNUMBER:
			debug::writef(L"NUMBER %f", lua_tonumber(L, i));
			break;
		case LUA_TSTRING:
			debug::writef("STRING %s", lua_tostring(L, i));
			break;
		case LUA_TTABLE:
			debug::writeLine(L"TABLE");
			break;
		case LUA_TFUNCTION:
			debug::writeLine(L"FUNCTION");
			break;
		case LUA_TUSERDATA:
			debug::writeLine(L"USERDATA");
			break;
		case LUA_TTHREAD:
			debug::writeLine(L"THREAD");
			break;
		}
	}
	debug::writeLine(L"-----------------------------");
}

}
}
