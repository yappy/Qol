// App.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include <debug.h>
#include <graphics.h>
#include <input.h>
#include <array>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

yappy::input::DInput *g_di;

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	{
		using namespace yappy::debug;

		enableDebugOutput();
		enableConsoleOutput();
		enableFileOutput(L"log.txt");

		writeLine(L"Start application");
		writeLine(L"‚É‚Ù‚ñ‚²");
		writef(L"%d 0x%08x %f %.8f", 123, 0x1234abcd, 3.14, 3.14);

		const wchar_t *digit = L"0123456789abcdef";
		wchar_t large[1024 + 32] = { 0 };
		for (int i = 0; i < 1024; i++) {
			large[i] = digit[i & 0xf];
		}
		writef(large);
	}

	int result = 0;
	{
		using namespace yappy::graphics;

		InitParam param;
		param.hInstance = hInstance;
		param.w = 1024;
		param.h = 768;
		param.wndClsName = L"TestAppClass";
		param.title = L"Test App";
		param.nCmdShow = SW_MINIMIZE;//nCmdShow;

		Application app(param);
		result = app.run();
	}

	{
		using namespace yappy::debug;

		shutdownDebugOutput();
	}

	return result;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_APP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{
		g_di = new yappy::input::DInput(hWnd);
		SetTimer(hWnd, 1, 16, nullptr);
		break;
	}
	case WM_TIMER:
	{
		g_di->processFrame();
		std::array<bool, 256> keys = g_di->getKeys();
		for (auto i = 0U; i < keys.size(); i++) {
			if (keys[i]) {
				yappy::debug::writef(L"Key 0x%02x", i);
			}
		}
		for (int i = 0; i < g_di->getPadCount(); i++) {
			DIJOYSTATE state;
			g_di->getPadState(&state, i);
			for (int b = 0; b < 32; b++) {
				if (state.rgbButtons[b] & 0x80) {
					yappy::debug::writef(L"pad[%d].button%d", i, b);
				}
			}
			{
				// left stick
				if (std::abs(state.lX) > yappy::input::DInput::AXIS_THRESHOLD) {
					yappy::debug::writef(L"pad[%d].x=%ld", i, state.lX);
				}
				if (std::abs(state.lY) > yappy::input::DInput::AXIS_THRESHOLD) {
					yappy::debug::writef(L"pad[%d].y=%ld", i, state.lY);
				}
				// right stick
				if (std::abs(state.lZ) > yappy::input::DInput::AXIS_THRESHOLD) {
					yappy::debug::writef(L"pad[%d].z=%ld", i, state.lZ);
				}
				if (std::abs(state.lRz) > yappy::input::DInput::AXIS_THRESHOLD) {
					yappy::debug::writef(L"pad[%d].rz=%ld", i, state.lRz);
				}
			}
			for (int b = 0; b < 4; b++) {
				if (state.rgdwPOV[b] != -1) {
					yappy::debug::writef(L"pad[%d].POV%d=%u", i, b, state.rgdwPOV[b]);
				}
			}
		}
		break;
	}
    case WM_DESTROY:
		delete g_di;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
