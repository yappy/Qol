#include "stdafx.h"
#include "include/script_debugger.h"
#include "include/script.h"
#include "include/debug.h"
#include "include/exceptions.h"

#define ENABLE_LUAIMPL_DEPENDENT

#ifdef ENABLE_LUAIMPL_DEPENDENT
extern "C" {
#include <lobject.h>
#include <lstate.h>
#include <ldebug.h>
}
#endif

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

#ifdef ENABLE_LUAIMPL_DEPENDENT
namespace impldep {

inline Proto *toproto(lua_State *L, int i)
{
	return getproto(L->top + (i));
}

template <class F>
void validLines(Proto *f, F callback)
{
	int n = f->sizecode;
	for (int pc = 0; pc < n; pc++) {
		int line = getfuncline(f, pc);
		callback(line);
	}
	for (int i = 0; i < f->sizep; i++) {
		validLines(f->p[i], callback);
	}
}

template <class F>
void forAllValidLines(lua_State *L, F callback)
{
	Proto *f = toproto(L, -1);
	validLines(f, callback);
}

}
#endif

}	// namespace

LuaDebugger::LuaDebugger(lua_State *L, bool debugEnable, int instLimit) :
	m_L(L), m_debugEnable(debugEnable)
{
	extra(L).dbg = this;
	// switch hook condition by debugEnable
	int mask = m_debugEnable ?
		(LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT) :
		LUA_MASKCOUNT;
	lua_sethook(L, hookRaw, mask, instLimit);
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
	// replace tab
	std::regex tabspace("\\t");
	for (; iter != end; ++iter) {
		// 0: all, 1: ([^\r\n]*)
		auto result = std::regex_replace(iter->str(1), tabspace, "  ");
		info.srcLines.emplace_back(result);
	}

	// get valid lines
	info.validLines.resize(info.srcLines.size(), 0);
	if (!lua_isfunction(L, -1)) {
		throw std::logic_error("stack top must be chunk");
	}
#ifdef ENABLE_LUAIMPL_DEPENDENT
	impldep::forAllValidLines(L, [&info](int line) {
		line--;
		if (line >= 0 && static_cast<size_t>(line) < info.validLines.size()) {
			info.validLines[line] = 1;
		}
	});
#endif

	// breakpoints (all false)
	info.breakPoints.resize(info.srcLines.size(), 0);

	m_debugInfo.emplace(name, std::move(info));
}

void LuaDebugger::pcall(int narg, int nret, bool autoBreak)
{
	lua_State *L = m_L;
	{
		// break at the first line?
		m_debugState = autoBreak ?
			DebugState::BREAK_LINE_ANY : DebugState::CONT;
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
		L"help [<cmd>...]", L"コマンドリスト、コマンド詳細説明表示",
		L"コマンド一覧を表示します。引数にコマンド名を指定すると詳細な説明を表示します。"
	},
	{
		L"conf", &LuaDebugger::conf,
		L"conf", L"Lua バージョンとコンパイル時設定表示",
		L"Luaのバージョンとコンパイル時設定を表示します。"
	},
	{
		L"mem", &LuaDebugger::mem,
		L"mem [-gc]", L"メモリ使用状況表示",
		L"現在のメモリ使用量を表示します。-gc をつけると full GC を実行します。"
	},
	{
		L"bt", &LuaDebugger::bt,
		L"bt", L"バックトレース(コールスタック)表示",
		L"バックトレース(関数呼び出し履歴)を表示します。"
	},
	{
		L"fr", &LuaDebugger::fr,
		L"fr [<frame_no>]", L"カレントフレームの変更、カレントフレームの情報表示",
		L"コールスタックのフレーム番号を指定してそのフレームに移動します。"
		"番号を指定しない場合、現在のフレームの詳細な情報を表示します。"
	},
	{
		L"eval", &LuaDebugger::eval,
		L"eval <lua_script>", L"Lua スクリプトの実行",
		L"引数を連結して Lua スクリプトとしてカレントフレーム上にいるかのように実行します。"
	},
	{
		L"watch", &LuaDebugger::watch,
		L"watch [<lua_script> | -d <number>]", L"ウォッチ式の登録",
		L"ブレークする度に自動で評価されるスクリプトを登録します。"
	},
	{
		L"src", &LuaDebugger::src,
		L"src [-f <file>] [<line>] [-n <numlines>] [-all]", L"ソースファイルの表示",
		L"デバッガがロードしているソースファイルの情報を表示します。"
		L"ブレーク可能ポイントや設置済みのブレークポイントも一緒に表示します。"
	},
	{
		L"c", &LuaDebugger::c,
		L"c", L"続行(continue)",
		L"実行を続行します。"
	},
	{
		L"s", &LuaDebugger::s,
		L"s", L"ステップイン(step)",
		L"新たな行に到達するまで実行します。関数呼び出しがあった場合、その中に入ります。"
	},
	{
		L"n", &LuaDebugger::n,
		L"n", L"ステップオーバー(next)",
		L"新たな行に到達するまで実行します。関数呼び出しがあった場合、それが終わるまで実行します。"
	},
	{
		L"out", &LuaDebugger::out,
		L"out", L"ステップアウト",
		L"現在の関数から戻るまで実行します。"
	},
	{
		L"bp", &LuaDebugger::bp,
		L"bp [-d] [-f <filename>] [<line>...]", L"ブレークポイントの設置/削除/表示",
		L"ブレークポイントを設定します。引数を指定しない場合、ブレークポイント一覧を表示します。"
		L"-d を指定すると削除します。"
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
		m_callDepth++;
		break;
	case LUA_HOOKTAILCALL:
		break;
	case LUA_HOOKRET:
		m_callDepth--;
		break;
	case LUA_HOOKLINE:
		if (m_debugState == DebugState::BREAK_LINE_ANY) {
			brk = true;
			break;
		}
		if (m_debugState == DebugState::BREAK_LINE_DEPTH0 && m_callDepth == 0) {
			brk = true;
			break;
		}
		// search breakpoints
		{
			lua_getinfo(L, "Sl", ar);
			// avoid frequent new at every line event
			m_fileNameStr = ar->source;
			auto kv = m_debugInfo.find(m_fileNameStr);
			if (kv != m_debugInfo.end()) {
				const auto &bps = kv->second.breakPoints;
				int ind = ar->currentline - 1;
				if (ind >= 0 && static_cast<size_t>(ind) < bps.size() && bps[ind]) {
					brk = true;
					break;
				}
			}
		}
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
		printWatchList();
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
	printSrcLines(ar->source, ar->currentline, DefSrcLines, ar->currentline);
}

void LuaDebugger::printSrcLines(const std::string &name,
	int line, int range, int execLine)
{
	// find info.chunkName == name
	const auto &kv = m_debugInfo.find(name);
	if (kv != m_debugInfo.cend()) {
		const ChunkDebugInfo &info = kv->second;
		line--;
		execLine--;
		int uh = (range - 1) / 2;
		int dh = range - uh - 1;
		int beg = line - uh;
		beg = (beg < 0) ? 0 : beg;
		int end = line + dh;
		int ssize = static_cast<int>(info.srcLines.size());
		end = (end >= ssize) ? ssize - 1 : end;
		for (int i = beg; i <= end; i++) {
			char execMark = (i == execLine) ? '>' : ' ';
			char bpMark = (info.validLines[i]) ? 'o' : ' ';
			bpMark = (info.breakPoints[i]) ? '*' : bpMark;
			debug::writef("%c%c%6d: %s", execMark, bpMark, i + 1,
				info.srcLines.at(i).substr(0, 69).c_str());
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
				debug::writef_nonl("[%3d] %s = ", n, name);
				for (const auto &val : luaValueToStrList(L, -1, maxDepth, 0)) {
					debug::writeLine(val.c_str());
				}
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
			debug::writef_nonl("[%3d] %s = ", n, name);
			for (const auto &val : luaValueToStrList(L, -1, d, 0)) {
				debug::writeLine(val.c_str());
			}
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

	int ret = lua_getstack(L, frameNo, ar);
	ASSERT(ret);
	ret = lua_getinfo(L, "nSltu", ar);
	ASSERT(ret);

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

void LuaDebugger::printEval(const std::string &expr)
{
	lua_State *L = m_L;
	try {
		// load str and push function
		int ret = luaL_loadstring(L, expr.c_str());
		if (ret != LUA_OK) {
			throw LuaError("load failed", L);
		}

		lua_Debug ar = { 0 };
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
				for (const auto &val : luaValueToStrList(L, i, DefTableDepth)) {
					debug::writeLine(val.c_str());
				}
			}
		}
		lua_settop(L, retBase);
	}
	catch (const LuaError &ex) {
		debug::writeLine(ex.what());
	}
}

void LuaDebugger::printWatchList()
{
	if (m_watchList.size() != 0) {
		debug::writeLine("Watch List:");
	}
	int i = 0;
	for (const auto &expr : m_watchList) {
		debug::writef("[%d]%s", i, expr.c_str());
		std::string src("return");
		src += expr;
		src += ";";
		printEval(src);
		i++;
	}
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

bool LuaDebugger::conf(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(LUA_COPYRIGHT);
	debug::writeLine(LUA_AUTHORS);
	debug::writeLine();
	debug::writeLine(L"lua_Integer");
	debug::writef(L"  0x%llx-0x%llx",
		static_cast<long long>(LUA_MININTEGER),
		static_cast<long long>(LUA_MAXINTEGER));
	debug::writef(L"  (%lld)-(%lld)",
		static_cast<long long>(LUA_MININTEGER),
		static_cast<long long>(LUA_MAXINTEGER));
	debug::writeLine(L"lua_Number");
	debug::writef(L"  abs range : (%.16Le) - (%.16Le)",
		static_cast<long double>(std::numeric_limits<lua_Number>::min()),
		static_cast<long double>(std::numeric_limits<lua_Number>::max()));
	debug::writef(L"  10-base digits : %d",
		std::numeric_limits<lua_Number>::digits10);
	debug::writef(L"  10-base exp range : (%d) - (%d)",
		std::numeric_limits<lua_Number>::min_exponent10,
		std::numeric_limits<lua_Number>::max_exponent10);
	return false;
}

bool LuaDebugger::mem(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	lua_State *L = m_L;

	bool gc = false;
	for (const auto &str : args) {
		if (str == L"-gc") {
			gc = true;
		}
		else {
			debug::writeLine(usage);
			return false;
		}
	}

	if (gc) {
		lua_gc(L, LUA_GCCOLLECT, 0);
	}
	int kb = lua_gc(L, LUA_GCCOUNT, 0);
	int b = kb * 1024 + lua_gc(L, LUA_GCCOUNTB, 0);
	debug::writef(L"%.3f KiB", b / 1024.0);
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
		printSrcLines(ar.source, ar.currentline, DefSrcLines, ar.currentline);
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
	lua_State *L = m_L;

	lua_Debug ar = { 0 };
	if (!lua_getstack(L, m_currentFrame, &ar)) {
		debug::writeLine(L"Cannot get current frame info");
		return false;
	}
	lua_getinfo(L, "Sl", &ar);

	// [-f <file>] [<line>] [-n <numlines>] [-all]
	std::string fileName = ar.source;
	int line = ar.currentline;
	int num = DefSrcLines;
	bool all = false;
	try {
		for (size_t i = 0; i < args.size(); i++) {
			if (args[i] == L"-f") {
				i++;
				fileName = util::wc2utf8(args.at(i).c_str()).get();
			}
			else if (args[i] == L"-n") {
				i++;
				num = stoi_s(args.at(i));
			}
			else if (args[i] == L"-all") {
				all = true;
			}
			else {
				line = stoi_s(args[i]);
			}
		}
	}
	catch (...) {
		debug::writeLine(usage);
		return false;
	}

	num = all ? 0x00ffffff : num;
	printSrcLines(fileName, line, num);
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
	std::wstring wsrc(L"return");
	for (const auto &str : args) {
		wsrc += L' ';
		wsrc += str;
	}
	wsrc += L";";
	auto src = util::wc2utf8(wsrc.c_str());

	printEval(src.get());

	return false;
}

bool LuaDebugger::watch(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	// watch [<lua_script> | -d <number>]
	std::vector<int> delInd;
	std::string addScript;
	try {
		if (args.empty()) {
			// do nothing
		}
		else if (args[0] == L"-d") {
			for (size_t i = 0; i < args.size(); i++) {
				delInd.push_back(stoi_s(args[i]));
			}
		}
		else {
			for (size_t i = 0; i < args.size(); i++) {
				addScript += ' ';
				addScript += util::wc2utf8(args[i].c_str()).get();
			}
		}
	}
	catch (...) {
		debug::writeLine(usage);
		return false;
	}
	// delete
	std::sort(delInd.begin(), delInd.end(), std::greater<int>());
	for (int ind : delInd) {
		if (ind < 0 || static_cast<size_t>(ind) >= m_watchList.size()) {
			debug::writef(L"Cannot delete: %d", ind);
			continue;
		}
		m_watchList.erase(m_watchList.begin() + ind);
	}
	// add
	if (!addScript.empty()) {
		m_watchList.emplace_back(addScript);
	}
	// print
	printWatchList();
	return false;
}

bool LuaDebugger::c(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(L"[LuaDbg] continue...");
	m_debugState = DebugState::CONT;
	return true;
}

bool LuaDebugger::s(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(L"[LuaDbg] step in...");
	m_debugState = DebugState::BREAK_LINE_ANY;
	return true;
}

bool LuaDebugger::n(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(L"[LuaDbg] step over...");
	m_callDepth = 0;
	m_debugState = DebugState::BREAK_LINE_DEPTH0;
	return true;
}

bool LuaDebugger::out(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	debug::writeLine(L"[LuaDbg] step out...");
	m_callDepth = 1;
	m_debugState = DebugState::BREAK_LINE_DEPTH0;
	return true;
}

bool LuaDebugger::bp(const wchar_t *usage, const std::vector<std::wstring> &args)
{
	lua_State *L = m_L;
	std::string fileName;
	std::vector<int> lines;

	// default is current frame source file
	lua_Debug ar = { 0 };
	if (lua_getstack(L, m_currentFrame, &ar)) {
		lua_getinfo(L, "S", &ar);
		fileName = ar.source;
	}
	// bp [-d] [-f <filename>] [<line>...]
	bool del = false;
	try {
		for (size_t i = 0; i < args.size(); i++) {
			if (args[i] == L"-d") {
				del = true;
			}
			else if (args[i] == L"-f") {
				i++;
				fileName = util::wc2utf8(args.at(i).c_str()).get();
			}
			else {
				lines.push_back(stoi_s(args[i], 10));
			}
		}
	}
	catch (...) {
		debug::writeLine(usage);
		return false;
	}
	// set/delete
	for (int line : lines) {
		auto kv = m_debugInfo.find(fileName);
		if (kv == m_debugInfo.cend()) {
			debug::writef("Error: Debug info not found: \"%s\"", fileName.c_str());
			continue;
		}
		ChunkDebugInfo &info = kv->second;
		if (line < 1 || static_cast<size_t>(line) > info.breakPoints.size()) {
			debug::writef("Error: %s:%d is out of range", fileName.c_str(), line);
			continue;
		}
		info.breakPoints[line - 1] = del ? 0 : 1;
	}
	// print
	debug::writeLine(L"Breakpoints:");
	for (const auto &kv : m_debugInfo) {
		const ChunkDebugInfo &info = kv.second;
		debug::writef("[%s]", info.chunkName.c_str());
		bool any = false;
		for (size_t i = 0; i < info.breakPoints.size(); i++) {
			if (info.breakPoints[i]) {
				debug::writef(L"%d", i + 1);
				any = true;
			}
		}
		if (!any) {
			debug::writeLine(L"(No breakpoints)");
		}
	}

	return false;
}

}
}
}
