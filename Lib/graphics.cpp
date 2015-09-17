#include "stdafx.h"
#include "include/graphics.h"

namespace yappy {
namespace graphics {

Application::Application()
{
	// Register class
	WNDCLASSEX cls = { 0 };
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = 0;
	cls.lpfnWndProc = ::DefWindowProc;	//TODO
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = ::GetModuleHandle(nullptr);
	cls.hIcon = nullptr;	//TODO
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);
	cls.lpszMenuName = nullptr;
	cls.lpszClassName = L"TutorialWindowClass";	//TODO
	cls.hIconSm = nullptr;	//TODO
	if (!::RegisterClassEx(&cls)) {
		// TODO throw
	}

	//TODO
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	/*
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 1: Direct3D 11 Basics", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);
	*/
}

Application::~Application() {}

int Application::run()
{
	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else {
			// TODO
		}
	}
	return static_cast<int>(msg.wParam);
}

}
}
