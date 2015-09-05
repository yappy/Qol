// App.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "App.h"
#include <debug.h>
#include <input.h>
#include <array>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

test::input::DInput *g_di;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	{
		using namespace test::debug;

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

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_APP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_APP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

	{
		using namespace test::debug;

		shutdownDebugOutput();
	}

    return (int) msg.wParam;
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
		g_di = new test::input::DInput(hWnd);
		SetTimer(hWnd, 1, 16, nullptr);
		break;
	}
	case WM_TIMER:
	{
		g_di->processFrame();
		std::array<bool, 256> keys = g_di->getKeys();
		for (auto i = 0U; i < keys.size(); i++) {
			if (keys[i]) {
				test::debug::writef(L"Key 0x%02x", i);
			}
		}
		for (int i = 0; i < g_di->getPadCount(); i++) {
			DIJOYSTATE state;
			g_di->getPadState(&state, i);
			for (int b = 0; b < 32; b++) {
				if (state.rgbButtons[b] & 0x80) {
					test::debug::writef(L"pad[%d].button%d", i, b);
				}
			}
			{
				// left stick
				if (std::abs(state.lX) > test::input::DInput::AXIS_THRESHOLD) {
					test::debug::writef(L"pad[%d].x=%ld", i, state.lX);
				}
				if (std::abs(state.lY) > test::input::DInput::AXIS_THRESHOLD) {
					test::debug::writef(L"pad[%d].y=%ld", i, state.lY);
				}
				// right stick
				if (std::abs(state.lZ) > test::input::DInput::AXIS_THRESHOLD) {
					test::debug::writef(L"pad[%d].z=%ld", i, state.lZ);
				}
				if (std::abs(state.lRz) > test::input::DInput::AXIS_THRESHOLD) {
					test::debug::writef(L"pad[%d].rz=%ld", i, state.lRz);
				}
			}
			for (int b = 0; b < 4; b++) {
				if (state.rgdwPOV[b] != -1) {
					test::debug::writef(L"pad[%d].POV%d=%u", i, b, state.rgdwPOV[b]);
				}
			}
		}
		break;
	}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		delete g_di;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
