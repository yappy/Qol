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
	FrameControl(uint32_t fps, uint32_t skipCount);
	~FrameControl() = default;
	bool shouldSkipFrame();
	void endFrame();
	double getFramePerSec();

private:
	int64_t m_freq;
	int64_t m_counterPerFrame;
	int64_t m_base = 0;
	uint32_t m_skipCount;
	uint32_t m_frameCount = 0;

	double m_fps = 0.0;
	uint32_t m_fpsPeriod;
	uint32_t m_fpsCount = 0;
	int64_t m_fpsBase = 0;
	uint32_t m_fpsFrameAcc = 0;
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
	const Texture *texture;
	int dx, dy;
	bool lrInv, udInv;
	int sx, sy, sw, sh;
	int cx, cy;
	float scaleX, scaleY;
	float angle;
	float alpha;

	explicit DrawTask(const Texture *texture_,
		int dx_, int dy_, bool lrInv_, bool udInv_,
		int sx_, int sy_, int sw_, int sh_,
		int cx_, int cy_, float scaleX_, float scaleY_, float angle_,
		float alpha_) :
		texture(texture_),
		dx(dx_), dy(dy_), lrInv(lrInv_), udInv(udInv_),
		sx(sx_), sy(sy_), sw(sw_), sh(sh_),
		cx(cx_), cy(cy_), scaleX(scaleX_), scaleY(scaleY_), angle(angle_),
		alpha(alpha_)
	{}
	DrawTask(const DrawTask &) = default;
	DrawTask &operator=(const DrawTask &) = default;
	~DrawTask() = default;
};

/** @brief User application base, managing window and Direct3D.
 * @details Please inherit this and override protected methods.
 */
class Application : private util::noncopyable {
public:
	struct InitParam {
		HINSTANCE hInstance = nullptr;
		int nCmdShow = SW_SHOW;
		int w = 1024;
		int h = 768;
		const wchar_t *wndClsName = L"GameWndCls";
		const wchar_t *title = L"GameApp";
		HICON hIcon = nullptr;
		HICON hIconSm = nullptr;
		uint32_t refreshRate = 60;
		bool fullScreen = false;
		bool vsync = true;
		uint32_t frameSkip = 0;
		bool showCursor = false;
	};

	Application(const InitParam &param);
	virtual ~Application();
	int run();

	HWND getHWnd() { return m_hWnd.get(); }

	/**@brief Use texture size.
	  * @details You can use cw, ch in drawTexture().
	  */
	static const int SrcSizeDefault = -1;

	/**@brief Load a texture.
	  * @param[in] id string id
	  * @param[in] path file path
	  */
	void loadTexture(const char *id, const wchar_t *path);

	/**@brief Get texture size.
	  * @param[in] id string id
	  * @param[out] w texture width
	  * @param[out] h texture height
	  */
	void getTextureSize(const char *id, uint32_t *w, uint32_t *h) const;

	/** @brief Draw texture.
	 * @param[in] id string id
	 * @param[in] dx destination X (center pos)
	 * @param[in] dy destination Y (center pos)
	 * @param[in] lrInv left-right invert
	 * @param[in] udInv up-down invert
	 * @param[in] sx source X
	 * @param[in] sy source Y
	 * @param[in] sw source width (texture size if SRC_SIZE_DEFAULT)
	 * @param[in] sh source height (texture size if SRC_SIZE_DEFAULT)
	 * @param[in] cx center X from (sx, sy)
	 * @param[in] cy center Y from (sx, sy)
	 * @param[in] angle rotation angle [rad] (using center pos)
	 * @param[in] scaleX size scaling factor X
	 * @param[in] scaleY size scaling factor Y
	 * @param[in] alpha alpha value
	 */
	void drawTexture(const char *id,
		int dx, int dy, bool lrInv = false, bool udInv = false,
		int sx = 0, int sy = 0, int sw = SrcSizeDefault, int sh = SrcSizeDefault,
		int cx = 0, int cy = 0, float angle = 0.0f,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);

protected:
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void render() = 0;

private:
	const UINT_PTR TimerEventId = 0xffff0001;
	const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_SWAP_CHAIN_FLAG SwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	const float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
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
	util::IUnknownPtr<ID3D11Buffer>				m_pCBNeverChanges, m_pCBChanges;
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
