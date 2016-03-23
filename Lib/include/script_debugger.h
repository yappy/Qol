#pragma once

#include "util.h"
#include <lua.hpp>

namespace yappy {
namespace lua {
namespace debugger {

// TODO
struct ChunkDebugInfo {
	// source code lines array
	std::vector<std::string> srcLines;
	// each line is valid? (can put breakpoint?)
	std::vector<uint8_t> validLines;
};

class LuaDebugger : private util::noncopyable {
public:
	LuaDebugger(lua_State *L, bool debugEnable);

	lua_State *getLuaState() const;
	// TODO
	void loadDebugInfo(const char *name, const char *src, size_t size);
	void pcall(int narg, int nret, int instLimit);

private:
	lua_State *m_L;
	bool m_debugEnable;
	std::unordered_map<std::string, ChunkDebugInfo> m_debugInfo;

	void hook(lua_Debug *ar);
	void hookNonDebug(lua_Debug *ar);
	void hookDebug(lua_Debug *ar);

	static int msghandler(lua_State *L);
	static void hookRaw(lua_State *L, lua_Debug *ar);
};

}
}
}
