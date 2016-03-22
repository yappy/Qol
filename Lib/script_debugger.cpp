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

// copied from lua.c
int msghandler(lua_State *L)
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

}	// namespace

LuaDebugger::LuaDebugger(lua_State *L) : m_lua(L)
{}

lua_State *LuaDebugger::getLuaState() const
{
	return m_lua;
}

void LuaDebugger::pcall(int narg, int nret, int instLimit, bool debug)
{
	int mask = debug ?
		LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT : 0;
	{
		LuaHook hook(this, hook, mask, instLimit);
		int base = lua_gettop(m_lua) - narg;  /* function index */
		lua_pushcfunction(m_lua, msghandler);  /* push message handler */
		lua_insert(m_lua, base);  /* put it under function and args */
		int ret = lua_pcall(m_lua, narg, nret, 0);
		lua_remove(m_lua, base);  /* remove message handler from the stack */
		if (ret != LUA_OK) {
			throw LuaError("Call global function failed", m_lua);
		}
	} // unhook
}

void LuaDebugger::hook(lua_State *L, lua_Debug *ar)
{
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
		// check instruction limit
		// check interrupt from another thread
		break;
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
