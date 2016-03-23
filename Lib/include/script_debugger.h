#pragma once

#include "util.h"
#include <lua.hpp>

namespace yappy {
namespace lua {
namespace debugger {

class LuaDebugger : private util::noncopyable {
public:
	explicit LuaDebugger(lua_State *L);

	lua_State *getLuaState() const;
	void pcall(int narg, int nret, int instLimit, bool debug);

private:
	lua_State *m_lua;

	static int msghandler(lua_State *L);
	static void hook(lua_State *L, lua_Debug *ar);
	static void hookNonDebug(lua_State *L, lua_Debug *ar);
};

}
}
}
