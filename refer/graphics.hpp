#pragma once

#include "file.hpp"
#include "util.hpp"
#include "exceptions.hpp"
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>

namespace dx9lib {

	class Timer : private Uncopyable {
	private:
		Timer();
		Timer(const Timer &);
		~Timer() {}
		Timer & operator =(const Timer &);

		LONGLONG m_freq;
	public:
		static const Timer &getInstance();
		LONGLONG getMili() const;
		LONGLONG getMicro() const;
	};

	class StopWatch : private Uncopyable {
	private:
		LONGLONG m_prev;
	public:
		StopWatch();
		~StopWatch();
	};

	class FPSSync : private Uncopyable {
	private:
		LONGLONG m_wait;
		LONGLONG m_last;
		int m_fpsUpdateFrame;
		int m_count;
		LONGLONG m_baseTime;
		float m_fps;
	public:
		FPSSync(int fps = 60, int fpsUpdateFrame = 60);
		void frame();
		float getFPS();
	};

	class Texture : private Uncopyable {
	private:
		shared_ptr<IDirect3DTexture9> m_pTexture;
		int m_orgw, m_orgh, m_texw, m_texh;
	public:
		Texture(shared_ptr<IDirect3DTexture9> pTexture, const D3DXIMAGE_INFO &info);
		shared_ptr<IDirect3DTexture9> getTexture() const;
		void getSizeInfo(int &orgw, int &orgh, int &texw, int &texh) const;
	};

	class DGraphics : private Uncopyable {
	private:
		shared_ptr<IDirect3D9> m_pD3d;
		shared_ptr<IDirect3DDevice9> m_pDev;
		D3DPRESENT_PARAMETERS m_params;
		//D3DFORMAT m_displayFormat;
		shared_ptr<ID3DXFont> m_pFont;
		FPSSync m_sync;

		static const DWORD CUSTOM_FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;
		struct CustomVertex {
			float x, y, z;
			DWORD color;
			float u, v;
		};
		shared_ptr<IDirect3DVertexBuffer9> m_pVertex;
	public:
		DGraphics(HWND hwnd, bool fullscreen, bool vsync, bool fpupreserve, int w, int h);
		void reset(bool fullscreen);
		void toggleScreenMode();

		void begin(D3DCOLOR clearColor);
		// returns false if device lost
		bool end();

		void clear(D3DCOLOR clearColor);
		void drawSprite(shared_ptr<const Texture> pTexture, int x, int y);
		void drawSpriteByCenter(shared_ptr<const Texture> pTexture, int x, int y, float rot = 0.0f);
		shared_ptr<Texture> createTexture(ResourceLoader &loader, const tstring &fileName);

		float getFPS();
	};

	void DisableAero();

}
