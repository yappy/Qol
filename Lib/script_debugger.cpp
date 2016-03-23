#include "stdafx.h"
#include "include/script_debugger.h"
#include "include/script.h"
#include "include/debug.h"

namespace yappy {
namespace lua {
namespace debugger {

namespace {

// for callbacks
LuaDebugger *s_dbg = nullptr;

// safe unset hook
class LuaHook : private util::noncopyable {
public:
	explicit LuaHook(LuaDebugger *dbg, lua_Hook hook, int mask, int count)
	{
		ASSERT(s_dbg == nullptr);
		s_dbg = dbg;
		lua_sethook(dbg->getLuaState(), hook, mask, count);
	}
	~LuaHook()
	{
		ASSERT(s_dbg != nullptr);
		lua_sethook(s_dbg->getLuaState(), nullptr, 0, 0);
		s_dbg = nullptr;
	}
};

}	// namespace

LuaDebugger::LuaDebugger(lua_State *L, bool debugEnable) :
	m_L(L), m_debugEnable(debugEnable)
{}

lua_State *LuaDebugger::getLuaState() const
{
	return m_L;
}

void LuaDebugger::pcall(int narg, int nret, int instLimit)
{
	lua_State *L = m_L;
	int mask = m_debugEnable ?
		(LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT) : 
		LUA_MASKCOUNT;
	{
		LuaHook hook(this, hookRaw, mask, instLimit);
		int base = lua_gettop(L) - narg;	// function index
		lua_pushcfunction(L, msghandler);	// push message handler
		lua_insert(L, base);				// put it under function and args
		int ret = lua_pcall(L, narg, nret, base);
		lua_remove(L, base);				// remove message handler
		if (ret != LUA_OK) {
			throw LuaError("Call global function failed", L);
		}
	} // unhook
}

// called when lua_error occurred
// (* Lua call stack is not unwinded yet *)
// return errmsg + backtrace
// copied from lua.c
int LuaDebugger::msghandler(lua_State *L)
{
	const char *msg = lua_tostring(L, 1);
	if (msg == NULL) {  /* is error object not a string? */
		if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
			lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
			return 1;  /* that is the message */
		else
			msg = lua_pushfstring(L, "(error object is a %s value)",
				luaL_typename(L, 1));
	}
	luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
	return 1;  /* return the traceback */
}

// static raw callback => non-static hook()
void LuaDebugger::hookRaw(lua_State *L, lua_Debug *ar)
{
	ASSERT(s_dbg != nullptr);
	ASSERT(s_dbg->m_L == L);
	s_dbg->hook(ar);
}

// switch to hookDebug() or hookNonDebug()
void LuaDebugger::hook(lua_Debug *ar)
{
	if (m_debugEnable) {
		hookDebug(ar);
	}
	else {
		hookNonDebug(ar);
	}
}

// non-debug mode main
void LuaDebugger::hookNonDebug(lua_Debug *ar)
{
	lua_State *L = m_L;
	switch (ar->event) {
	case LUA_HOOKCOUNT:
		luaL_error(L, "Instruction count exceeded");
		break;
	default:
		ASSERT(false);
	}
}

// debug mode hook main
void LuaDebugger::hookDebug(lua_Debug *ar)
{
	lua_State *L = m_L;
	bool brk = false;
	switch (ar->event) {
	case LUA_HOOKCALL:
	case LUA_HOOKTAILCALL:
		// break at entry point
		/*if (s_state == DbgState::INIT_BREAK) {
			brk = true;
			s_state = DbgState::CONT;
			break;
		}*/
		break;
	case LUA_HOOKRET:
		break;
	case LUA_HOOKLINE:
		// step in
		/*if (s_state == DbgState::STEP_IN) {
			brk = true;
			s_state = DbgState::CONT;
			break;
		}
		// search breakpoints
		lua_getinfo(L, "l", ar);
		if (s_bp.find(ar->currentline) != s_bp.end()) {
			brk = true;
			break;
		}*/
		break;
	case LUA_HOOKCOUNT:
		luaL_error(L, "Instruction count exceeded");
		break;
	default:
		ASSERT(false);
	}
	if (brk) {
		debug::writeLine(L"*** break ***");
		//summary(L, ar);
		//cmd_loop(L, ar);
	}
}

}
}
}
