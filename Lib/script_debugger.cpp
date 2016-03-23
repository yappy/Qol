﻿#include "stdafx.h"
#include "include/script_debugger.h"
#include "include/script.h"
#include "include/debug.h"
#include "include/exceptions.h"

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
		// TODO: break at first
		m_debugState = DebugState::INIT_BREAK;
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Debugger
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
namespace {

struct CmdEntry {
	const wchar_t *cmd;
	bool (LuaDebugger::*exec)(const wchar_t *usage, const std::vector<std::wstring> &argv);
	const wchar_t *usage;
	const wchar_t *brief;
	const wchar_t *description;
};

const CmdEntry CmdList[] = {
	{
		L"help", &LuaDebugger::help,
		L"help [<cmd>]", L"Show command list or show specific command help",
		L"コマンド一覧を表示します。引数にコマンド名を指定すると詳細な説明を表示します。"
	},
	{
		L"bt", &LuaDebugger::bt,
		L"bt", L"Show backtrace (call stack)",
		L"バックトレース(関数呼び出し履歴)を表示します。"
	},
	{
		L"fr", nullptr,
		L"fr [<frame_no>]", L"Show detailed info of specific frame on call stack",
		L""
	},
	{
		L"src", nullptr,
		L"src [<file> <line>]", L"Show source file list or show specific file",
		L""
	},
	{
		L"cont", &LuaDebugger::cont,
		L"cont", L"Continue the program",
		L"実行を続行します。"
	},
	{
		L"si", nullptr,
		L"si", L"Step in",
		L"TODO"
	},
	{
		L"sout", nullptr,
		L"sout", L"Step out",
		L"TODO"
	},
	{
		L"bp", nullptr,
		L"bp [line]", L"Set/Show breakpoint",
		L"TODO"
	},
};

const CmdEntry *searchCommandList(const std::wstring &cmd)
{
	for (const auto &entry : CmdList) {
		if (cmd == entry.cmd) {
			return &entry;
		}
	}
	return nullptr;
}

std::vector<std::wstring> readLine()
{
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	error::checkWin32Result(hStdIn != INVALID_HANDLE_VALUE, "GetStdHandle() failed");

	wchar_t buf[1024];
	DWORD readSize = 0;
	BOOL ret = ::ReadConsole(hStdIn, buf, sizeof(buf), &readSize, nullptr);
	error::checkWin32Result(ret != 0, "ReadConsole() failed");
	std::wstring line(buf, readSize);

	// split the line
	// "[not "]*" or [not space]+
	std::wregex re(L"(?:\"([^\"]*)\")|(\\S+)");
	std::wsregex_iterator iter(line.cbegin(), line.cend(), re);
	std::wsregex_iterator end;
	std::vector<std::wstring> result;
	for (; iter != end; ++iter)
	{
		if (iter->str(1) != L"") {
			result.emplace_back(iter->str(1));
		}
		else {
			result.emplace_back(iter->str(2));
		}
	}
	return result;
}

}	// namespace

// debug mode hook main
void LuaDebugger::hookDebug(lua_Debug *ar)
{
	lua_State *L = m_L;
	bool brk = false;
	switch (ar->event) {
	case LUA_HOOKCALL:
	case LUA_HOOKTAILCALL:
		// break at entry point
		if (m_debugState == DebugState::INIT_BREAK) {
			brk = true;
			m_debugState = DebugState::CONT;
			break;
		}
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
		debug::writeLine(L"[LuaDbg] ***** break *****");
		debug::writeLine(L"[LuaDbg] \"help\" command for usage");
		//summary(ar);
		cmdLoop(ar);
	}
}

void LuaDebugger::cmdLoop(lua_Debug *ar)
{
	try {
		while (1) {
			debug::write(L"[LuaDbg] > ");
			std::vector<std::wstring> argv = readLine();
			if (argv.empty()) {
				continue;
			}
			const CmdEntry *entry = searchCommandList(argv[0]);
			if (entry == nullptr) {
				wprintf(L"[LuaDbg] Command not found: %s\n", argv[0].c_str());
				continue;
			}
			if ((this->*(entry->exec))(entry->usage, argv)) {
				break;
			}
			debug::writeLine();
		}
	}
	catch (error::Win32Error &) {
		// not AllocConsole()-ed?
		debug::writeLine(L"Lua Debugger cannot read from stdin.");
		debug::writeLine(L"continue...");
	}
}

///////////////////////////////////////////////////////////////////////////////
// Commands
///////////////////////////////////////////////////////////////////////////////
bool LuaDebugger::help(const wchar_t *usage, const std::vector<std::wstring> &argv)
{
	if (argv.size() == 1) {
		for (const auto &entry : CmdList) {
			debug::writef(L"%s\n\t%s", entry.usage, entry.brief);
		}
	}
	else {
		const CmdEntry *entry = searchCommandList(argv[1]);
		if (entry == nullptr) {
			debug::writef(L"[LuaDbg] Command not found: %s", argv[1].c_str());
			return false;
		}
		debug::writef(L"%s\nUsage: %s\n\n%s",
			entry->brief, entry->usage, entry->description);
	}
	return false;
}

bool LuaDebugger::cont(const wchar_t *usage, const std::vector<std::wstring> &argv)
{
	debug::writeLine(L"[LuaDbg] continue...");
	return true;
}

bool LuaDebugger::bt(const wchar_t *usage, const std::vector<std::wstring> &argv)
{
	lua_State *L = m_L;
	int lv = 0;
	lua_Debug ar = { 0 };
	while (lua_getstack(L, lv, &ar)) {
		debug::writef("[frame #%d]", lv);
		lua_getinfo(L, "nSltu", &ar);
		debug::writef("name=%s", ar.name);
		debug::writef("what=%s", ar.what);
		debug::writef("source=%s", ar.source);
		debug::writef("curentline=%d", ar.currentline);
		//print_src(ar.currentline, 21);
		//print_locals(L, &ar);
		lv++;
	}
	return false;
}

}
}
}
