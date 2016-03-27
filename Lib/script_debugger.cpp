#include "stdafx.h"
#include "include/script_debugger.h"
#include "include/script.h"
#include "include/debug.h"
#include "include/exceptions.h"

namespace yappy {
namespace lua {
namespace debugger {

namespace {

struct ExtraSpace {
	LuaDebugger *dbg;
};
static_assert(sizeof(ExtraSpace) <= LUA_EXTRASPACE,
	"lua extra space is not enough");

inline ExtraSpace &extra(lua_State *L)
{
	void *extra = lua_getextraspace(L);
	return *reinterpret_cast<ExtraSpace *>(extra);
}

// safe unset hook
class LuaHook : private util::noncopyable {
public:
	explicit LuaHook(LuaDebugger *dbg, lua_Hook hook, int mask, int count) :
		m_dbg(dbg)
	{
		lua_State *L = m_dbg->getLuaState();
		ASSERT(extra(L).dbg == nullptr);
		extra(L).dbg = dbg;
		lua_sethook(L, hook, mask, count);
	}
	~LuaHook()
	{
		lua_State *L = m_dbg->getLuaState();
		ASSERT(extra(L).dbg == m_dbg);
		lua_sethook(L, nullptr, 0, 0);
		extra(L).dbg = nullptr;
	}
private:
	LuaDebugger *m_dbg;
};

}	// namespace

LuaDebugger::LuaDebugger(lua_State *L, bool debugEnable) :
	m_L(L), m_debugEnable(debugEnable)
{
	extra(L).dbg = nullptr;
}

lua_State *LuaDebugger::getLuaState() const
{
	return m_L;
}

void LuaDebugger::loadDebugInfo(const char *name, const char *src, size_t size)
{
	lua_State *L = m_L;
	ChunkDebugInfo info;
	info.chunkName = name;

	// split src to lines
	std::string srcStr(src, size);
	std::regex re("([^\\r\\n]*)\\r?\\n?");
	std::smatch m;
	std::sregex_iterator iter(srcStr.cbegin(), srcStr.cend(), re);
	std::sregex_iterator end;
	for (; iter != end; ++iter) {
		// 0: all, 1: ([^\r\n]*)
		info.srcLines.emplace_back(iter->str(1));
	}

	// TODO: get valid lines
	if (!lua_isfunction(L, -1)) {
		throw std::logic_error("stack top must be chunk");
	}

	m_debugInfo.emplace_back(std::move(info));
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
	extra(L).dbg->hook(ar);
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
		L"fr", &LuaDebugger::fr,
		L"fr [<frame_no>]", L"Change current frame / Show detailed info of current frame",
		L"コールスタックのフレーム番号を指定してそのフレームに移動します。"
		"番号を指定しない場合、現在のフレームの詳細な情報を表示します。"
	},
	{
		L"eval", &LuaDebugger::eval,
		L"eval <lua script>",
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
		L"si", &LuaDebugger::si,
		L"si", L"Step in",
		L"新たな行に到達するまで実行します。関数呼び出しがあった場合、その中に入ります。"
	},
	{
		L"sout", nullptr,
		L"sout", L"Step out",
		L"TODO"
	},
	{
		L"bp", &LuaDebugger::bp,
		L"bp [-f <filename>] [<line>*]", L"Set/Show breakpoint",
		L"ブレークポイントを設定します。引数を指定しない場合、ブレークポイント一覧を表示します。"
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
	std::wregex re(L"\\S+");
	std::wsregex_iterator iter(line.cbegin(), line.cend(), re);
	std::wsregex_iterator end;
	std::vector<std::wstring> result;
	for (; iter != end; ++iter)
	{
		result.emplace_back(iter->str());
	}
	return result;
}

inline int stoi_s(const std::wstring &str, int base = 10)
{
	size_t idx = 0;
	int ret = std::stoi(str, &idx, base);
	if (idx != str.size()) {
		throw std::invalid_argument("invalid stoi argument");
	}
	return ret;
}

}	// namespace

// debug mode hook main
void LuaDebugger::hookDebug(lua_Debug *ar)
{
	lua_State *L = m_L;
	bool brk = false;
	switch (ar->event) {
	case LUA_HOOKCALL:
		break;
	case LUA_HOOKTAILCALL:
		break;
	case LUA_HOOKRET:
		break;
	case LUA_HOOKLINE:
		// break at entry point
		if (m_debugState == DebugState::INIT_BREAK) {
			brk = true;
			break;
		}
		// step in
		if (m_debugState == DebugState::STEP_IN) {
			brk = true;
			break;
		}
		// search breakpoints
		/*lua_getinfo(L, "l", ar);
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
		// set call stack top
		m_currentFrame = 0;
		debug::writeLine();
		debug::writeLine(L"[LuaDbg] ***** break *****");
		summaryOnBreak(ar);
		debug::writeLine(L"[LuaDbg] \"help\" command for usage");
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
				debug::writef(L"[LuaDbg] Command not found: %s\n", argv[0].c_str());
				continue;
			}
			argv.erase(argv.begin());
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
// Helpers
///////////////////////////////////////////////////////////////////////////////

void LuaDebugger::summaryOnBreak(lua_Debug *ar)
{
	lua_State *L = m_L;
	lua_getinfo(L, "nSltu", ar);
	const char *name = (ar->name == nullptr) ? "?" : ar->name;
	debug::writef("%s (%s) %s:%d",
		name, ar->what, ar->source, ar->currentline);
	printSrcLines(ar->source, ar->currentline, DefSrcLines);
}

void LuaDebugger::printSrcLines(const char *name, int line, int range)
{
	// find info.chunkName == name
	const auto &info = std::find_if(m_debugInfo.cbegin(), m_debugInfo.cend(),
		[name](const ChunkDebugInfo &cdi) { return (cdi.chunkName == name); });

	if (info != m_debugInfo.cend()) {
		line--;
		int uh = (range - 1) / 2;
		int dh = range - uh - 1;
		int beg = line - uh;
		beg = (beg < 0) ? 0 : beg;
		int end = line + dh;
		int ssize = static_cast<int>(info->srcLines.size());
		end = (end >= ssize) ? ssize - 1 : end;
		for (int i = beg; i <= end; i++) {
			char mark = (i == line) ? '>' : ' ';
			debug::writef("%c%6d: %s", mark, i + 1, info->srcLines.at(i).c_str());
		}
	}
	else {
		debug::writef("Debug info not found: %s", name);
	}
}

void LuaDebugger::printLocalAndUpvalue(lua_Debug *ar, int maxDepth, bool skipNoName)
{
	lua_State *L = m_L;
	{
		int n = 1;
		const char *name = nullptr;
		debug::writeLine(L"Local variables:");
		while ((name = lua_getlocal(L, ar, n)) != nullptr) {
			if (!skipNoName || name[0] != '(') {
				debug::writef("[%3d] %s = %s", n, name,
					luaValueToStr(L, -1, maxDepth, 0).c_str());
			}
			// pop value
			lua_pop(L, 1);
			n++;
		}
	}
	{
		// push current frame function
		lua_getinfo(L, "f", ar);
		int n = 1;
		const char *name = nullptr;
		debug::writeLine(L"Upvalues:");
		while ((name = lua_getupvalue(L, -1, n)) != nullptr) {
			int d = maxDepth;
			if (std::strcmp(name, "_ENV") == 0) {
				d = 1;
			}
			debug::writef("[%3d] %s = %s", n, name,
				luaValueToStr(L, -1, d, 0).c_str());
			// pop value
			lua_pop(L, 1);
			n++;
		}
		// pop function
		lua_pop(L, 1);
	}
}

// _ENV proxy meta-method for eval
namespace {
int evalIndex(lua_State *L)
{
	// param[1] = table, param[2] = key
	// upvalue[1] = orig _ENV, upvalue[2] = lua_Debug *
	ASSERT(lua_istable(L, lua_upvalueindex(1)));
	ASSERT(lua_islightuserdata(L, lua_upvalueindex(2)));
	auto *ar = static_cast<lua_Debug *>(
		lua_touserdata(L, lua_upvalueindex(2)));
	ASSERT(ar != nullptr);

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		// (1) local variables
		{
			int n = 1;
			const char *name = nullptr;
			while ((name = lua_getlocal(L, ar, n)) != nullptr) {
				if (name[0] != '(' && std::strcmp(key, name) == 0) {
					// return stack top value
					return 1;
				}
				lua_pop(L, 1);
				n++;
			}
		}
		// (2) upvalues
		{
			// push current frame function
			lua_getinfo(L, "f", ar);
			int n = 1;
			const char *name = nullptr;
			while ((name = lua_getupvalue(L, -1, n)) != nullptr) {
				if (std::strcmp(key, name) == 0) {
					// return stack top value
					return 1;
				}
				lua_pop(L, 1);
				n++;
			}
			// pop function
			lua_pop(L, 1);
		}
	}
	// (3) return _ENV[key]
	lua_pushvalue(L, 2);
	lua_gettable(L, lua_upvalueindex(1));
	return 1;
}

int evalNewIndex(lua_State *L)
{
	return luaL_error(L, "new index");
}
}	// namespace

void LuaDebugger::pushLocalEnv(lua_Debug *ar, int frameNo)
{
	lua_State *L = m_L;

	// push another _ENV for eval
	lua_newtable(L);
	int tabind = lua_gettop(L);

	// push metatable for another _ENV
	lua_newtable(L);

	lua_pushliteral(L, "__metatable");
	lua_pushliteral(L, "read only table");
	lua_settable(L, -3);

	lua_pushliteral(L, "__index");
	// upvalue[1] = orig _ENV
	lua_getglobal(L, "_G");
	ASSERT(lua_istable(L, -1));
	// upvalue[2] = lua_Debug
	lua_pushlightuserdata(L, ar);
	lua_pushcclosure(L, evalIndex, 2);
	lua_settable(L, -3);

	lua_pushliteral(L, "__newindex");
	// upvalue[1] = orig _ENV
	lua_getglobal(L, "_G");
	// upvalue[2] = lua_Debug
	lua_pushlightuserdata(L, ar);
	lua_pushcclosure(L, evalIndex, 2);
	lua_settable(L, -3);

	lua_setmetatable(L, -2);
}

///////////////////////////////////////////////////////////////////////////////
// Commands
///////////////////////////////////////////////////////////////////////////////
bool LuaDebugger::help(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	if (args.empty()) {
		for (const auto &entry : CmdList) {
			debug::writef(L"%s\n\t%s", entry.usage, entry.brief);
		}
	}
	else {
		for (const auto &cmdstr : args) {
			const CmdEntry *entry = searchCommandList(cmdstr);
			if (entry == nullptr) {
				debug::writef(L"[LuaDbg] Command not found: %s", cmdstr.c_str());
				return false;
			}
			debug::writef(L"%s\nUsage: %s\n\n%s",
				entry->brief, entry->usage, entry->description);
		}
	}
	return false;
}

bool LuaDebugger::bt(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	lua_State *L = m_L;
	int lv = 0;
	lua_Debug ar = { 0 };
	while (lua_getstack(L, lv, &ar)) {
		lua_getinfo(L, "nSltu", &ar);
		const char *name = (ar.name == nullptr) ? "?" : ar.name;
		debug::writef("[frame #%d] %s (%s) %s:%d", lv,
			name, ar.what, ar.source, ar.currentline);
		lv++;
	}
	return false;
}

bool LuaDebugger::fr(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	int lv = m_currentFrame;
	try {
		if (args.size() == 1) {
			lv = stoi_s(args[0], 10);
		}
		else if (args.size() >= 2) {
			throw std::invalid_argument("invalid argument");
		}
	}
	catch (...) {
		debug::writeLine(usage);
		return false;
	}

	lua_State *L = m_L;
	lua_Debug ar = { 0 };
	if (lua_getstack(L, lv, &ar)) {
		lua_getinfo(L, "nSltu", &ar);
		const char *name = (ar.name == nullptr) ? "?" : ar.name;
		debug::writef("[frame #%d] %s (%s) %s:%d", lv,
			name, ar.what, ar.source, ar.currentline);
		printSrcLines(ar.source, ar.currentline, DefSrcLines);
		printLocalAndUpvalue(&ar, DefTableDepth, true);
		// set current frame
		m_currentFrame = lv;
	}
	else {
		debug::writef(L"Invalid frame No: %d", lv);
	}
	return false;
}

bool LuaDebugger::src(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	// TODO
	debug::writeLine(L"Not implemented...");
	return false;
}

bool LuaDebugger::eval(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	if (args.empty()) {
		debug::writeLine(usage);
		return false;
	}

	lua_State *L = m_L;

	// "return" <args...> ";"
	std::wstring wsrc(L"return ");
	for (size_t i = 1; i < args.size(); i++) {
		wsrc += args[i];
	}
	wsrc += L";";
	auto src = util::wc2utf8(wsrc.c_str());

	lua_Debug ar = { 0 };
	lua_getstack(L, 0, &ar);
	lua_getinfo(L, "nSltu", &ar);
	try {
		// load str and push function
		int ret = luaL_loadstring(L, src.get());
		if (ret != LUA_OK) {
			throw LuaError("load failed", L);
		}

		// overwrite upvalue[1] (_ENV) of chunk
		pushLocalEnv(&ar, m_currentFrame);
		const char * uvName = lua_setupvalue(L, -2, 1);
		ASSERT(std::strcmp(uvName, "_ENV") == 0);

		// <func> => <ret...>
		int retBase = lua_gettop(L);
		ret = lua_pcall(L, 0, LUA_MULTRET, 0);
		if (ret != LUA_OK) {
			throw LuaError("pcall failed", L);
		}

		int retLast = lua_gettop(L);
		if (retBase > retLast) {
			debug::writeLine(L"No return values");
		}
		else {
			for (int i = retBase; i <= retLast; i++) {
				auto valstr = luaValueToStr(L, i, DefTableDepth);
				debug::writeLine(valstr.c_str());
			}
		}
		lua_settop(L, retBase);
	}
	catch (const LuaError &ex) {
		debug::writeLine(ex.what());
		return false;
	}

	return false;
}

bool LuaDebugger::cont(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(L"[LuaDbg] continue...");
	m_debugState = DebugState::CONT;
	return true;
}

bool LuaDebugger::si(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(L"[LuaDbg] step in...");
	m_debugState = DebugState::STEP_IN;
	return true;
}

bool LuaDebugger::bp(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	// TODO
	debug::writeLine(L"Not implemented...");
	if (args.empty()) {

	}
	else {

	}
	return false;
}

}
}
}
