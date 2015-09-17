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

class Application {
public:
	Application();
	virtual ~Application();
	int run();

protected:

private:
	using HWndPtr = std::unique_ptr<HWND, hwndDeleter>;
	HWndPtr m_hwnd;
};

}
}
