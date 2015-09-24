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

class FrameControl : private util::noncopyable {
public:
	FrameControl(int64_t fpsNumer, int64_t fpsDenom);
	~FrameControl() = default;
	bool shouldSkipFrame();
	void endFrame();
	double getFramePerSec();
	int getSkipPerSec();

private:
	const double Scale = 0.95;
	const uint32_t MaxSkipCount = 5;

	int64_t m_freq;
	int64_t m_countPerFrame;
	int64_t m_base;
	uint32_t m_skipCount;

	double m_fps = 0.0;
	int m_sps = 0;
	int m_fpsPeriod;
	int m_fpsCount = 0;
	int64_t m_fpsBase = 0;
	int m_fpsFrameAcc = 0;
	int m_fpsSkipAcc = 0;
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
	const UINT_PTR TIMER_EVENT_ID = 0xffff0001;
	const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_SWAP_CHAIN_FLAG SwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	const float ClearColor[4] = { 0.9f, 0.9f, 0.9f, 1.0f };

	using HWndPtr = std::unique_ptr<HWND, hwndDeleter>;
	HWndPtr m_hWnd;

	InitParam m_initParam;
	FrameControl m_frameCtrl;
	util::IUnknownPtr<ID3D11Device>				m_pDevice;
	util::IUnknownPtr<ID3D11DeviceContext>		m_pContext;
	util::IUnknownPtr<IDXGISwapChain>			m_pSwapChain;
	util::IUnknownPtr<ID3D11RenderTargetView>	m_pRenderTargetView;

	void initializeWindow(const InitParam &param);
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void onIdle();
	void updateInternal();
	void renderInternal();

	void initializeD3D(const InitParam &param);
	void initBackBuffer();
};

}
}
