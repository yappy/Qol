#pragma once

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
	int w = 640;
	int h = 480;
	const wchar_t *wndClsName = L"GameWndCls";
	const wchar_t *title = L"GameApp";
	uint32_t refreshRateNumer = 60;
	uint32_t refreshRateDenom = 1;
	bool windowed = true;
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

	ID3D11Device *m_pD3dDevice = nullptr;
	ID3D11DeviceContext *m_pImmediateContext = nullptr;
	IDXGISwapChain *m_pSwapChain = nullptr;
	ID3D11RenderTargetView *m_pRenderTargetView = nullptr;

	void initializeD3d(const InitParam &param);
};

}
}
