#pragma once

#include "util.hpp"
#include <string>

namespace dx9lib {

	class Window : private Uncopyable {
	private:
		HWND m_hwnd;
		WPARAM m_exitCode;

	public:
		Window(const tstring &className, const tstring &title, HICON hIcon, int w, int h);
		~Window();
		void setTitle(const tstring &title);
		void setVisible(bool visible);
		HWND getHWND();
		// Process window message and return false if WM_QUIT
		bool processMessage();
		// wParam of WM_QUIT
		WPARAM getExitCode();
	};

}
