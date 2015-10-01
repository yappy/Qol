// App.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include <debug.h>
#include <file.h>
#include <graphics.h>
#include <input.h>
#include <array>


using yappy::graphics::Application;
using yappy::input::DInput;
using namespace yappy;


class MyApp : public Application {
public:
	MyApp(const InitParam &param) :
		Application(param),
		m_di(param.hInstance, getHWnd())
	{}
protected:
	void init() override;
	void update() override;
	void render() override;
private:
	DInput m_di;
};


void MyApp::init()
{
	loadTexture("notpow2", L"../sampledata/test_400_300.png");
	loadTexture("testtex", L"../sampledata/circle.png");
}

void MyApp::render()
{
	static int test = 0;
	test += 5;
	test %= 768;
	drawTexture("testtex", test, test);
}

void MyApp::update()
{
	m_di.processFrame();
	std::array<bool, 256> keys = m_di.getKeys();
	for (size_t i = 0U; i < keys.size(); i++) {
		if (keys[i]) {
			debug::writef(L"Key 0x%02x", i);
		}
	}
	for (int i = 0; i < m_di.getPadCount(); i++) {
		DIJOYSTATE state;
		m_di.getPadState(&state, i);
		for (int b = 0; b < 32; b++) {
			if (state.rgbButtons[b] & 0x80) {
				debug::writef(L"pad[%d].button%d", i, b);
			}
		}
		{
			// left stick
			if (std::abs(state.lX) > DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].x=%ld", i, state.lX);
			}
			if (std::abs(state.lY) > DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].y=%ld", i, state.lY);
			}
			// right stick
			if (std::abs(state.lZ) > DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].z=%ld", i, state.lZ);
			}
			if (std::abs(state.lRz) > DInput::AXIS_THRESHOLD) {
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
	// debug test
	{
		debug::enableDebugOutput();
#ifdef _DEBUG
		debug::enableConsoleOutput();
		debug::enableFileOutput(L"log.txt");
#endif

		debug::writeLine(L"Start application");
		debug::writeLine(L"�ɂق�");
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

	int result = 0;
	try {
		file::initWithFileSystem(L".");

		Application::InitParam param;
		param.hInstance = hInstance;
		param.w = 1024;
		param.h = 768;
		param.wndClsName = L"TestAppClass";
		param.title = L"Test App";
		param.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
		param.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
		param.nCmdShow = nCmdShow;

		MyApp app(param);
		result = app.run();
	}
	catch (const std::exception &ex) {
		debug::writef(L"Error: %s", ex.what());
	}

	debug::shutdownDebugOutput();
	return result;
}
