﻿#include "stdafx.h"
#include "include/script.h"
#include "include/exceptions.h"
#include "include/debug.h"
#include "include/file.h"

namespace yappy {
namespace lua {

using error::throwTrace;

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
	lua_pop(L, 1);
}

namespace {

// Error happens outside protected env
// This is fatal and exit application
int atpanic(lua_State *L)
{
	error::throwTrace<std::runtime_error>("Lua panic");
	// Don't return
	return 0;
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

Lua::Lua(bool debugEnable, size_t maxHeapSize, size_t initHeapSize,
	int instLimit) :
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

	m_dbg = std::make_unique<debugger::LuaDebugger>(
		m_lua.get(), debugEnable, instLimit, maxHeapSize);

	debug::writeLine("Initializing lua OK");

	::lua_atpanic(m_lua.get(), atpanic);
	my_luaL_openlibs(m_lua.get());

	lua_State *L = m_lua.get();
	// delete print function
	lua_pushnil(L);
	lua_setglobal(L, "print");
	// delete load function
	lua_pushnil(L);
	lua_setglobal(L, "dofile");
	lua_pushnil(L);
	lua_setglobal(L, "loadfile");
	lua_pushnil(L);
	lua_setglobal(L, "load");
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

	// print <- trace.write
	lua_pushcfunction(L, export::trace::write);
	lua_setglobal(L, "print");
}

void Lua::loadSysLib()
{
	lua_State *L = m_lua.get();
	luaL_newlibtable(L, export::trace_RegList);
	// upvalue[1]: Lua *
	lua_pushlightuserdata(L, this);
	luaL_setfuncs(L, export::sys_RegList, 1);
	lua_setglobal(L, "sys");
}

void Lua::loadRandLib()
{
	lua_State *L = m_lua.get();
	luaL_newlib(L, export::rand_RegList);
	lua_setglobal(L, "rand");
}

void Lua::loadResourceLib(framework::Application *app)
{
	lua_State *L = m_lua.get();
	luaL_newlibtable(L, export::resource_RegList);
	// upvalue[1]: Application *
	lua_pushlightuserdata(L, app);
	luaL_setfuncs(L, export::resource_RegList, 1);
	lua_setglobal(L, "resource");
}

void Lua::loadGraphLib(framework::Application *app)
{
	lua_State *L = m_lua.get();
	luaL_newlibtable(L, export::graph_RegList);
	// upvalue[1]: Application *
	lua_pushlightuserdata(L, app);
	luaL_setfuncs(L, export::graph_RegList, 1);
	lua_setglobal(L, "graph");
}

void Lua::loadSoundLib(framework::Application *app)
{
	lua_State *L = m_lua.get();
	luaL_newlibtable(L, export::sound_RegList);
	// upvalue[1]: Application *
	lua_pushlightuserdata(L, app);
	luaL_setfuncs(L, export::sound_RegList, 1);
	lua_setglobal(L, "sound");
}

void Lua::loadFile(const wchar_t *fileName, bool autoBreak, bool prot)
{
	lua_State *L = m_lua.get();

	file::Bytes buf = file::loadFile(fileName);

	// push chunk function
	std::string chunkName("@");
	chunkName += util::wc2utf8(fileName).get();
	int ret = ::luaL_loadbufferx(L,
		reinterpret_cast<const char *>(buf.data()), buf.size(),
		chunkName.c_str(), "t");
	if (ret != LUA_OK) {
		throwTrace<LuaError>("Load script failed", L);
	}
	// prepare debug info
	m_dbg->loadDebugInfo(chunkName.c_str(),
		reinterpret_cast<const char *>(buf.data()), buf.size());
	// call it
	if (prot) {
		pcallInternal(0, 0, autoBreak);
	}
	else {
		lua_call(L, 0, 0);
	}
}

void Lua::pcallInternal(int narg, int nret, bool autoBreak)
{
	m_dbg->pcall(narg, nret, autoBreak);
}


namespace {

std::vector<std::string> luaValueToStrListInternal(
	lua_State *L, int ind, int maxDepth,
	int depth, int indent, char kind, int kindIndent,
	std::vector<const void *> visitedTable)
{
	std::vector<std::string> result;

	int tind = lua_absindex(L, ind);

	int type = lua_type(L, ind);
	if (type == LUA_TNONE) {
		throwTrace<std::logic_error>("invalid index: " + std::to_string(ind));
	}
	const char *typestr = lua_typename(L, type);
	// copy and tostring it
	lua_pushvalue(L, ind);
	const char *valstr = lua_tostring(L, -1);
	valstr = (valstr == NULL) ? "" : valstr;
	// pop copy
	lua_pop(L, 1);
	// boolean cannot be tostring
	if (type == LUA_TBOOLEAN) {
		valstr = lua_toboolean(L, ind) ? "true" : "false";
	}

	{
		std::string str;
		for (int i = 0; i < indent; i++) {
			if (i == kindIndent) {
				str += kind;
			}
			else {
				str += ' ';
			}
		}
		str += "(";
		str += typestr;
		str += ") ";
		str += valstr;
		result.emplace_back(std::move(str));
	}
	// table recursive call
	if (type == LUA_TTABLE && depth < maxDepth) {
		// call pairs(table)
		// __pairs(table) or
		// next, table, nil
		lua_getglobal(L, "pairs");
		lua_pushvalue(L, tind);
		lua_call(L, 1, 3);

		// initially: next, table, key=nil
		// save next and table
		int nextInd = lua_absindex(L, -3);
		int tableInd = lua_absindex(L, -2);
		// stack top is key=nil
		while (1) {
			// call next(table, key)
			lua_pushvalue(L, nextInd);
			lua_pushvalue(L, tableInd);
			lua_pushvalue(L, -3);
			// next, table, key, [next, table, key]
			lua_call(L, 2, 2);
			// next, table, key, [newkey, val]

			// end
			if (lua_isnil(L, -1)) {
				lua_pop(L, 2);
				break;
			}

			auto visited = [&visitedTable, L](int ind) -> bool {
				return std::find(visitedTable.cbegin(), visitedTable.cend(),
					lua_topointer(L, -2)) != visitedTable.cend();
			};
			// key:-2, value:-1
			if (!visited(-2)) {
				visitedTable.emplace_back(lua_topointer(L, -2));
				auto key = luaValueToStrListInternal(
					L, -2, maxDepth, depth + 1, indent + 4, 'K', indent, visitedTable);
				result.insert(result.end(), key.begin(), key.end());
				visitedTable.pop_back();
			}
			if (!visited(-1)) {
				visitedTable.emplace_back(lua_topointer(L, -1));
				auto val = luaValueToStrListInternal(
					L, -1, maxDepth, depth + 1, indent + 4, 'V', indent, visitedTable);
				result.insert(result.end(), val.begin(), val.end());
				visitedTable.pop_back();
			}
			// pop value, keep key stack top
			// next, table, key, newkey
			lua_pop(L, 1);
			// replace old key with new key
			// next, table, newkey
			lua_replace(L, -2);
		}
		// next, table, newkey => none
		lua_pop(L, 3);
	}
	return result;
}

}	// namespace

std::vector<std::string> luaValueToStrList(
	lua_State *L, int ind, int maxDepth)
{
	return luaValueToStrListInternal(L, ind, maxDepth, 0, 0, '\0', 0,
		std::vector<const void *>());
}

}	// namespace lua
}	// namespace yappy
