#pragma once

#include "util.h"
#include <lua.h>

namespace yappy {
namespace lua {
/// Lua script debugger
namespace debugger {

class LuaDebugger : private util::noncopyable {
public:
	LuaDebugger(lua_State *L, bool debugEnable, int instLimit);

	lua_State *getLuaState() const;
	void loadDebugInfo(const char *name, const char *src, size_t size);
	void pcall(int narg, int nret, bool autoBreak);

	bool help(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool conf(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool mem(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool bt(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool fr(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool src(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool eval(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool watch(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool c(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool s(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool n(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool out(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool bp(const wchar_t *usage, const std::vector<std::wstring> &args);

private:
	struct ChunkDebugInfo {
		// must be unique
		std::string chunkName;
		// source code lines array
		std::vector<std::string> srcLines;
		// each line is valid? (can put breakpoint?)
		std::vector<uint8_t> validLines;
		// breakpoints
		std::vector<uint8_t> breakPoints;

		// Cannot copy, move only
		ChunkDebugInfo() = default;
		ChunkDebugInfo(const ChunkDebugInfo &) = delete;
		ChunkDebugInfo &operator=(const ChunkDebugInfo &) = delete;
		ChunkDebugInfo(ChunkDebugInfo &&) = default;
		ChunkDebugInfo &operator=(ChunkDebugInfo &&) = default;
	};

	enum class DebugState {
		CONT,				// only bp or error
		BREAK_LINE_ANY,		// line event any
		BREAK_LINE_DEPTH0,	// line event, if call-ret depth == 0
	};

	static const int DefSrcLines = 21;
	static const int DefTableDepth = 3;

	lua_State *m_L;
	bool m_debugEnable;
	int m_instLimit;

	std::unordered_map<std::string, ChunkDebugInfo> m_debugInfo;
	std::string m_fileNameStr;
	DebugState m_debugState = DebugState::CONT;
	int m_callDepth = 0;
	int m_currentFrame = 0;
	std::vector<std::string> m_watchList;

	void hook(lua_Debug *ar);
	void hookNonDebug(lua_Debug *ar);
	void hookDebug(lua_Debug *ar);

	static int msghandler(lua_State *L);
	static void hookRaw(lua_State *L, lua_Debug *ar);

	void cmdLoop();
	void summaryOnBreak(lua_Debug *ar);
	void printSrcLines(const std::string &name, int line, int range, int execLine = -1);
	void printLocalAndUpvalue(lua_Debug *ar, int maxDepth, bool skipNoName);
	void pushLocalEnv(lua_Debug *ar, int frameNo);
	void printEval(const std::string &expr);
	void printWatchList();
};

}
}
}
