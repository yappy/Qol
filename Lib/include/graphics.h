#pragma once

#include "util.h"
#include <windows.h>
#pragma warning(disable: 4005)
#include <d3d11.h>
#include <memory>
#include <unordered_map>

namespace yappy {
namespace graphics {

struct Texture : private util::noncopyable {
	using RvPtr = util::IUnknownPtr<ID3D11ShaderResourceView>;
	RvPtr pRV;
	uint32_t w, h;

	Texture(RvPtr::pointer pRV_, RvPtr::deleter_type dRV_, uint32_t w_, uint32_t h_) :
		pRV(pRV_, dRV_), w(w_), h(h_)
	{}
	~Texture() = default;
};

struct FontTexture : private util::noncopyable {
	using TexPtr = util::IUnknownPtr<ID3D11Texture2D>;
	using RvPtr = util::IUnknownPtr<ID3D11ShaderResourceView>;
	std::vector<TexPtr> pTexList;
	std::vector<RvPtr> pRVList;
	uint32_t w, h;
	uint32_t startChar, endChar;

	FontTexture(uint32_t w_, uint32_t h_, uint32_t startChar_, uint32_t endChar_) :
		w(w_), h(h_), startChar(startChar_), endChar(endChar_)
	{}
	~FontTexture() = default;
};

struct DrawTask {
	ID3D11ShaderResourceView *pRV;
	uint32_t texW, texH;
	int dx, dy;
	bool lrInv, udInv;
	int sx, sy, sw, sh;
	int cx, cy;
	float scaleX, scaleY;
	float angle;
	uint32_t fontColor;		// ARGB
	float alpha;

	DrawTask(ID3D11ShaderResourceView *pRV_,
		uint32_t texW_, uint32_t texH_,
		int dx_, int dy_, bool lrInv_, bool udInv_,
		int sx_, int sy_, int sw_, int sh_,
		int cx_, int cy_, float scaleX_, float scaleY_, float angle_,
		uint32_t fontColor_, float alpha_) :
		pRV(pRV_), texW(texW_), texH(texH_),
		dx(dx_), dy(dy_), lrInv(lrInv_), udInv(udInv_),
		sx(sx_), sy(sy_), sw(sw_), sh(sh_),
		cx(cx_), cy(cy_), scaleX(scaleX_), scaleY(scaleY_), angle(angle_),
		fontColor(fontColor_), alpha(alpha_)
	{}
	DrawTask(const DrawTask &) = default;
	DrawTask &operator=(const DrawTask &) = default;
	~DrawTask() = default;
};

struct GraphicsParam {
	HWND hWnd = nullptr;
	int w = 1024;
	int h = 768;
	uint32_t refreshRate = 60;
	bool fullScreen = false;
	bool vsync = true;
};

/** @brief DirectGraphics manager.
 */
class DGraphics : private util::noncopyable {
public:
	explicit DGraphics(const GraphicsParam &param);
	~DGraphics();

	void render();
	LRESULT onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	using TextureResource = std::shared_ptr<const Texture>;

	/**@brief Use texture size.
	  * @details You can use cw, ch in drawTexture().
	  */
	static const int SrcSizeDefault = -1;

	/**@brief Load a texture.
	  * @param[in] path file path
	  */
	TextureResource loadTexture(const wchar_t *path);

	/** @brief Draw texture.
	 * @param[in] texture texture resource
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
	void drawTexture(const TextureResource &texture,
		int dx, int dy, bool lrInv = false, bool udInv = false,
		int sx = 0, int sy = 0, int sw = SrcSizeDefault, int sh = SrcSizeDefault,
		int cx = 0, int cy = 0, float angle = 0.0f,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);

	using FontResource = std::shared_ptr<const FontTexture>;

	FontResource loadFont(const wchar_t *fontName, uint32_t startChar, uint32_t endChar,
		uint32_t w, uint32_t h);

	void drawChar(const FontResource &font, wchar_t c, int dx, int dy,
		uint32_t color = 0x000000,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f,
		int *nextx = nullptr, int *nexty = nullptr);

	void drawString(const FontResource &font, const wchar_t *str, int dx, int dy,
		uint32_t color = 0x000000, int ajustX = 0,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f,
		int *nextx = nullptr, int *nexty = nullptr);

private:
	const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_SWAP_CHAIN_FLAG SwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	const float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const size_t DrawListMax = 1024;		// not strict limit
	const wchar_t * const VS_FileName = L"@VertexShader.cso";
	const wchar_t * const PS_FileName = L"@PixelShader.cso";

	GraphicsParam m_param;
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

	std::vector<DrawTask> m_drawTaskList;

	void initializeD3D();
	void initBackBuffer();
};

}
}
