#pragma once

#include "util.h"
#include <windows.h>
#pragma warning(disable: 4005)
#include <d3d11.h>
#include <memory>
#include <unordered_map>

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

struct Texture : private util::noncopyable {
	using RvPtr = util::IUnknownPtr<ID3D11ShaderResourceView>;
	RvPtr pRV;
	uint32_t w;
	uint32_t h;

	Texture(RvPtr::pointer pRV_, RvPtr::deleter_type dRV_, uint32_t w_, uint32_t h_) :
		pRV(pRV_, dRV_), w(w_), h(h_)
	{}
	~Texture() = default;
};

struct DrawTask {
	Texture *texture;

	explicit DrawTask(Texture *texture_) :
		texture(texture_)
	{}
	DrawTask(const DrawTask &) = default;
	DrawTask &operator=(const DrawTask &) = default;
	~DrawTask() = default;
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

	void loadTexture(const char *id, const wchar_t *path);
	void drawTexture(const char *id, int x, int y, bool lrMirror = false);
	/**
	* @param id string id
	* @param dx destination X (center pos)
	* @param dy destination Y (center pos)
	* @param lrInv left-right invert
	* @param udInv up-down invert
	* @param sx source X
	* @param sy source Y
	* @param sw source width
	* @param sh source height
	* @param cx center X from (sx, sy)
	* @param cy center Y from (sx, sy)
	* @param scaleX size scaling factor X
	* @param scaleY size scaling factor Y
	* @param angle rotation angle [rad] (using center pos)
	*/
	void drawTexture(const char *id,
		int dx, int dy, bool lrInv, bool udInv,
		int sx, int sy, int sw, int sh,
		int cx, int cy, float scaleX, float scaleY, float angle);

protected:

private:
	const UINT_PTR TimerEventId = 0xffff0001;
	const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_SWAP_CHAIN_FLAG SwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	const float ClearColor[4] = { 0.9f, 0.9f, 0.9f, 1.0f };
	const size_t DrawListMax = 1024;		// not strict limit
	const wchar_t * const VS_FileName = L"@VertexShader.cso";
	const wchar_t * const PS_FileName = L"@PixelShader.cso";

	using HWndPtr = std::unique_ptr<HWND, hwndDeleter>;
	HWndPtr m_hWnd;

	InitParam m_initParam;
	FrameControl m_frameCtrl;
	util::IUnknownPtr<ID3D11Device>				m_pDevice;
	util::IUnknownPtr<ID3D11DeviceContext>		m_pContext;
	util::IUnknownPtr<IDXGISwapChain>			m_pSwapChain;
	util::IUnknownPtr<ID3D11RenderTargetView>	m_pRenderTargetView;
	util::IUnknownPtr<ID3D11VertexShader>		m_pVertexShader;
	util::IUnknownPtr<ID3D11PixelShader>		m_pPixelShader;
	util::IUnknownPtr<ID3D11InputLayout>		m_pInputLayout;
	util::IUnknownPtr<ID3D11Buffer>				m_pVertexBuffer;
	util::IUnknownPtr<ID3D11Buffer>				m_pCBChanges;
	util::IUnknownPtr<ID3D11RasterizerState>	m_pRasterizerState;
	util::IUnknownPtr<ID3D11SamplerState>		m_pSamplerState;
	util::IUnknownPtr<ID3D11BlendState>			m_pBlendState;

	std::unordered_map<std::string, Texture> m_texMap;
	std::vector<DrawTask> m_drawTaskList;

	void initializeWindow(const InitParam &param);
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void initializeD3D(const InitParam &param);
	void initBackBuffer();

	void onIdle();
	void updateInternal();
	void renderInternal();
};

}
}
