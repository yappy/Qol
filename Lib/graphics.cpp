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

FrameControl::FrameControl(int64_t fpsNumer, int64_t fpsDenom) :
	m_prev(0)
{
	// counter/sec
	LARGE_INTEGER freq;
	BOOL b = ::QueryPerformanceFrequency(&freq);
	error::checkWin32Result(b != FALSE, "QueryPerformanceFrequency() failed");
	m_freq = freq.QuadPart;

	// counter/frame = (counter/sec) / (frame/sec)
	//               = m_freq / (fpsNumer / fpsDenom)
	//               = m_freq * fpsDenom / fpsNumer
	m_target = m_freq * fpsDenom / fpsNumer;
	m_target = static_cast<int64_t>(m_target * SCALE);
}

void FrameControl::endFrame()
{
	LARGE_INTEGER cur;
	do {
		BOOL b = ::QueryPerformanceCounter(&cur);
		error::checkWin32Result(b != 0, "QueryPerformanceCounter() failed");
	} while (cur.QuadPart < m_prev + m_target);

	m_fpsCount++;
	m_fpsAcc += cur.QuadPart - m_prev;
	if (m_fpsCount >= 60) {
		double sec = static_cast<double>(m_fpsAcc) / m_freq;
		debug::writef(L"fps=%.2f", m_fpsCount / sec);
		m_fpsCount = 0;
		m_fpsAcc = 0;
	}

	m_prev = cur.QuadPart;
}

Application::Application(const InitParam &param) :
	m_initParam(param),
	m_frameCtrl(param.refreshRateNumer, param.refreshRateDenom),
	m_pDevice(nullptr, util::iunknownDeleter),
	m_pContext(nullptr, util::iunknownDeleter),
	m_pSwapChain(nullptr, util::iunknownDeleter),
	m_pRenderTargetView(nullptr, util::iunknownDeleter)
{
	initializeWindow(param);
	initializeD3D(param);
	::ShowWindow(m_hWnd.get(), param.nCmdShow);
	::UpdateWindow(m_hWnd.get());
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
	cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
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
		nullptr, nullptr, param.hInstance, this);
	checkWin32Result(hWnd != nullptr, "CreateWindow() failed");
	m_hWnd.reset(hWnd);
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
		sd.Flags = SwapChainFlag;

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

	// MaximumFrameLatency
	{
		IDXGIDevice1 *ptmpDXGIDevice;
		hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&ptmpDXGIDevice);
		checkDXResult<D3DError>(hr, "QueryInterface(IDXGIDevice1) failed");
		util::IUnknownPtr<IDXGIDevice1> pDXGIDevice(ptmpDXGIDevice, util::iunknownDeleter);
		hr = pDXGIDevice->SetMaximumFrameLatency(1);
		checkDXResult<D3DError>(hr, "IDXGIDevice1::SetMaximumFrameLatency() failed");
	}

	initBackBuffer();

	// fullscreen initially
	/*
	DXGI_ERROR_NOT_CURRENTLY_AVAILABLE (quote from dx11 document)
	For instance:
	- The application is running over Terminal Server.
	- The output window is occluded.
	- The output window does not have keyboard focus.
	- Another application is already in full-screen mode.
	*/
	if(param.fullScreen) {
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

void Application::initBackBuffer()
{
	HRESULT hr = S_OK;

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
		vp.Width = static_cast<float>(m_initParam.w);
		vp.Height = static_cast<float>(m_initParam.h);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_pContext->RSSetViewports(1, &vp);
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
	if (msg == WM_CREATE) {
		// Associate this ptr with hWnd
		void *userData = reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams;
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userData));
	}

	LONG_PTR user = ::GetWindowLongPtr(hWnd, GWLP_USERDATA);
	Application *self = reinterpret_cast<Application *>(user);

	switch (msg) {
	case WM_SIZE:
		return self->onSize(hWnd, msg, wParam, lParam);
	case WM_CLOSE:
		self->m_hWnd.reset();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT Application::onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_pSwapChain == nullptr || wParam == SIZE_MINIMIZED) {
		debug::writeLine(L"!!!");
		return 0;
	}
	debug::writef(L"WM_SIZE %d %d", LOWORD(lParam), HIWORD(lParam));

	HRESULT hr;
	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_pRenderTargetView.reset();
	hr = m_pSwapChain->ResizeBuffers(1, 0, 0, BufferFormat, SwapChainFlag);
	checkDXResult<D3DError>(hr, "IDXGISwapChain::ResizeBuffers() failed");

	initBackBuffer();

	return DefWindowProc(hWnd, msg, wParam, lParam);
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
			//debug::StopWatch test(L"Present()");
			m_pContext->ClearRenderTargetView(m_pRenderTargetView.get(), ClearColor);
			m_pSwapChain->Present(1, 0);
			m_frameCtrl.endFrame();
		}
	}
	return static_cast<int>(msg.wParam);
}

}
}
