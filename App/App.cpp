﻿// App.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include "resource.h"
#include <debug.h>
#include <file.h>
#include <config.h>
#include <array>

MyApp::MyApp(const framework::AppParam &appParam,
	const graphics::GraphicsParam &graphParam)
	: Application(appParam, graphParam, ResSetId::Count)
{}

std::unique_ptr<framework::SceneBase> &MyApp::getScene(SceneId id)
{
	return m_scenes[static_cast<uint32_t>(id)];
}

void MyApp::setScene(SceneId id)
{
	m_pCurrentScene = getScene(id).get();
}

void MyApp::init()
{
	// load common resource
	addFontResource(ResSetId::Common, "e", L"ＭＳ 明朝", 0x00, 0xff, 16, 32);
	addFontResource(ResSetId::Common, "j", L"メイリオ", L'あ', L'ん', 128, 128);
	addSeResource(ResSetId::Common, "testse", L"/C:/Windows/Media/chord.wav");
	addBgmResource(ResSetId::Common, "testbgm", L"../sampledata/Epoq-Lepidoptera.ogg");
	loadResourceSet(ResSetId::Common, std::atomic_bool());

	m_scenes[static_cast<uint32_t>(SceneId::Main)] = std::make_unique<MainScene>(this);
	m_scenes[static_cast<uint32_t>(SceneId::Sub)] = std::make_unique<SubScene>(this);

	// set initial scene
	auto mainScene = getSceneAs<MainScene>(SceneId::Main);
	mainScene->setup();
	setScene(SceneId::Main);

	// TODO: tmp test
	sound().playBgm(getBgm(ResSetId::Common, "testbgm"));
}

void MyApp::update()
{
	m_pCurrentScene->update();
}

void MyApp::render()
{
	m_pCurrentScene->render();
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// debug test
	{
		debug::enableDebugOutput();
		debug::enableConsoleOutput();
		debug::enableFileOutput(L"log.txt");

		debug::writeLine(L"Start application");
		debug::writeLine(L"にほんご");
		debug::writef(L"%d 0x%08x %f %.8f", 123, 0x1234abcd, 3.14, 3.14);

		const wchar_t *digit = L"0123456789abcdef";
		wchar_t large[1024 + 32] = { 0 };
		for (int i = 0; i < 1024; i++) {
			large[i] = digit[i & 0xf];
		}
		debug::writef(large);

		wchar_t dir[MAX_PATH];
		::GetCurrentDirectory(MAX_PATH, dir);
		debug::writef(L"Current dir: %s", dir);
	}
	// fixed length string key test
	{
		using util::IdString;
		IdString key1, key2, key3;
		util::createFixedString(&key1, "alice");
		util::createFixedString(&key2, "sinki");
		util::createFixedString(&key3, "takenoko");
		std::unordered_map<IdString, int> hash;
		hash.emplace(key1, 1);
		hash.emplace(key2, 2);
		hash.emplace(key3, 3);
		hash.insert({key1, 4});
		for (auto entry : hash) {
			debug::writef("%s=%d", entry.first.data(), entry.second);
		}
	}

	int result = 0;
	try {
		struct {
			int skip;
			bool cursor;
			bool fullscreen;
		} config;
		{
			config::ConfigFile cf(L"config.txt", {
				{ "graphics.skip", "0" },
				{ "graphics.cursor", "true" },
				{ "graphics.fullscreen", "false" }
			});
			config.skip       = cf.getInt("graphics.skip");
			config.cursor     = cf.getBool("graphics.cursor");
			config.fullscreen = cf.getBool("graphics.fullscreen");
		}

		file::initWithFileSystem(L".");

		framework::AppParam appParam;
		graphics::GraphicsParam graphParam;
		appParam.hInstance = hInstance;
		appParam.wndClsName = L"TestAppClass";
		appParam.title = L"Test App";
		appParam.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
		appParam.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
		appParam.nCmdShow = nCmdShow;
		appParam.frameSkip = config.skip;
		appParam.showCursor = config.cursor;
		graphParam.w = 1024;
		graphParam.h = 768;
		graphParam.fullScreen = config.fullscreen;

		auto app = std::make_unique<MyApp>(appParam, graphParam);
		result = app->run();
		// destruct app
	}
	catch (const std::exception &ex) {
		debug::writef(L"Error: %s", util::utf82wc(ex.what()).get());
	}

	debug::shutdownDebugOutput();
	return result;
}
