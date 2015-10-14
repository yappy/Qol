#include "stdafx.h"
#include "include/script.h"
#include "include/file.h"

namespace yappy {
namespace lua {

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

void Lua::load(const wchar_t *fileName, const char *name)
{
	// TODO
	//::luaL_loadbufferx(m_lua.get());
}

}
}
