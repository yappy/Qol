#pragma once

#include <windows.h>
#include <memory>

namespace yappy {
namespace graphics {

struct hwndDeleter {
	using pointer = HWND;
	void operator()(HWND hwnd)
	{
		::DestroyWindow(hwnd);
	}
};

struct InitParam {
	HINSTANCE hInstance;
	int nCmdShow;
	int w;
	int h;
	const wchar_t *wndClsName;
	const wchar_t *title;
};

class Application {
public:
	Application(const InitParam &param);
	virtual ~Application();
	int run();

protected:

private:
	using HWndPtr = std::unique_ptr<HWND, hwndDeleter>;
	HWndPtr m_hWnd;

	void initializeWindow(const InitParam &param);
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

}
}
