﻿#include "stdafx.h"
#include "include/graphics.h"
#include "include/debug.h"
#include "include/exceptions.h"
#include "include/file.h"
#include <d3dx11.h>
#pragma warning(push)
#pragma warning(disable: 4838)
#include <xnamath.h>
#pragma warning(pop)

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")

namespace yappy {
namespace graphics {

using error::throwTrace;
using error::checkWin32Result;
using error::D3DError;
using error::checkDXResult;

namespace {

struct SpriteVertex {
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct CBNeverChanges {
	XMMATRIX	Projection;
};

struct CBChanges {
	XMMATRIX	lrInv;
	XMMATRIX	udInv;
	XMMATRIX	DestScale;
	XMMATRIX	Centering;
	XMMATRIX	Scale;
	XMMATRIX	Rotation;
	XMMATRIX	Translate;
	XMFLOAT2	uvOffset;
	XMFLOAT2	uvSize;
	XMFLOAT4	FontColor;	// rgba
	float		Alpha;
};

inline void createCBFromTask(CBChanges *out, const DrawTask &task)
{
	// ax + by + cz + d = 0
	// x = 0.5
	XMVECTORF32 xMirror = { 1.0f, 0.0f, 0.0f, -0.5f };
	// y = 0.5
	XMVECTORF32 yMirror = { 0.0f, 1.0f, 0.0f, -0.5f };
	out->lrInv = XMMatrixTranspose(
		task.lrInv ? XMMatrixReflect(xMirror) : XMMatrixIdentity());
	out->udInv = XMMatrixTranspose(
		task.udInv ? XMMatrixReflect(yMirror) : XMMatrixIdentity());
	out->DestScale = XMMatrixTranspose(XMMatrixScaling(
		static_cast<float>(task.sw), static_cast<float>(task.sh), 1.0f));
	out->Centering = XMMatrixTranspose(XMMatrixTranslation(
		-static_cast<float>(task.cx), -static_cast<float>(task.cy), 0.0f));
	out->Scale = XMMatrixTranspose(XMMatrixScaling(
		task.scaleX, task.scaleY, 1.0f));
	out->Rotation = XMMatrixTranspose(XMMatrixRotationZ(task.angle));
	out->Translate = XMMatrixTranspose(XMMatrixTranslation(
		static_cast<float>(task.dx), static_cast<float>(task.dy), 1.0f));
	out->uvOffset = XMFLOAT2(
		static_cast<float>(task.sx) / task.texW,
		static_cast<float>(task.sy) / task.texH);
	out->uvSize = XMFLOAT2(
		static_cast<float>(task.sw) / task.texW,
		static_cast<float>(task.sh) / task.texH);
	out->FontColor = XMFLOAT4(
		((task.fontColor & 0x00ff0000) >> 16) / 255.0f, 
		((task.fontColor & 0x0000ff00) >>  8) / 255.0f, 
		((task.fontColor & 0x000000ff) >>  0) / 255.0f,
		((task.fontColor & 0xff000000) >> 24) / 255.0f);
	out->Alpha = task.alpha;
}

}	// namespace

DGraphics::DGraphics(const GraphicsParam &param) :
	m_param(param)
{
	m_drawTaskList.reserve(DrawListMax);

	initializeD3D();
}

void DGraphics::initializeD3D()
{
	debug::writeLine(L"Initializing Direct3D 11...");

	HRESULT hr = S_OK;

	if (!::XMVerifyCPUSupport()) {
		throwTrace<D3DError>("XMVerifyCPUSupport() failed", 0);
	}

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
		sd.BufferDesc.Width = m_param.w;
		sd.BufferDesc.Height = m_param.h;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = m_param.refreshRate;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = m_param.hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.Flags = SwapChainFlag;

		ID3D11Device *ptmpDevice = nullptr;
		ID3D11DeviceContext *ptmpContext = nullptr;
		IDXGISwapChain *ptmpSwapChain = nullptr;
		D3D_FEATURE_LEVEL featureLevel = static_cast<D3D_FEATURE_LEVEL>(0);

		for (auto driverType : driverTypes) {
			debug::writef(L"Creating device and swapchain... (driverType=%d)",
				static_cast<int>(driverType));
			hr = ::D3D11CreateDeviceAndSwapChain(nullptr, driverType, nullptr,
				createDeviceFlags, featureLevels.data(), static_cast<UINT>(featureLevels.size()),
				D3D11_SDK_VERSION, &sd,
				&ptmpSwapChain, &ptmpDevice, &featureLevel, &ptmpContext);
			if (SUCCEEDED(hr)) {
				debug::writef(L"Creating device and swapchain OK (Feature level=0x%04x)",
					static_cast<uint32_t>(featureLevel));
				break;
			}
			else {
				debug::writeLine(L"Creating device and swapchain: Failed");
			}
		}
		checkDXResult<D3DError>(hr, "D3D11CreateDeviceAndSwapChain() failed");
		m_pDevice.reset(ptmpDevice);
		m_pContext.reset(ptmpContext);
		m_pSwapChain.reset(ptmpSwapChain);
	}

	// MaximumFrameLatency
	{
		debug::writeLine(L"SetMaximumFrameLatency...");

		IDXGIDevice1 *ptmpDXGIDevice = nullptr;
		hr = m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&ptmpDXGIDevice);
		checkDXResult<D3DError>(hr, "QueryInterface(IDXGIDevice1) failed");
		util::ComPtr<IDXGIDevice1> pDXGIDevice(ptmpDXGIDevice);
		hr = pDXGIDevice->SetMaximumFrameLatency(1);
		checkDXResult<D3DError>(hr, "IDXGIDevice1::SetMaximumFrameLatency() failed");

		debug::writeLine(L"SetMaximumFrameLatency OK");
	}

	initBackBuffer();

	// Vertex Shader
	debug::writeLine(L"Creating vertex shader...");
	{
		file::Bytes bin = file::loadFile(VS_FileName);
		ID3D11VertexShader *ptmpVS = nullptr;
		hr = m_pDevice->CreateVertexShader(bin.data(), bin.size(), nullptr, &ptmpVS);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateVertexShader() failed");
		m_pVertexShader.reset(ptmpVS);

		// Create input layout
		debug::writeLine(L"Creating input layout...");
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		ID3D11InputLayout *ptmpInputLayout = nullptr;
		hr = m_pDevice->CreateInputLayout(layout, _countof(layout), bin.data(),
			bin.size(), &ptmpInputLayout);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateInputLayout() failed");
		m_pInputLayout.reset(ptmpInputLayout);
		// Set input layout
		m_pContext->IASetInputLayout(m_pInputLayout.get());
		debug::writeLine(L"Creating input layout OK");
	}
	debug::writeLine(L"Creating vertex shader OK");
	// Pixel Shader
	debug::writeLine(L"Creating pixel shader...");
	{
		file::Bytes bin = file::loadFile(PS_FileName);
		ID3D11PixelShader *ptmpPS = nullptr;
		hr = m_pDevice->CreatePixelShader(bin.data(), bin.size(), nullptr, &ptmpPS);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreatePixelShader() failed");
		m_pPixelShader.reset(ptmpPS);
	}
	debug::writeLine(L"Creating pixel shader OK");

	// Create vertex buffer
	{
		debug::writeLine(L"Creating vertex buffer...");

		SpriteVertex vertices[] = {
			{ XMFLOAT3(0.0f, 1.0f, 0.0f) , XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 0.0f) , XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(0.0f, 0.0f, 0.0f) , XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 0.0f, 0.0f) , XMFLOAT2(1.0f, 0.0f) },
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

		debug::writeLine(L"Creating vertex buffer OK");

		// Set vertex buffer
		ID3D11Buffer *pVertexBuffer = m_pVertexBuffer.get();
		UINT stride = sizeof(SpriteVertex);
		UINT offset = 0;
		m_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
		// Set primitive topology
		m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		debug::writeLine(L"Set vertex buffer OK");
	}
	// Create constant buffer
	debug::writeLine(L"Creating constant buffer...");
	{
		CBNeverChanges buffer;
		// Projection matrix: Window coord -> (-1, -1) .. (1, 1)
		buffer.Projection = XMMatrixIdentity();
		buffer.Projection._41 = -1.0f;
		buffer.Projection._42 = 1.0f;
		buffer.Projection._11 = 2.0f / m_param.w;
		buffer.Projection._22 = -2.0f / m_param.h;
		buffer.Projection = XMMatrixTranspose(buffer.Projection);
		D3D11_BUFFER_DESC bd = { 0 };
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBNeverChanges);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA initData = { 0 };
		initData.pSysMem = &buffer;
		ID3D11Buffer *ptmpCBNeverChanges = nullptr;
		hr = m_pDevice->CreateBuffer(&bd, &initData, &ptmpCBNeverChanges);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateBuffer() failed");
		m_pCBNeverChanges.reset(ptmpCBNeverChanges);
	}
	{
		D3D11_BUFFER_DESC bd = { 0 };
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBChanges);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		ID3D11Buffer *ptmpCBChanges = nullptr;
		hr = m_pDevice->CreateBuffer(&bd, nullptr, &ptmpCBChanges);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateBuffer() failed");
		m_pCBChanges.reset(ptmpCBChanges);
	}
	debug::writeLine(L"Creating constant buffer OK");

	// Create rasterizer state
	debug::writeLine(L"Creating rasterizer state...");
	{
		D3D11_RASTERIZER_DESC rasterDesc;
		::ZeroMemory(&rasterDesc, sizeof(rasterDesc));
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.CullMode = D3D11_CULL_NONE;
		ID3D11RasterizerState* ptmpRasterizerState = nullptr;
		hr = m_pDevice->CreateRasterizerState(&rasterDesc, &ptmpRasterizerState);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateBuffer() failed");
		m_pRasterizerState.reset(ptmpRasterizerState);
	}
	debug::writeLine(L"Creating rasterizer state OK");
	// Create sampler state
	debug::writeLine(L"Creating sampler state...");
	{
		D3D11_SAMPLER_DESC sampDesc;
		::ZeroMemory(&sampDesc, sizeof(sampDesc));
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		ID3D11SamplerState *ptmpSamplerState = nullptr;
		hr = m_pDevice->CreateSamplerState(&sampDesc, &ptmpSamplerState);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateSamplerState() failed");
		m_pSamplerState.reset(ptmpSamplerState);
	}
	debug::writeLine(L"Creating sampler state OK");
	// Create blend state
	debug::writeLine(L"Creating blend state...");
	{
		D3D11_BLEND_DESC blendDesc;
		::ZeroMemory(&blendDesc, sizeof(blendDesc));
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		ID3D11BlendState* ptmpBlendState = nullptr;
		hr = m_pDevice->CreateBlendState(&blendDesc, &ptmpBlendState);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateBlendState() failed");
		m_pBlendState.reset(ptmpBlendState);
	}
	debug::writeLine(L"Creating blend state OK");

	// Multithread
	{
		debug::writeLine(L"Set multithreading...");

		ID3D10Multithread *ptmpMt = nullptr;
		hr = m_pDevice->QueryInterface(__uuidof(ID3D10Multithread), (void **)&ptmpMt);
		checkDXResult<D3DError>(hr, "QueryInterface(IDXGIDevice1) failed");
		util::ComPtr<ID3D10Multithread> pMt(ptmpMt);
		pMt->SetMultithreadProtected(TRUE);

		debug::writeLine(L"Set multithreading OK");
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
	if(m_param.fullScreen) {
		debug::writeLine(L"Set fullscreen...");

		hr = m_pSwapChain->SetFullscreenState(TRUE, nullptr);
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE) {
			debug::writeLine(L"Fullscreen mode is currently unavailable");
			debug::writeLine(L"Launch on window mode...");
		}
		else {
			checkDXResult<D3DError>(hr, "IDXGISwapChain::SetFullscreenState() failed");
		}
	}

	debug::writeLine(L"Initializing Direct3D 11 OK");
}

void DGraphics::initBackBuffer()
{
	debug::writeLine(L"initalizing BackBuffer...");

	HRESULT hr = S_OK;

	// Create a render target view
	{
		ID3D11Texture2D *ptmpBackBuffer = nullptr;
		hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&ptmpBackBuffer);
		checkDXResult<D3DError>(hr, "IDXGISwapChain::GetBuffer() failed");
		// Release() at scope end
		util::ComPtr<ID3D11Texture2D> pBackBuffer(ptmpBackBuffer);

		ID3D11RenderTargetView *ptmpRenderTargetView = nullptr;
		hr = m_pDevice->CreateRenderTargetView(
			pBackBuffer.get(), nullptr, &ptmpRenderTargetView);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateRenderTargetView() failed");
		m_pContext->OMSetRenderTargets(1, &ptmpRenderTargetView, nullptr);
		m_pRenderTargetView.reset(ptmpRenderTargetView);

		debug::writeLine(L"SetRenderTarget OK");
	}

	// Setup the viewport
	{
		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = static_cast<float>(m_param.w);
		vp.Height = static_cast<float>(m_param.h);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_pContext->RSSetViewports(1, &vp);

		debug::writeLine(L"SetViewPort OK");
	}

	debug::writeLine(L"initalizing BackBuffer OK");
}

DGraphics::~DGraphics()
{
	if (m_pContext != nullptr) {
		m_pContext->ClearState();
	}
	if (m_pSwapChain != nullptr) {
		m_pSwapChain->SetFullscreenState(FALSE, nullptr);
	}
	debug::writeLine(L"Finalize Application, Direct3D 11, window");
}

LRESULT DGraphics::onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

void DGraphics::render()
{
	// Clear target
	m_pContext->ClearRenderTargetView(m_pRenderTargetView.get(), ClearColor);

	// VS, PS, constant buffer
	m_pContext->VSSetShader(m_pVertexShader.get(), nullptr, 0);
	m_pContext->PSSetShader(m_pPixelShader.get(), nullptr, 0);
	ID3D11Buffer *pCBs[2] = { m_pCBNeverChanges.get(), m_pCBChanges.get() };
	m_pContext->VSSetConstantBuffers(0, 2, pCBs);
	// RasterizerState, SamplerState, BlendState
	m_pContext->RSSetState(m_pRasterizerState.get());
	ID3D11SamplerState *pSamplerState = m_pSamplerState.get();
	m_pContext->PSSetSamplers(0, 1, &pSamplerState);
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_pContext->OMSetBlendState(m_pBlendState.get(), blendFactor, 0xffffffff);

	// Draw and clear list
	for (auto &task : m_drawTaskList) {
		// Set constant buffer
		CBChanges cbChanges;
		createCBFromTask(&cbChanges, task);
		m_pContext->UpdateSubresource(m_pCBChanges.get(), 0, nullptr, &cbChanges, 0, 0);

		ID3D11ShaderResourceView *pView = task.pRV.get();
		m_pContext->PSSetShaderResources(0, 1, &pView);

		m_pContext->Draw(4, 0);
	}
	m_drawTaskList.clear();

	// vsync and flip(blt)
	m_pSwapChain->Present(m_param.vsync ? 1 : 0, 0);
}

DGraphics::TextureResourcePtr DGraphics::loadTexture(const wchar_t *path)
{
	file::Bytes bin = file::loadFile(path);

	HRESULT hr = S_OK;

	D3DX11_IMAGE_INFO imageInfo = { 0 };
	hr = ::D3DX11GetImageInfoFromMemory(bin.data(), bin.size(), nullptr,
		&imageInfo, nullptr);
	checkDXResult<D3DError>(hr, "D3DX11GetImageInfoFromMemory() failed");
	if (imageInfo.ResourceDimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		throwTrace<D3DError>("Not 2D Texture", S_OK);
	}

	ID3D11ShaderResourceView *ptmpRV = nullptr;
	hr = ::D3DX11CreateShaderResourceViewFromMemory(m_pDevice.get(), bin.data(), bin.size(),
		nullptr, nullptr, &ptmpRV, nullptr);
	checkDXResult<D3DError>(hr, "D3DX11CreateShaderResourceViewFromMemory() failed");

	return std::make_shared<Texture>(
		ptmpRV, imageInfo.Width, imageInfo.Height);
}

void DGraphics::drawTexture(const TextureResourcePtr &texture,
	int dx, int dy, bool lrInv, bool udInv,
	int sx, int sy, int sw, int sh,
	int cx, int cy, float angle, float scaleX, float scaleY,
	float alpha)
{
	sw = (sw == SrcSizeDefault) ? texture->w : sw;
	sh = (sh == SrcSizeDefault) ? texture->h : sh;
	m_drawTaskList.emplace_back(texture->pRV, texture->w, texture->h,
		dx, dy, lrInv, udInv, sx, sy, sw, sh,
		cx, cy, scaleX, scaleY, angle, 0x00000000, alpha);
}

DGraphics::FontResourcePtr DGraphics::loadFont(const wchar_t *fontName,
	uint32_t startChar, uint32_t endChar, uint32_t w, uint32_t h)
{
	HRESULT hr = S_OK;

	// Create font
	HFONT htmpFont = ::CreateFont(
		h, 0, 0, 0, 0, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		FIXED_PITCH | FF_MODERN, fontName);
	checkWin32Result(htmpFont != nullptr, "CreateFont() failed");
	auto fontDel = [](HFONT hFont) { ::DeleteObject(hFont); };
	std::unique_ptr<std::remove_pointer<HFONT>::type, decltype(fontDel)>
		hFont(htmpFont, fontDel);
	// Get DC
	HDC htmpDC = ::GetDC(nullptr);
	checkWin32Result(htmpDC != nullptr, "GetDC() failed");
	auto dcRel = [](HDC hDC) { ::ReleaseDC(nullptr, hDC); };
	std::unique_ptr<std::remove_pointer<HDC>::type, decltype(dcRel)>
		hDC(htmpDC, dcRel);
	// Select font
	HFONT tmpOldFont = static_cast<HFONT>(::SelectObject(hDC.get(), hFont.get()));
	checkWin32Result(tmpOldFont != nullptr, "SelectObject() failed");
	auto restoreObj = [&hDC](HFONT oldFont) { ::SelectObject(hDC.get(), oldFont); };
	std::unique_ptr<std::remove_pointer<HFONT>::type, decltype(restoreObj)>
		hOldFont(tmpOldFont, restoreObj);

	TEXTMETRIC tm = { 0 };
	checkWin32Result(::GetTextMetrics(hDC.get(), &tm) != FALSE,
		"GetTextMetrics() failed");

	std::vector<FontTexture::TexPtr> texList;
	std::vector<FontTexture::RvPtr> rvList;
	texList.reserve(endChar - startChar + 1);
	rvList.reserve(endChar - startChar + 1);

	for (uint32_t c = startChar; c <= endChar; c++) {
		// Get font bitmap
		GLYPHMETRICS gm = { 0 };
		const MAT2 mat = { { 0, 1 },{ 0, 0 },{ 0, 0 },{ 0, 1 } };
		DWORD bufSize = ::GetGlyphOutline(hDC.get(), c, GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat);
		checkWin32Result(bufSize != GDI_ERROR, "GetGlyphOutline() failed");
		auto buf = std::make_unique<uint8_t[]>(bufSize);
		DWORD ret = ::GetGlyphOutline(hDC.get(), c, GGO_GRAY8_BITMAP, &gm, bufSize, buf.get(), &mat);
		checkWin32Result(ret != GDI_ERROR, "GetGlyphOutline() failed");
		uint32_t pitch = (gm.gmBlackBoxX + 3) / 4 * 4;
		// Black box pos in while box
		uint32_t destX = gm.gmptGlyphOrigin.x;
		uint32_t destY = tm.tmAscent - gm.gmptGlyphOrigin.y;

		// Create texture
		D3D11_TEXTURE2D_DESC desc = { 0 };
		desc.Width = w;
		desc.Height = h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ID3D11Texture2D *ptmpTex = nullptr;
		hr = m_pDevice->CreateTexture2D(&desc, nullptr, &ptmpTex);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateTexture2D() failed");
		// make unique_ptr and push
		texList.emplace_back(ptmpTex);

		// Write
		D3D11_MAPPED_SUBRESOURCE mapped;
		UINT subres = ::D3D11CalcSubresource(0, 0, 1);
		hr = m_pContext->Map(ptmpTex, subres, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		checkDXResult<D3DError>(hr, "ID3D11DeviceContext::Map() failed");
		uint8_t *pTexels = static_cast<uint8_t *>(mapped.pData);
		::ZeroMemory(pTexels, w * h * 4);
		for (uint32_t y = 0; y < gm.gmBlackBoxY; y++) {
			for (uint32_t x = 0; x < gm.gmBlackBoxX; x++) {
				if (destX + x >= w || destY + y >= h) {
					continue;
				}
				uint32_t alpha = buf[y * pitch + x] * 255 / 64;
				uint32_t destInd = (((destY + y) * w) + (destX + x)) * 4;
				pTexels[destInd + 0] = 0;
				pTexels[destInd + 1] = 0;
				pTexels[destInd + 2] = 0;
				pTexels[destInd + 3] = alpha;
			}
		}
		m_pContext->Unmap(ptmpTex, subres);

		// Create resource view
		ID3D11ShaderResourceView *ptmpRV = nullptr;
		hr = m_pDevice->CreateShaderResourceView(ptmpTex, nullptr, &ptmpRV);
		checkDXResult<D3DError>(hr, "ID3D11Device::CreateShaderResourceView() failed");
		// make unique_ptr and push
		rvList.emplace_back(ptmpRV);
	}

	// add (id, FontTexture(...))
	auto res = std::make_shared<FontTexture>(
		w, h, startChar, endChar);
	res->pTexList.swap(texList);
	res->pRVList.swap(rvList);
	return res;
}

void DGraphics::drawChar(const FontResourcePtr &font, wchar_t c, int dx, int dy,
	uint32_t color, float scaleX, float scaleY, float alpha,
	int *nextx, int *nexty)
{
	// skip if space
	if (!::iswspace(c)) {
		// Set alpha 0xff
		color |= 0xff000000;
		auto &pRV = font->pRVList.at(c - font->startChar);
		m_drawTaskList.emplace_back(pRV, font->w, font->h,
			dx, dy, false, false, 0, 0, font->w, font->h,
			0, 0, scaleX, scaleY, 0.0f, color, alpha);
	}

	if (nextx != nullptr) {
		*nextx = dx + font->w;
	}
	if (nexty != nullptr) {
		*nexty = dy + font->h;
	}
}

void DGraphics::drawString(const FontResourcePtr &font, const wchar_t *str, int dx, int dy,
	uint32_t color, int ajustX, float scaleX, float scaleY, float alpha,
	int *nextx, int *nexty)
{
	while (*str != L'\0') {
		drawChar(font, *str, dx, dy, color, scaleX, scaleY, alpha, &dx, nexty);
		dx += ajustX;
		str++;
	}
	if (nextx != nullptr) {
		*nextx = dx;
	}
}

#pragma endregion

}	// namespace graphics
}	// namespace yappy
