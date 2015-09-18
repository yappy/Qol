#pragma once

#include "util.h"
#include <windows.h>
#pragma warning(disable: 4005)
#include <d3d11.h>
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
	HINSTANCE hInstance = nullptr;
	int nCmdShow = SW_SHOW;
	int w = 1024;
	int h = 768;
	const wchar_t *wndClsName = L"GameWndCls";
	const wchar_t *title = L"GameApp";
	uint32_t refreshRateNumer = 60;
	uint32_t refreshRateDenom = 1;
	bool fullScreen = false;
};

class Application : private util::noncopyable {
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

	util::IUnknownPtr<ID3D11Device>				m_pDevice;
	util::IUnknownPtr<ID3D11DeviceContext>		m_pContext;
	util::IUnknownPtr<IDXGISwapChain>			m_pSwapChain;
	util::IUnknownPtr<ID3D11RenderTargetView>	m_pRenderTargetView;

	void initializeD3D(const InitParam &param);
};

}
}
