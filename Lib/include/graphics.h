﻿#pragma once

#include "util.h"
#include <windows.h>
#pragma warning(disable: 4005)
#include <d3d11.h>
#include <memory>
#include <unordered_map>

namespace yappy {
/// Graphics library.
namespace graphics {

/// Texture resource.
struct Texture : private util::noncopyable {
	using RvPtr = util::ComPtr<ID3D11ShaderResourceView>;

	RvPtr pRV;
	uint32_t w, h;

	Texture(RvPtr::pointer pRV_, uint32_t w_, uint32_t h_) :
		pRV(pRV_), w(w_), h(h_)
	{}
	~Texture() = default;
};

/// Font resource.
struct FontTexture : private util::noncopyable {
	using TexPtr = util::ComPtr<ID3D11Texture2D>;
	using RvPtr = util::ComPtr<ID3D11ShaderResourceView>;

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
	using RvPtr = Texture::RvPtr;

	const RvPtr &pRV;
	uint32_t texW, texH;
	int dx, dy;
	bool lrInv, udInv;
	int sx, sy, sw, sh;
	int cx, cy;
	float scaleX, scaleY;
	float angle;
	uint32_t fontColor;		// ARGB
	float alpha;

	DrawTask(const RvPtr &pRV_,
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

/**@brief DirectGraphics parameters.
 * @details Each field has a default value.
 */
struct GraphicsParam {
	/// Window handle.
	HWND hWnd = nullptr;
	/// Viewport size width.
	int w = 1024;
	/// Viewport size height.
	int h = 768;
	/// Refresh rate. (fps)
	uint32_t refreshRate = 60;
	/// Full-screen mode.
	bool fullScreen = false;
	/// Enable V-SYNC.
	bool vsync = true;
};

/**@brief DirectGraphics manager.
 * @details
 * drawXXX() functions don't execute actual drawing.
 * They just queues DrawTask and @ref render() function executes GPU drawing.
 */
class DGraphics : private util::noncopyable {
public:
	using TextureResource = const Texture;
	using TextureResourcePtr = std::shared_ptr<TextureResource>;
	using FontResource = const FontTexture;
	using FontResourcePtr = std::shared_ptr<FontResource>;

	/**@brief Use texture size.
	 * @details You can use cw, ch in drawTexture().
	 */
	static const int SrcSizeDefault = -1;

	/**@brief Initialize DirectGraphics.
	 * @param[in]	param	Graphics parameters.
	 */
	explicit DGraphics(const GraphicsParam &param);
	/// Finalize DirectGraphics.
	~DGraphics();

	/**@brief Renders a frame and wait for vsync (if enabled).
	 */
	void render();

	/**@brief Application must call this function when WM_SIZE message is received.
	 */
	LRESULT onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/// @name Texture
	//@{
	/**@brief Load a texture resource.
	 * @details This function may take time.
	 * @param[in]	path	File path.
	 * @return				shared_ptr to texture resource.
	 * @sa yappy::file
	 */
	TextureResourcePtr loadTexture(const wchar_t *path);

	/**@brief Draw a texture.
	 * @param[in]	texture	Texture resource.
	 * @param[in]	dx		Destination X. (center pos)
	 * @param[in]	dy		Destination Y. (center pos)
	 * @param[in]	lrInv	Left-right invert.
	 * @param[in]	udInv	Up-down invert.
	 * @param[in]	sx		Source X.
	 * @param[in]	sy		Source Y.
	 * @param[in]	sw		Source width. (texture size if SRC_SIZE_DEFAULT)
	 * @param[in]	sh		Source height. (texture size if SRC_SIZE_DEFAULT)
	 * @param[in]	cx		Center X from (sx, sy).
	 * @param[in]	cy		Center Y from (sx, sy).
	 * @param[in]	angle	Rotation angle [rad]. (using center pos)
	 * @param[in]	scaleX	Size scaling factor X.
	 * @param[in]	scaleY	Size scaling factor Y.
	 * @param[in]	alpha	Alpha value.
	 */
	void drawTexture(const TextureResourcePtr &texture,
		int dx, int dy, bool lrInv = false, bool udInv = false,
		int sx = 0, int sy = 0, int sw = SrcSizeDefault, int sh = SrcSizeDefault,
		int cx = 0, int cy = 0, float angle = 0.0f,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f);
	//@}

	/// @name Font
	//@{
	/**@brief Load a font resource.
	 * @details
	 * startChar <= character_code <= endChar will be available.
	 * This function may take time.
	 * @param[in]	fontName	Font name.
	 * @param[in]	startChar	The first character code to be available.
	 * @param[in]	endChar		The last character code to be available.
 	 * @param[in]	w			Size width.
  	 * @param[in]	h			Size height.
	 * @return					shared_ptr to font resource.
	 */
	FontResourcePtr loadFont(const wchar_t *fontName,
		uint32_t startChar, uint32_t endChar, uint32_t w, uint32_t h);

	/**@brief Draw a character.
	 * @param[in]	font	Font resource.
	 * @param[in]	c		Character code. (UTF-16)
	 * @param[in]	dx		Destination X. (center pos)
	 * @param[in]	dy		Destination X. (center pos)
	 * @param[in]	color	Text color. (0xRRGGBB)
	 * @param[in]	scaleX	Scaling factor X.
	 * @param[in]	scaleY	Scaling factor Y.
	 * @param[in]	alpha	Alpha value.
	 * @param[out]	nextx	X of next column.
	 * @param[out]	nexty	Y of next row.
	 */
	void drawChar(const FontResourcePtr &font, wchar_t c, int dx, int dy,
		uint32_t color = 0x000000,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f,
		int *nextx = nullptr, int *nexty = nullptr);

	/**@brief Draw a string.
	 * @param[in]	font	Font resource.
	 * @param[in]	str		Text string to be drawn.
	 * @param[in]	dx		Destination X. (center pos)
	 * @param[in]	dy		Destination Y. (center pos)
	 * @param[in]	color	Text color. (0xRRGGBB)
	 * @param[in]	ajustX	Ajust char-to-char space.
	 * @param[in]	scaleX	Scaling factor X.
	 * @param[in]	scaleY	Scaling factor Y.
	 * @param[in]	alpha	Alpha value.
	 * @param[out]	nextx	X of next column.
	 * @param[out]	nexty	Y of next row.
	 */
	void drawString(const FontResourcePtr &font, const wchar_t *str, int dx, int dy,
		uint32_t color = 0x000000, int ajustX = 0,
		float scaleX = 1.0f, float scaleY = 1.0f, float alpha = 1.0f,
		int *nextx = nullptr, int *nexty = nullptr);
	//@}

private:
	const DXGI_FORMAT BufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_SWAP_CHAIN_FLAG SwapChainFlag = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	const float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const size_t DrawListMax = 1024;		// not strict limit
	const wchar_t * const VS_FileName = L"@VertexShader.cso";
	const wchar_t * const PS_FileName = L"@PixelShader.cso";

	GraphicsParam m_param;
	util::ComPtr<ID3D11Device>				m_pDevice;
	util::ComPtr<ID3D11DeviceContext>		m_pContext;
	util::ComPtr<IDXGISwapChain>			m_pSwapChain;
	util::ComPtr<ID3D11RenderTargetView>	m_pRenderTargetView;
	util::ComPtr<ID3D11VertexShader>		m_pVertexShader;
	util::ComPtr<ID3D11PixelShader>			m_pPixelShader;
	util::ComPtr<ID3D11InputLayout>			m_pInputLayout;
	util::ComPtr<ID3D11Buffer>				m_pVertexBuffer;
	util::ComPtr<ID3D11Buffer>				m_pCBNeverChanges, m_pCBChanges;
	util::ComPtr<ID3D11RasterizerState>		m_pRasterizerState;
	util::ComPtr<ID3D11SamplerState>		m_pSamplerState;
	util::ComPtr<ID3D11BlendState>			m_pBlendState;

	std::vector<DrawTask> m_drawTaskList;

	void initializeD3D();
	void initBackBuffer();
};

}	// namespace graphics
}	// namespace yappy
