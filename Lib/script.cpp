#include "stdafx.h"
#include "include/script.h"
#include "include/exceptions.h"
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

void *Lua::luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	auto *lua = reinterpret_cast<Lua *>(ud);
	if (nsize == 0) {
		// free(ptr);
		//debug::writef(L"luaAlloc(free)    %p, %08zx, %08zx", ptr, osize, nsize);
		if (ptr != nullptr) {
			BOOL ret = ::HeapFree(lua->m_heap.get(), 0, ptr);
			if (!ret) {
				debug::writeLine(L"[warning] HeapFree() failed");
			}
		}
		return nullptr;
	}
	else {
		// realloc(ptr, nsize);
		if (nsize >= 0x7FFF8) {
			debug::writef(L"[warning] Attempt to allocate %zu bytes", nsize);
		}
		if (ptr == nullptr) {
			//debug::writef(L"luaAlloc(alloc)   %p, %08zx, %08zx", ptr, osize, nsize);
			return ::HeapAlloc(lua->m_heap.get(), 0, nsize);
		}
		else {
			//debug::writef(L"luaAlloc(realloc) %p, %08zx, %08zx", ptr, osize, nsize);
			return ::HeapReAlloc(lua->m_heap.get(), 0, ptr, nsize);
		}
	}
}

Lua::Lua(bool debugEnable, size_t maxHeapSize, size_t initHeapSize) :
	m_debugEnable(debugEnable)
{
	debug::writeLine("Initializing lua...");
	HANDLE tmpHeap = ::HeapCreate(HeapOption, initHeapSize, maxHeapSize);
	error::checkWin32Result(tmpHeap != nullptr, "HeapCreate() failed");
	m_heap.reset(tmpHeap);

	lua_State *tmpLua = lua_newstate(luaAlloc, this);
	if (tmpLua == nullptr) {
		throw std::bad_alloc();
	}
	m_lua.reset(tmpLua);
	m_dbg = std::make_unique<debugger::LuaDebugger>(m_lua.get(), debugEnable);
	debug::writeLine("Initializing lua OK");

	::lua_atpanic(m_lua.get(), atpanic);
	my_luaL_openlibs(m_lua.get());
}

lua_State *Lua::getLuaState() const
{
	return m_lua.get();
}

Lua::~Lua()
{
	debug::writeLine("Finalize lua");
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

void Lua::loadFile(const wchar_t *fileName, int instLimit)
{
	lua_State *L = m_lua.get();

	file::Bytes buf = file::loadFile(fileName);

	// push chunk function
	auto cvtName = util::wc2utf8(fileName);
	int ret = ::luaL_loadbufferx(L,
		reinterpret_cast<const char *>(buf.data()), buf.size(),
		cvtName.get(), "t");
	if (ret != LUA_OK) {
		throw LuaError("Load script failed", L);
	}
	// prepare debug info
	m_dbg->loadDebugInfo(cvtName.get(),
		reinterpret_cast<const char *>(buf.data()), buf.size());
	// call it
	pcallInternal(0, LUA_MULTRET, instLimit);
}

void Lua::pcallInternal(int narg, int nret, int instLimit)
{
	m_dbg->pcall(narg, nret, instLimit);
}

}
}
