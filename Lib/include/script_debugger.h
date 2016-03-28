#pragma once

#include "util.h"
#include <lua.hpp>

namespace yappy {
namespace lua {
namespace debugger {

struct ChunkDebugInfo {
	// must be unique
	std::string chunkName;
	// source code lines array
	std::vector<std::string> srcLines;
	// each line is valid? (can put breakpoint?)
	std::vector<uint8_t> validLines;
	// breakpoints
	std::vector<uint8_t> breakPoints;

	// cannot copy, move only
	ChunkDebugInfo() = default;
	ChunkDebugInfo(const ChunkDebugInfo &) = delete;
	ChunkDebugInfo &operator=(const ChunkDebugInfo &) = delete;
	ChunkDebugInfo(ChunkDebugInfo &&) = default;
	ChunkDebugInfo &operator=(ChunkDebugInfo &&) = default;
};

class LuaDebugger : private util::noncopyable {
public:
	LuaDebugger(lua_State *L, bool debugEnable);

	lua_State *getLuaState() const;
	void loadDebugInfo(const char *name, const char *src, size_t size);
	void pcall(int narg, int nret, int instLimit);

	bool help(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool bt(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool fr(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool src(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool eval(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool cont(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool si(const wchar_t *usage, const std::vector<std::wstring> &args);
	bool bp(const wchar_t *usage, const std::vector<std::wstring> &args);

private:
	enum class DebugState {
		CONT,			// only bp or error
		INIT_BREAK,		// line event (once)
		STEP_IN,		// line or call event
		STEP_OUT,		// ret event
		STEP_OVER,		// line event, if call-ret count == 0
	};

	static const int DefSrcLines = 21;
	static const int DefTableDepth = 1;

	lua_State *m_L;
	bool m_debugEnable;
	int m_currentFrame = 0;

	DebugState m_debugState = DebugState::CONT;
	std::unordered_map<std::string, ChunkDebugInfo> m_debugInfo;

	void hook(lua_Debug *ar);
	void hookNonDebug(lua_Debug *ar);
	void hookDebug(lua_Debug *ar);

	static int msghandler(lua_State *L);
	static void hookRaw(lua_State *L, lua_Debug *ar);

	void cmdLoop(lua_Debug *ar);
	void summaryOnBreak(lua_Debug *ar);
	void printSrcLines(const char *name, int line, int range);
	void printLocalAndUpvalue(lua_Debug *ar, int maxDepth, bool skipNoName);
	void pushLocalEnv(lua_Debug *ar, int frameNo);
};

}
}
}
