#include "stdafx.h"
#include "include/graphics.h"
#include "include/exceptions.h"
#include <array>

#pragma comment(lib, "d3d11.lib")

namespace yappy {
namespace graphics {

using error::checkWin32Result;
using error::D3DError;
using error::checkDXResult;

Application::Application(const InitParam &param)
{
	initializeWindow(param);
	initializeD3d(param);
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

void Application::initializeD3d(const InitParam &param)
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	std::array<D3D_DRIVER_TYPE, 2> driverTypes {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP
	};

	std::array<D3D_FEATURE_LEVEL, 3> featureLevels {
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
	sd.Windowed = param.windowed;

	ID3D11Device *pD3dDevice = nullptr;
	ID3D11DeviceContext *pImmediateContext = nullptr;
	IDXGISwapChain *pSwapChain = nullptr;
	D3D_FEATURE_LEVEL featureLevel = static_cast<D3D_FEATURE_LEVEL>(0);

	for (auto driverType : driverTypes) {
		hr = ::D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr,
			createDeviceFlags, &featureLevels[0], featureLevels.size(),
			D3D11_SDK_VERSION, &sd, &pSwapChain, &pD3dDevice, &featureLevel, &pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	checkDXResult<D3DError>(hr, "todo");

	// Create a render target view
	ID3D11Texture2D *pBackBuffer = nullptr;
	hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&pBackBuffer);
	checkDXResult<D3DError>(hr, "todo");

	ID3D11RenderTargetView *pRenderTargetView = nullptr;
	hr = pD3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
	pBackBuffer->Release();
	checkDXResult<D3DError>(hr, "todo");

	pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(param.w);
	vp.Height = static_cast<float>(param.h);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	pImmediateContext->RSSetViewports(1, &vp);

	//TODO
	m_pD3dDevice = pD3dDevice;
	m_pImmediateContext = pImmediateContext;
	m_pSwapChain = pSwapChain;
	m_pRenderTargetView = pRenderTargetView;
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
			float color[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
			m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, color);
			m_pSwapChain->Present(0, 0);
		}
	}
	return static_cast<int>(msg.wParam);
}

}
}
