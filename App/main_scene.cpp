#include "stdafx.h"
#include "App.h"

using framework::keyPressedAsync;

MainScene::MainScene(MyApp *app, bool debugEnable) :
	m_app(app),
	m_lua(debugEnable, LuaHeapSize)
{
	m_lua.loadTraceLib();
	m_lua.loadSysLib();
	m_lua.loadRandLib();
	m_lua.loadResourceLib(m_app);
	m_lua.loadGraphLib(m_app);
	m_lua.loadSoundLib(m_app);

	bool dbg = keyPressedAsync(VK_F12);
	m_lua.loadFile(L"../sampledata/test.lua", dbg);

	{
		framework::UnsealResource autoSeal(*m_app);
		m_lua.callGlobal("load", dbg);
	}
}

void MainScene::setup()
{
	m_loading = true;
	startLoadThread();
}

void MainScene::loadOnSubThread(std::atomic_bool &cancel)
{
	m_app->loadResourceSet(0, cancel);
	// dummy 1000ms load time
	for (int i = 0; i < 1000; i++) {
		Sleep(1);
	}
	debug::writeLine(L"sub thread complete!");
}

void MainScene::update()
{
	bool dbg = keyPressedAsync(VK_F12);

	if (m_loading) {
		updateLoadStatus();
		if (isLoadCompleted()) {
			m_loading = false;
			m_lua.callGlobal("start", dbg);
		}
		else {
			return;
		}
	}

	auto keys = m_app->input().getKeys();

	m_lua.callGlobal("update", dbg,
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

	if (keys[DIK_SPACE]) {
		// SPACE to sub scene
		m_lua.callGlobal("exit", dbg);
		auto *next = m_app->getSceneAs<SubScene>(SceneId::Sub);
		next->setup();
		next->update();
		m_app->setScene(SceneId::Sub);
	}
}

void MainScene::render()
{
	if (!isLoadCompleted()) {
		// Loading screen
		return;
	}
	bool dbg = keyPressedAsync(VK_F12);
	m_lua.callGlobal("draw", dbg);
}
