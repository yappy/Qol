#include "stdafx.h"
#include "include/graphics.h"
#include "include/debug.h"
#include "include/exceptions.h"
#include <array>

#pragma comment(lib, "d3d11.lib")

namespace yappy {
namespace graphics {

using error::checkWin32Result;
using error::D3DError;
using error::checkDXResult;

Application::Application(const InitParam &param) :
	m_pDevice(nullptr, util::iunknownDeleter),
	m_pContext(nullptr, util::iunknownDeleter),
	m_pSwapChain(nullptr, util::iunknownDeleter),
	m_pRenderTargetView(nullptr, util::iunknownDeleter)
{
	initializeWindow(param);
	initializeD3D(param);
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

void Application::initializeD3D(const InitParam &param)
{
	HRESULT hr = S_OK;

	// Create device etc.
	{
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		std::array<D3D_DRIVER_TYPE, 2> driverTypes{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP
		};
		std::array<D3D_FEATURE_LEVEL, 3> featureLevels{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};

		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
		sd.BufferDesc.Width = param.w;
		sd.BufferDesc.Height = param.h;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = param.refreshRateNumer;
		sd.BufferDesc.RefreshRate.Denominator = param.refreshRateDenom;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = m_hWnd.get();
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		ID3D11Device *ptmpDevice = nullptr;
		ID3D11DeviceContext *ptmpContext = nullptr;
		IDXGISwapChain *ptmpSwapChain = nullptr;
		D3D_FEATURE_LEVEL featureLevel = static_cast<D3D_FEATURE_LEVEL>(0);

		for (auto driverType : driverTypes) {
			hr = ::D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr,
				createDeviceFlags, &featureLevels[0], static_cast<UINT>(featureLevels.size()),
				D3D11_SDK_VERSION, &sd,
				&ptmpSwapChain, &ptmpDevice, &featureLevel, &ptmpContext);
			if (SUCCEEDED(hr))
				break;
		}
		checkDXResult<D3DError>(hr, "D3D11CreateDeviceAndSwapChain() failed");
		m_pDevice.reset(ptmpDevice);
		m_pContext.reset(ptmpContext);
		m_pSwapChain.reset(ptmpSwapChain);
	}

	// Create a render target view
	{
		ID3D11Texture2D *ptmpBackBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&ptmpBackBuffer);
		checkDXResult<D3DError>(hr, "IDXGISwapChain::GetBuffer() failed");
		// Release() at scope end
		util::IUnknownPtr<ID3D11Texture2D> pBackBuffer(ptmpBackBuffer, util::iunknownDeleter);

		ID3D11RenderTargetView *ptmpRenderTargetView = nullptr;
		hr = m_pDevice->CreateRenderTargetView(
			pBackBuffer.get(), nullptr, &ptmpRenderTargetView);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateRenderTargetView() failed");
		m_pContext->OMSetRenderTargets(1, &ptmpRenderTargetView, nullptr);
		m_pRenderTargetView.reset(ptmpRenderTargetView);
	}

	// Setup the viewport
	{
		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = static_cast<float>(param.w);
		vp.Height = static_cast<float>(param.h);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_pContext->RSSetViewports(1, &vp);
	}

	// fullscreen initially
	/*
	DXGI_ERROR_NOT_CURRENTLY_AVAILABLE (quote from dx11 document)
	For instance:
	- The application is running over Terminal Server.
	- The output window is occluded.
	- The output window does not have keyboard focus.
	- Another application is already in full-screen mode.
	*/
	if (param.fullScreen) {
		hr = m_pSwapChain->SetFullscreenState(TRUE, nullptr);
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
			debug::writeLine(L"Fullscreen mode is currently unavailable");
			debug::writeLine(L"Launch on window mode...");
		}
		else {
			checkDXResult<D3DError>(hr, "IDXGISwapChain::SetFullscreenState() failed");
		}
	}
}

Application::~Application()
{
	if (m_pContext != nullptr) {
		m_pContext->ClearState();
	}
	if (m_pSwapChain != nullptr) {
		m_pSwapChain->SetFullscreenState(FALSE, nullptr);
	}
}

LRESULT CALLBACK Application::wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_SIZE:
		debug::writeLine(L"WM_SIZE");
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
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
			debug::StopWatch test(L"Present()");
			float color[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
			m_pContext->ClearRenderTargetView(m_pRenderTargetView.get(), color);
			m_pSwapChain->Present(1, 0);
		}
	}
	return static_cast<int>(msg.wParam);
}

}
}
