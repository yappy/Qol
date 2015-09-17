#include "stdafx.h"
#include "include/graphics.h"
#include "include/exceptions.h"

namespace yappy {
namespace graphics {

using error::checkWin32Result;

Application::Application(const InitParam &param)
{
	initializeWindow(param);
}

void Application::initializeWindow(const InitParam &param)
{
	// Register class
	WNDCLASSEX cls = { 0 };
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = 0;
	cls.lpfnWndProc = wndProc;
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = param.hInstance;
	cls.hIcon = nullptr;	//TODO
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);
	cls.lpszMenuName = nullptr;
	cls.lpszClassName = param.wndClsName;
	cls.hIconSm = nullptr;	//TODO
	checkWin32Result(::RegisterClassEx(&cls) != 0, "RegisterClassEx() failed");

	const DWORD wndStyle = WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX;

	RECT rc = { 0, 0, param.w, param.h };
	checkWin32Result(::AdjustWindowRect(&rc, wndStyle, FALSE) != FALSE,
		"AdjustWindowRect() failed");

	HWND hWnd = ::CreateWindow(param.wndClsName, param.title, wndStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr, param.hInstance, nullptr);
	checkWin32Result(hWnd != nullptr, "CreateWindow() failed");
	m_hWnd.reset(hWnd);

	::ShowWindow(m_hWnd.get(), param.nCmdShow);
}

Application::~Application() {}

LRESULT CALLBACK Application::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

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
