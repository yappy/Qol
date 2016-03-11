#include "stdafx.h"
#include "App.h"

MainScene::MainScene(MyApp *app) : m_app(app)
{
	/*
	sound().playBgm(L"../sampledata/Epoq-Lepidoptera.ogg");

	addTextureResource(0, "unyo", L"../sampledata/test_400_300.png");
	addTextureResource(0, "maru", L"../sampledata/circle.png");

	addFontResource(0, "e", L"‚l‚r –¾’©", 'A', 'Z', 16, 32);
	addFontResource(0, "j", L"ƒƒCƒŠƒI", L'‚ ', L'‚ñ', 128, 128);

	addSeResource(0, "testse", L"/C:/Windows/Media/chimes.wav");

	loadResourceSet(0);
	*/

	m_app->addSeResource(1, "testse", L"/C:/Windows/Media/chimes.wav");
	m_app->loadResourceSet(1, std::atomic_bool());

	m_lua.loadTraceLib();
	m_lua.loadGraphLib(m_app);
	m_lua.loadSoundLib(m_app);

	m_lua.loadFile(L"../sampledata/test.lua", "testfile.lua");

	m_lua.callWithResourceLib("load", m_app);
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
	if (m_loading) {
		updateLoadStatus();
		if (isLoadCompleted()) {
			m_loading = false;
			m_lua.callGlobal("start");
		}
		else {
			return;
		}
	}
	m_lua.callGlobal("update");
}

void MainScene::render()
{
	if (!isLoadCompleted()) {
		// Loading screen
		return;
	}
	m_lua.callGlobal("draw");
}
