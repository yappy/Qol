#include "stdafx.h"
#include "App.h"

using framework::keyPressedAsync;

const wchar_t *const LuaSrcFile = L"../sampledata/test.lua";

MainScene::MainScene(MyApp *app, bool luaDebug) :
	m_app(app), m_luaDebug(luaDebug)
{
	initializeLua();

	// === If LuaError occured, throw it and exit application ===
	// compile and exec chunk
	bool dbg = keyPressedAsync(VK_F12);
	m_lua->loadFile(LuaSrcFile, dbg);
	// exec load()
	{
		framework::UnsealResource autoSeal(*m_app);
		bool dbg = keyPressedAsync(VK_F12);
		m_lua->callGlobal("load", dbg);
	}
}

void MainScene::initializeLua()
{
	// destruct if m_lua != nullptr
	m_lua.reset(new lua::Lua(m_luaDebug, LuaHeapSize));

	m_lua->loadTraceLib();
	m_lua->loadSysLib();
	m_lua->loadRandLib();
	m_lua->loadResourceLib(m_app);
	m_lua->loadGraphLib(m_app);
	m_lua->loadSoundLib(m_app);
}

void MainScene::reloadLua()
{
	initializeLua();

	bool dbg = keyPressedAsync(VK_F12);
	try {
		m_lua->loadFile(LuaSrcFile, dbg);
	}
	catch(lua::LuaError &err) {
		luaErrorHandler(err, L"compile and exec chunk");
	}
}

void MainScene::luaErrorHandler(const lua::LuaError &err, const wchar_t *msg)
{
	// go to lua error state (m_lua == nullptr)
	m_lua.reset();

	debug::writeLine(err.what());
	debug::writef(L"Lua error: %s", msg);
	debug::writeLine(L"Press F5 to reload");
}

void MainScene::setup()
{
	m_luaStartCalled = false;
	startLoadThread();
}

void MainScene::loadOnSubThread(std::atomic_bool &cancel)
{
	// dummy 1000ms load time
	for (int i = 0; i < 1000; i++) {
		if (cancel.load()) {
			debug::writeLine("cancel");
			break;
		}
		Sleep(1);
	}
	m_app->loadResourceSet(ResSetId::Main, cancel);
	debug::writeLine(L"sub thread complete!");
}

void MainScene::updateOnMainThread()
{
	if (isLoading()) {
		return;
	}

	bool reload = keyPressedAsync(VK_F5);
	bool dbg = keyPressedAsync(VK_F12);
	for (int num = 0; num <= 9; num++) {
		if (keyPressedAsync('0' + num)) {
			m_speed = num;
		}
	}

	if (reload) {
		m_luaStartCalled = false;
		m_app->sound().stopAllSoundEffect();
		m_app->sound().stopBgm();
		reloadLua();
	}
	if (m_lua == nullptr) {
		// Lua error state, do nothing
		return;
	}

	// exec start() (Only once)
	if (!m_luaStartCalled) {
		try {
			m_lua->callGlobal("start", dbg);
		}
		catch (lua::LuaError &err) {
			luaErrorHandler(err, L"exec start()");
			return;
		}
		m_luaStartCalled = true;
	}

	auto keys = m_app->input().getKeys();
	try {
		for (int i = 0; i < m_speed; i++) {
			m_lua->callGlobal("update", dbg,
				[&keys](lua_State *L) {
				// arg1: key input table str->bool
				const int Count = static_cast<int>(keys.size());
				lua_createtable(L, 0, Count);
				for (int i = 0; i < Count; i++) {
					// key = dir2str(i), value = keys[i]
					lua_pushstring(L, input::dikToString(i));
					lua_pushboolean(L, keys[i]);
					lua_settable(L, -3);
				}
			}, 1);
		}
	}
	catch(lua::LuaError &err) {
		luaErrorHandler(err, L"exec update()");
		return;
	}

	if (keys[DIK_SPACE]) {
		// SPACE to sub scene
		try {
			m_lua->callGlobal("exit", dbg);
		}
		catch (lua::LuaError &err) {
			luaErrorHandler(err, L"exec update()");
			return;
		}
		auto *next = m_app->getSceneAs<SubScene>(SceneId::Sub);
		next->setup();
		next->update();
		m_app->setScene(SceneId::Sub);
	}
}

void MainScene::render()
{
	if (isLoading()) {
		// Loading screen
		return;
	}
	if (m_lua == nullptr) {
		// Lua error state, do nothing
		return;
	}

	bool dbg = keyPressedAsync(VK_F12);
	try {
		m_lua->callGlobal("draw", dbg);
	}
	catch(lua::LuaError &err) {
		luaErrorHandler(err, L"exec draw()");
	}
}
