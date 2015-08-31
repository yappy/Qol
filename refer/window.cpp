#include "stdafx.h"

#include "window.hpp"
#include "exceptions.hpp"

namespace {

	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
		switch (msg){
		case WM_SYSCOMMAND:
			// supress monitor off / screen saver
			switch (wParam & 0xfff0){
			case SC_MONITORPOWER:
			case SC_SCREENSAVE:
				return 0;
			}
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}

}

namespace dx9lib {

	Window::Window(const tstring &className, const tstring &title, HICON hIcon, int w, int h) :
		m_exitCode(0)
	{
		HINSTANCE hInst = ::GetModuleHandle(NULL);
		WNDCLASS wc = {0};
		wc.style 		= 0;
		wc.lpfnWndProc	= WndProc;
		wc.cbClsExtra	= wc.cbWndExtra = 0;
		wc.hInstance	= hInst;
		wc.hIcon		= hIcon;
		wc.hCursor		= LoadCursor(NULL , IDC_ARROW);
		wc.hbrBackground= NULL;
		wc.lpszMenuName	= NULL;
		wc.lpszClassName= className.c_str();

		w += ::GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
		h += ::GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + ::GetSystemMetrics(SM_CYCAPTION);

		if (!RegisterClass(&wc))
			Win32Error::throwError(_T("RegisterClass failed"));
		const DWORD STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		m_hwnd = ::CreateWindow(
			className.c_str(), title.c_str(), STYLE,
			CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL,
			hInst, NULL
		);
		if (m_hwnd == NULL)
			Win32Error::throwError(_T("CreateWindow failed"));
	}

	Window::~Window() {}

	void Window::setVisible(bool visible)
	{
		ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
	}

	void Window::setTitle(const tstring &title)
	{
		SetWindowText(m_hwnd, title.c_str());
	}

	HWND Window::getHWND()
	{
		return m_hwnd;
	}

	bool Window::processMessage()
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				m_exitCode = msg.wParam;
				return false;
			}
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		return true;
	}

	WPARAM Window::getExitCode()
	{
		return m_exitCode;
	}

}
