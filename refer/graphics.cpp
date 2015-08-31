#include "stdafx.h"
#include "graphics.hpp"
#include "debug.hpp"
#include <dwmapi.h>

#	pragma comment(lib, "d3d9.lib")
#ifdef _DEBUG
#	pragma comment(lib, "d3dx9d.lib")
#else
#	pragma comment(lib, "d3dx9.lib")
#endif

namespace {

	void SetCursorVisible(bool fullscreen) {
		if (fullscreen) {
			while(::ShowCursor(FALSE) >= 0);
		} else {
			while(::ShowCursor(TRUE) < 0);
		}
	}

}

namespace dx9lib {

	Timer::Timer()
	{
		LARGE_INTEGER freq;
		if (!::QueryPerformanceFrequency(&freq))
			Win32Error::throwError(_T("QueryPerformanceFrequency failed."));
		m_freq = freq.QuadPart;
	}

	const Timer & Timer::getInstance()
	{
		static Timer timer;
		return timer;
	}

	LONGLONG Timer::getMili() const
	{
		LARGE_INTEGER count;
		if (!::QueryPerformanceCounter(&count))
			Win32Error::throwError(_T("QueryPerformanceCounter failed."));
		return count.QuadPart * 1000 / m_freq;
	}

	LONGLONG Timer::getMicro() const
	{
		LARGE_INTEGER count;
		if (!::QueryPerformanceCounter(&count))
			Win32Error::throwError(_T("QueryPerformanceCounter failed."));
		return count.QuadPart * 1000000 / m_freq;
	}


	StopWatch::StopWatch()
	{
		m_prev = Timer::getInstance().getMicro();
	}

	StopWatch::~StopWatch()
	{
		debug.printf(_T("%fms\n"), (Timer::getInstance().getMicro() - m_prev) / 1e3);
	}


	FPSSync::FPSSync(int fps, int fpsUpdateFrame) :
		m_last(0), m_fpsUpdateFrame(fpsUpdateFrame),
		m_count(0), m_baseTime(0), m_fps(0.0f)
	{
		m_wait = 1000000 / fps;
		m_wait = m_wait - (m_wait % 1000);
		m_last = m_baseTime = Timer::getInstance().getMicro();
	}

	void FPSSync::frame()
	{
		const Timer &timer = Timer::getInstance();
		LONGLONG now;
		do {
			Sleep(0);
			now = timer.getMicro();
		} while (now - m_last < m_wait);
		m_last = now;
		
		m_count++;
		if (m_count >= m_fpsUpdateFrame) {
			m_fps = 1e6f * m_fpsUpdateFrame / (now - m_baseTime);
			m_baseTime = now;
			m_count = 0;
		}
	}

	float FPSSync::getFPS()
	{
		return m_fps;
	}


	Texture::Texture(shared_ptr<IDirect3DTexture9> pTexture, const D3DXIMAGE_INFO &info) :
	m_pTexture(pTexture)
	{
		m_orgw = info.Width;
		m_orgh = info.Height;
		D3DSURFACE_DESC desc;
		DGraphicsError::check(m_pTexture->GetLevelDesc(0, &desc));
		m_texw = desc.Width;
		m_texh = desc.Height;
	}

	shared_ptr<IDirect3DTexture9> Texture::getTexture() const
	{
		return m_pTexture;
	}

	void Texture::getSizeInfo(int &orgw, int &orgh, int &texw, int &texh) const
	{
		orgw = m_orgw;
		orgh = m_orgh;
		texw = m_texw;
		texh = m_texh;
	}


	DGraphics::DGraphics(HWND hwnd, bool fullscreen, bool vsync, bool fpupreserve, int w, int h) :
	m_sync(60, 60)
	{
		debug.println("Initializing Direct3D9...");

		IDirect3D9 *pD3d;
		pD3d = ::Direct3DCreate9(D3D_SDK_VERSION);
		if (pD3d == NULL)
			throw DGraphicsError(_T("Direct3DCreate9 failed."));
		m_pD3d.reset(pD3d, iunknown_deleter());

		// adapter disaplay format
		D3DDISPLAYMODE dispMode;
		m_pD3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dispMode);

		m_params.BackBufferWidth = w;
		m_params.BackBufferHeight = h;
		m_params.BackBufferFormat = dispMode.Format;
		m_params.BackBufferCount = 1;
		m_params.MultiSampleType = D3DMULTISAMPLE_NONE;
		m_params.MultiSampleQuality = 0;
		m_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_params.hDeviceWindow = hwnd;
		m_params.Windowed = !fullscreen;
		m_params.EnableAutoDepthStencil = FALSE;
		m_params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
		m_params.Flags = 0;
		m_params.FullScreen_RefreshRateInHz = 0;//fullscreen ? 60 : 0;
		m_params.PresentationInterval = vsync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;

		HRESULT hr;
		IDirect3DDevice9 *pDev;
		DWORD flags = fpupreserve ? D3DCREATE_FPU_PRESERVE : 0;
		debug.println(_T("D3DDEVTYPE_HAL - D3DCREATE_HARDWARE_VERTEXPROCESSING"));
		hr = m_pD3d->CreateDevice(
			D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
			flags | D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&m_params, &pDev
		);
		if (FAILED(hr)) {
			debug.println(_T("D3DDEVTYPE_HAL - D3DCREATE_SOFTWARE_VERTEXPROCESSING"));
			hr = m_pD3d->CreateDevice(
				D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
				flags | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&m_params, &pDev
			);
			if (FAILED(hr)) {
				debug.println(_T("D3DDEVTYPE_SW - D3DCREATE_HARDWARE_VERTEXPROCESSING"));
				hr = m_pD3d->CreateDevice(
					D3DADAPTER_DEFAULT, D3DDEVTYPE_SW, hwnd,
					flags | D3DCREATE_HARDWARE_VERTEXPROCESSING,
					&m_params, &pDev
				);
				if (FAILED(hr)) {
					debug.println("D3DDEVTYPE_SW - D3DCREATE_SOFTWARE_VERTEXPROCESSING");
					hr = m_pD3d->CreateDevice(
						D3DADAPTER_DEFAULT, D3DDEVTYPE_SW, hwnd,
						flags | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
						&m_params, &pDev
					);
					DGraphicsError::check(hr);
				}
			}
		}
		m_pDev.reset(pDev, iunknown_deleter());

		// TODO: device caps
		D3DCAPS9 caps;
		m_pDev->GetDeviceCaps(&caps);
		//debug.printf("%d\n", caps.TextureCaps & D3DPTEXTURECAPS_POW2);

		SetCursorVisible(fullscreen);

		CustomVertex v[4] = {
			{ 0.5f, -0.5f, 0.0f, 0xffffffff, 1.0f, 1.0f},
			{-0.5f, -0.5f, 0.0f, 0xffffffff, 0.0f, 1.0f},
			{ 0.5f,  0.5f, 0.0f, 0xffffffff, 1.0f, 0.0f},
			{-0.5f,  0.5f, 0.0f, 0xffffffff, 0.0f, 0.0f}
		};
		IDirect3DVertexBuffer9 *pTmpVertex;
		DGraphicsError::check(
			m_pDev->CreateVertexBuffer(
				sizeof(v), D3DUSAGE_WRITEONLY, CUSTOM_FVF, D3DPOOL_MANAGED, &pTmpVertex, NULL
			)
		);
		m_pVertex.reset(pTmpVertex, iunknown_deleter());
		void *pData;
		DGraphicsError::check(
			m_pVertex->Lock(0, 0, (void **)&pData, 0)
		);
		memcpy(pData, v, sizeof(v));
		DGraphicsError::check(
			m_pVertex->Unlock()
		);

		// font
		ID3DXFont *pFont;
		hr = D3DXCreateFont(
			m_pDev.get(), 32, 16, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE, NULL, &pFont);
		DGraphicsError::check(hr);
		m_pFont.reset(pFont, iunknown_deleter());

		debug.println("Initializing Direct3D9 OK.");
	}

	void DGraphics::reset(bool fullscreen)
	{
		debug.println("Reseting Device...");
		m_params.Windowed = !fullscreen;
		m_pFont->OnLostDevice();
		DGraphicsError::check(m_pDev->Reset(&m_params));
		m_pFont->OnResetDevice();
		SetCursorVisible(fullscreen);
		debug.println("Reseting Device OK.");
	}

	void DGraphics::toggleScreenMode()
	{
		// supress warning C4800
		reset(m_params.Windowed == TRUE);
	}

	void DGraphics::begin(D3DCOLOR clearColor)
	{
		clear(clearColor);
		DGraphicsError::check(m_pDev->BeginScene());

		// disable light
		m_pDev->SetRenderState(D3DRS_LIGHTING, FALSE);

		// projection matrix
		D3DVIEWPORT9 viewport;
		m_pDev->GetViewport(&viewport);
		D3DXMATRIX matProj;
		D3DXMatrixIdentity(&matProj);
		matProj._11 = 2.0f / viewport.Width;
		matProj._22 = 2.0f / viewport.Height;
		m_pDev->SetTransform(D3DTS_PROJECTION, &matProj);

		m_pDev->SetStreamSource(0, m_pVertex.get(), 0, sizeof(CustomVertex));
		m_pDev->SetFVF(CUSTOM_FVF);
		m_pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		m_pDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		m_pDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		m_pDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		m_pDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

/*		D3DXMATRIX matWorld;
		D3DXMatrixScaling(&matWorld, 320, 240, 1);
		m_pDev->SetTransform(D3DTS_WORLD, &matWorld);

		m_pDev->SetStreamSource(0, m_pVertex.get(), 0, sizeof(CustomVertex));
		m_pDev->SetFVF(CUSTOM_FVF);
		m_pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);*/

		/*
		RECT rect = {0, 0, 640, 480};
		m_pFont->DrawText(NULL, _T("This is test."), -1, &rect, DT_NOCLIP | DT_CENTER | DT_VCENTER, D3DCOLOR_XRGB(0,0,0));
		*/
	}

	// TODO handle device lost
	bool DGraphics::end() {
		DGraphicsError::check(m_pDev->EndScene());
		HRESULT hr;
		hr = m_pDev->Present(NULL, NULL, NULL, NULL);
		if (hr == D3DERR_DEVICELOST) {
			return false;
		} else {
			DGraphicsError::check(hr);
		}
		m_sync.frame();
		return true;
	}

	void DGraphics::clear(D3DCOLOR clearColor)
	{
		DGraphicsError::check(
			m_pDev->Clear(0, NULL, D3DCLEAR_TARGET, clearColor, 1.0f, 0));
	}

	void DGraphics::drawSprite(shared_ptr<const Texture> pTexture, int x, int y)
	{
		int orgw, orgh, texw, texh;
		pTexture->getSizeInfo(orgw, orgh, texw, texh);

		D3DVIEWPORT9 viewport;
		m_pDev->GetViewport(&viewport);
		D3DXMATRIX matWorld, matTrans;
		D3DXMatrixScaling(&matWorld, static_cast<float>(texw), static_cast<float>(texh), 1.0f);
		D3DXMatrixTranslation(&matTrans, viewport.Width / -2.0f, viewport.Height / 2.0f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		D3DXMatrixTranslation(&matTrans, (orgw - texw) / -2.0f, (orgh - texh) / 2.0f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		D3DXMatrixTranslation(&matTrans, orgw / 2.0f, orgh / -2.0f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		D3DXMatrixTranslation(&matTrans, static_cast<float>(x), static_cast<float>(-y), 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		D3DXMatrixTranslation(&matTrans, -0.5f, -0.5f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		m_pDev->SetTransform(D3DTS_WORLD, &matWorld);

		m_pDev->SetTexture(0, pTexture->getTexture().get());
		m_pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	}

	void DGraphics::drawSpriteByCenter(shared_ptr<const Texture> pTexture, int x, int y, float rot)
	{
		int orgw, orgh, texw, texh;
		pTexture->getSizeInfo(orgw, orgh, texw, texh);

		D3DVIEWPORT9 viewport;
		m_pDev->GetViewport(&viewport);
		D3DXMATRIX matWorld, matTrans, matRot;
		D3DXMatrixScaling(&matWorld, static_cast<float>(texw), static_cast<float>(texh), 1.0f);
		D3DXMatrixTranslation(&matTrans, (orgw - texw) / -2.0f, (orgh - texh) / 2.0f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		if (abs(rot) > 1e-3) {
			D3DXMatrixRotationZ(&matRot, rot);
			D3DXMatrixMultiply(&matWorld, &matWorld, &matRot);
		}
		D3DXMatrixTranslation(&matTrans, viewport.Width / -2.0f, viewport.Height / 2.0f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		D3DXMatrixTranslation(&matTrans, static_cast<float>(x), static_cast<float>(-y), 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		D3DXMatrixTranslation(&matTrans, -0.5f, -0.5f, 0.0f);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matTrans);
		m_pDev->SetTransform(D3DTS_WORLD, &matWorld);

		m_pDev->SetTexture(0, pTexture->getTexture().get());
		m_pDev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	}

	shared_ptr<Texture> DGraphics::createTexture(ResourceLoader &loader, const tstring &fileName)
	{
		std::vector<BYTE> src;
		loader.readFile(src, fileName);

		IDirect3DTexture9 *pTmpTex;
		D3DXIMAGE_INFO imageInfo = {0};
		DGraphicsError::check(
			D3DXCreateTextureFromFileInMemoryEx(
				m_pDev.get(), &src.at(0), src.size(), 0, 0, 1, 0, /*m_displayFormat*/D3DFMT_UNKNOWN,
				D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0,
				&imageInfo, NULL, &pTmpTex));
		shared_ptr<IDirect3DTexture9> pTexture(pTmpTex, iunknown_deleter());
		shared_ptr<Texture> ret(new Texture(pTexture, imageInfo));
		return ret;
	}

	float DGraphics::getFPS()
	{
		return m_sync.getFPS();
	}


	void DisableAero()
	{
		typedef decltype(DwmEnableComposition) *FuncType;
		HMODULE hDwm =  ::LoadLibrary(_T("dwmapi.dll"));
		if (hDwm != NULL) {
			debug.println(_T("dwmapi.dll found."));
			FuncType pf = (FuncType)::GetProcAddress(hDwm, "DwmEnableComposition");
			if (pf != NULL) {
				HRESULT hr = pf(DWM_EC_DISABLECOMPOSITION);
				if (SUCCEEDED(hr)) {
					debug.println(_T("Composition disabled."));
				} else {
					debug.println(_T("Warning: DwmEnableComposition() failed"));
				}
			} else {
				debug.println(_T("Warning: DwmEnableComposition() not found"));
			}
			::FreeLibrary(hDwm);
		} else {
			debug.println(_T("dwmapi.dll not found."));
			debug.println(_T("(probably the version is XP or earlier)"));
		}
	}

}
