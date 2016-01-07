// App.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include <debug.h>
#include <file.h>
#include <config.h>
#include <script.h>
#include <framework.h>
#include <array>

using namespace yappy;

class TestScene : public framework::SceneBase {
protected:
	void loadOnSubThread(std::atomic_bool &cancel)
	{
		for (int i = 0; i < 3000; i++) {
			if (cancel.load()) {
				debug::writeLine(L"cancel!");
				break;
			}
			debug::writef(L"%d...", i);
			::Sleep(1);
		}
	}
};

class MyApp : public framework::Application {
public:
	MyApp(const framework::AppParam &appParam,
		const graphics::GraphicsParam &graphParam) :
		Application(appParam, graphParam)
	{}
protected:
	void init() override;
	void update() override;
	void render() override;
private:
	TestScene m_testScene;
	lua::Lua m_lua;
	uint64_t m_frameCount = 0;
};

void MyApp::init()
{
	/*
	sound().playBgm(L"../sampledata/Epoq-Lepidoptera.ogg");

	addTextureResource(0, "unyo", L"../sampledata/test_400_300.png");
	addTextureResource(0, "maru", L"../sampledata/circle.png");

	addFontResource(0, "e", L"ＭＳ 明朝", 'A', 'Z', 16, 32);
	addFontResource(0, "j", L"メイリオ", L'あ', L'ん', 128, 128);

	addSeResource(0, "testse", L"/C:/Windows/Media/chimes.wav");

	loadResourceSet(0);
	*/

	//*
	m_lua.loadTraceLib();
	m_lua.loadGraphLib(this);
	m_lua.loadSoundLib(this);

	m_lua.loadFile(L"../sampledata/test.lua", "testfile.lua");

	m_lua.callWithResourceLib("load", this);
	addSeResource(0, "testse", L"/C:/Windows/Media/chimes.wav");
	loadResourceSet(0);

	m_lua.callGlobal("start");
	//*/

	// performance test
	{
		debug::StopWatch(L"Resource ID hash");
		uint32_t dummy = 0;
		for (int i = 0; i < 10000; i++) {
			const auto &r = getTexture(0, "unyo");
			dummy += r->w;
		}
		debug::writef(L"%d", dummy);
	}

	// sub thread start
	m_testScene.startLoadThread();
}

void MyApp::render()
{
	/*
	int test = static_cast<int>(m_frameCount * 5 % 768);

	auto unyo = getTexture(0, "unyo");
	auto maru = getTexture(0, "maru");
	graph().drawTexture(maru, test, test);
	graph().drawTexture(unyo, 1024 / 2, 768 / 2, false, false, 0, 0, -1, -1, 200, 150, m_frameCount / 3.14f / 10);

	auto testfont = getFont(0, "e");
	graph().drawChar(testfont, 'Y', 100, 100);
	graph().drawChar(testfont, 'A', 116, 100);
	graph().drawChar(testfont, 'P', 132, 100);
	graph().drawChar(testfont, 'P', 148, 100);
	graph().drawChar(testfont, 'Y', 164, 100, 0x00ff00, 2, 2, 1.0f);

	auto testjfont = getFont(0, "j");
	graph().drawChar(testjfont, L'ほ', 100, 200);
	graph().drawString(testjfont, L"ほわいと", 100, 600, 0x000000, -32);
	//*/
	if (m_testScene.checkLoadStatus()) {
		m_lua.callGlobal("draw");
	}
}

void MyApp::update()
{
	m_lua.callGlobal("update");
	m_frameCount++;

	auto testse = getSoundEffect(0, "testse");

	std::array<bool, 256> keys = input().getKeys();
	for (size_t i = 0U; i < keys.size(); i++) {
		if (keys[i]) {
			debug::writef(L"Key 0x%02x", i);
			sound().playSoundEffect(testse);
		}
	}
	for (int i = 0; i < input().getPadCount(); i++) {
		DIJOYSTATE state;
		input().getPadState(&state, i);
		for (int b = 0; b < 32; b++) {
			if (state.rgbButtons[b] & 0x80) {
				debug::writef(L"pad[%d].button%d", i, b);
				sound().playSoundEffect(testse);
			}
		}
		{
			// left stick
			if (std::abs(state.lX) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].x=%ld", i, state.lX);
			}
			if (std::abs(state.lY) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].y=%ld", i, state.lY);
			}
			// right stick
			if (std::abs(state.lZ) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].z=%ld", i, state.lZ);
			}
			if (std::abs(state.lRz) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].rz=%ld", i, state.lRz);
			}
		}
		for (int b = 0; b < 4; b++) {
			if (state.rgdwPOV[b] != -1) {
				debug::writef(L"pad[%d].POV%d=%u", i, b, state.rgdwPOV[b]);
			}
		}
	}
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

		MyApp app(appParam, graphParam);
		result = app.run();
	}
	catch (const std::exception &ex) {
		debug::writef(L"Error: %s", util::utf82wc(ex.what()).get());
	}

	debug::shutdownDebugOutput();
	return result;
}
