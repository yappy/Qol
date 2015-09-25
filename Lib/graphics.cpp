#include "stdafx.h"
#include "include/graphics.h"
#include "include/debug.h"
#include "include/exceptions.h"
#include "include/file.h"
#include <array>
#pragma warning(push)
#pragma warning(disable: 4838)
#include <xnamath.h>
#pragma warning(pop)

#pragma comment(lib, "d3d11.lib")

namespace yappy {
namespace graphics {

using error::checkWin32Result;
using error::D3DError;
using error::checkDXResult;

///////////////////////////////////////////////////////////////////////////////
// class FrameControl impl
///////////////////////////////////////////////////////////////////////////////

namespace {

inline int64_t getTimeCounter()
{
	LARGE_INTEGER cur;
	BOOL b = ::QueryPerformanceCounter(&cur);
	error::checkWin32Result(b != 0, "QueryPerformanceCounter() failed");
	return cur.QuadPart;
}

}	// namespace

FrameControl::FrameControl(int64_t fpsNumer, int64_t fpsDenom) :
	m_base(0), m_skipCount(0),
	m_fpsPeriod(static_cast<int>(fpsNumer / fpsDenom))
{
	// counter/sec
	LARGE_INTEGER freq;
	BOOL b = ::QueryPerformanceFrequency(&freq);
	error::checkWin32Result(b != FALSE, "QueryPerformanceFrequency() failed");
	m_freq = freq.QuadPart;

	// counter/frame = (counter/sec) / (frame/sec)
	//               = m_freq / (fpsNumer / fpsDenom)
	//               = m_freq * fpsDenom / fpsNumer
	m_countPerFrame = m_freq * fpsDenom / fpsNumer;
}

bool FrameControl::shouldSkipFrame()
{
	return m_skipCount > 0;
}

void FrameControl::endFrame()
{
	if (shouldSkipFrame()) {
		m_fpsSkipAcc++;
	}
	else {
		m_fpsFrameAcc++;
	}

	int64_t target = m_base + m_countPerFrame * (m_skipCount + 1);
	int64_t cur = getTimeCounter();
	// cur < target					OK
	// m_base == 0					the first frame, force OK
	// m_skipCount > MaxSkipCount	force OK
	if (cur < target || m_base == 0 || m_skipCount > MaxSkipCount) {
		// OK, wait for next frame
		while (cur < target && m_base != 0 && m_skipCount <= MaxSkipCount) {
			cur = getTimeCounter();
			::Sleep(1);
		}
		m_base = cur;
		m_skipCount = 0;
	}
	else {
		// too late, skip the next frame
		m_skipCount++;
	}

	m_fpsCount++;
	if (m_fpsCount >= m_fpsPeriod) {
		double sec = static_cast<double>(cur - m_fpsBase) / m_freq;
		m_fps = m_fpsFrameAcc / sec;
		m_sps = m_fpsSkipAcc;

		m_fpsCount = 0;
		m_fpsFrameAcc = 0;
		m_fpsSkipAcc = 0;
		m_fpsBase = cur;

		debug::writef(L"fps=%.2f (%d)", getFramePerSec(), getSkipPerSec());
	}
}

double FrameControl::getFramePerSec()
{
	return m_fps;
}

int FrameControl::getSkipPerSec()
{
	return m_sps;
}

///////////////////////////////////////////////////////////////////////////////
// class Application impl
///////////////////////////////////////////////////////////////////////////////

namespace {

struct SpriteVertex
{
	XMFLOAT3 Pos;
};

}

Application::Application(const InitParam &param) :
	m_initParam(param),
	m_frameCtrl(param.refreshRateNumer, param.refreshRateDenom),
	m_pDevice(nullptr, util::iunknownDeleter),
	m_pContext(nullptr, util::iunknownDeleter),
	m_pSwapChain(nullptr, util::iunknownDeleter),
	m_pRenderTargetView(nullptr, util::iunknownDeleter),
	m_pVertexShader(nullptr, util::iunknownDeleter),
	m_pPixelShader(nullptr, util::iunknownDeleter),
	m_pInputLayout(nullptr, util::iunknownDeleter),
	m_pVertexBuffer(nullptr, util::iunknownDeleter)
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

	// for frame processing while window drugging etc.
	::SetTimer(m_hWnd.get(), TimerEventId, 1, nullptr);
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
		IDXGIDevice1 *ptmpDXGIDevice = nullptr;
		hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&ptmpDXGIDevice);
		checkDXResult<D3DError>(hr, "QueryInterface(IDXGIDevice1) failed");
		util::IUnknownPtr<IDXGIDevice1> pDXGIDevice(ptmpDXGIDevice, util::iunknownDeleter);
		hr = pDXGIDevice->SetMaximumFrameLatency(1);
		checkDXResult<D3DError>(hr, "IDXGIDevice1::SetMaximumFrameLatency() failed");
	}

	initBackBuffer();

	// Vertex Shader
	{
		file::Bytes bin = file::loadFile(VS_FileName);
		ID3D11VertexShader *ptmpVS = nullptr;
		hr = m_pDevice->CreateVertexShader(bin.data(), bin.size(), nullptr, &ptmpVS);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateVertexShader() failed");
		m_pVertexShader.reset(ptmpVS);

		// Create input layout
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		ID3D11InputLayout *ptmpInputLayout = nullptr;
		hr = m_pDevice->CreateInputLayout(layout, _countof(layout), bin.data(),
			bin.size(), &ptmpInputLayout);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateInputLayout() failed");
		m_pInputLayout.reset(ptmpInputLayout);
		// Set input layout
		m_pContext->IASetInputLayout(m_pInputLayout.get());
	}
	// Pixel Shader
	{
		file::Bytes bin = file::loadFile(PS_FileName);
		ID3D11PixelShader *ptmpPS = nullptr;
		hr = m_pDevice->CreatePixelShader(bin.data(), bin.size(), nullptr, &ptmpPS);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreatePixelShader() failed");
		m_pPixelShader.reset(ptmpPS);
	}

	// Create vertex buffer
	{
		SpriteVertex vertices[] = {
			XMFLOAT3(0.0f, 0.5f, 0.5f),
			XMFLOAT3(0.5f, -0.5f, 0.5f),
			XMFLOAT3(-0.5f, -0.5f, 0.5f),
		};
		D3D11_BUFFER_DESC bd = { 0 };
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = vertices;
		ID3D11Buffer *ptmpVertexBuffer = nullptr;
		hr = m_pDevice->CreateBuffer(&bd, &initData, &ptmpVertexBuffer);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateBuffer() failed");
		m_pVertexBuffer.reset(ptmpVertexBuffer);

		// Set vertex buffer
		ID3D11Buffer *pVertexBuffer = m_pVertexBuffer.get();
		UINT stride = sizeof(SpriteVertex);
		UINT offset = 0;
		m_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		// Set primitive topology
		m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	case WM_TIMER:
		self->onIdle();
		return 0;
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

void Application::onIdle()
{
	updateInternal();
	if (!m_frameCtrl.shouldSkipFrame()) {
		renderInternal();
	}
	m_frameCtrl.endFrame();
}

void Application::updateInternal()
{
	// TEST
	// update() = 0 ms
}

void Application::renderInternal()
{
	// TEST
	// 30fps by frame skip test
	// render() > 16.67 ms
	::Sleep(17);
	m_pContext->ClearRenderTargetView(m_pRenderTargetView.get(), ClearColor);

	m_pContext->VSSetShader(m_pVertexShader.get(), nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader.get(), nullptr, 0);
	m_pContext->Draw(3, 0);

	m_pSwapChain->Present(1, 0);
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
			onIdle();
		}
	}
	return static_cast<int>(msg.wParam);
}

}
}
