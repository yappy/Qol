#pragma once

#include "util.h"
#include "framework.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "script_debugger.h"
#include "script_export.h"

namespace yappy {
/// Lua scripting library.
namespace lua {

class LuaError : public std::runtime_error {
public:
	LuaError(const std::string &msg, lua_State *L);
	const char *what() const override
	{ return m_what.c_str(); }
private:
	std::string m_what;
};

/** @brief Lua state manager.
 * @details Each Lua object manages one lua_State.
 * If debugEnable at the constructor is true, full debug hook will be enabled.
 * This may be heavy.
 * If debugEnable is false, debug hook mask is only instruction count.
 * instLimit is a feature for preventing infinite loop.
 * LuaDebugger set it as hook count, and if COUNT event would happened,
 * hook function raises an lua error.
 */
class Lua : private util::noncopyable {
public:
	/** @brief Create new lua_State and open standard libs.
	 * @param[in]	debugEnable		Enable debug feature.
	 * @param[in]	maxHeapSize		Max memory usage.
	 *								(only virtual address range will be reserved at first)
	 * @param[in]	initHeapSize	Initial commit size. (physical memory mapped)
	 * @param[in]	instLimit		Lua bytecode instruction count limit. (no limit if 0)
	 */
	Lua(bool debugEnable, size_t maxHeapSize, size_t initHeapSize = 1024 * 1024,
		int instLimit = 0x0fffffff);
	/** @brief Destruct lua_State.
	 */
	~Lua();

	/** @brief Returns lua_State which this object has.
	 * @return lua_State
	 */
	lua_State *getLuaState() const;

	void loadTraceLib();
	void loadSysLib();
	void loadResourceLib(framework::Application *app);
	void loadGraphLib(framework::Application *app);
	void loadSoundLib(framework::Application *app);

	/** @brief Load script file and eval it.
	 * @param[in] fileName	Script file name.
	 * @param[in] autoBreak	Debug break at first line.
	 * @param[in] prot		Use pcall() if true. (false is include from Lua only)
	 */
	void loadFile(const wchar_t *fileName, bool autoBreak, bool prot = true);

	struct doNothing {
		void operator ()(lua_State *L) {}
	};

	/** @brief Calls global function.
	 * @details pushParamFunc and getRetFunc must be able to be called by:
	 * @code
	 * pushParamFunc(lua_State *);
	 * getRetFunc(lua_State *);
	 * @endcode
	 * (lambda expr is recommended)
	 * @code
	 * [](lua_State *L) {
	 *     lua_pushXXX(L, ...);
	 * }
	 * @endcode
	 *
	 * @param[in] funcName		Function name.
	 * @param[in] autoBreak		Break by debugger at the first LINE event if true.
	 * @param[in] pushArgFunc	Will be called just before lua_pcall().
	 * @param[in] narg			Args count.
	 * @param[in] getRetFunc	Will be called just after lua_pcall().
	 * @param[in] nret			Return values count.
	 */
	template <class ParamFunc = doNothing, class RetFunc = doNothing>
	void callGlobal(const char *funcName, bool autoBreak,
		ParamFunc pushArgFunc = doNothing(), int narg = 0,
		RetFunc getRetFunc = doNothing(), int nret = 0)
	{
		lua_State *L = m_lua.get();
		lua_getglobal(L, funcName);
		// push args
		pushArgFunc(L);
		// pcall
		pcallInternal(narg, nret, autoBreak);
		// get results
		getRetFunc(L);
		// clear stack
		lua_settop(L, 0);
	}

private:
	static const DWORD HeapOption = HEAP_NO_SERIALIZE;

	struct LuaDeleter {
		void operator()(lua_State *L);
	};

	bool m_debugEnable;
	util::HeapPtr m_heap;
	std::unique_ptr<lua_State, LuaDeleter> m_lua;
	std::unique_ptr<debugger::LuaDebugger> m_dbg;

	// custom allocator
	static void *luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize);

	void pcallInternal(int narg, int nret, bool autoBreak);
};

std::vector<std::string> luaValueToStrList(lua_State *L, int ind,
	int maxDepth = 0, int depth = 0);

}
}
