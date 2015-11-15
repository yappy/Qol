#pragma once

#include "util.h"
#include "graphics.h"
#include "sound.h"
#include "input.h"

namespace yappy {
namespace framework {

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

struct AppParam {
	HINSTANCE hInstance = nullptr;
	int nCmdShow = SW_SHOW;
	const wchar_t *wndClsName = L"GameWndCls";
	const wchar_t *title = L"GameApp";
	HICON hIcon = nullptr;
	HICON hIconSm = nullptr;
	uint32_t frameSkip = 0;
	bool showCursor = false;
};

/** @brief User application base, managing window.
* @details Please inherit this and override protected methods.
*/
class Application : private util::noncopyable {
public:

	Application(const AppParam &appParam, const graphics::GraphicsParam &graphParam);
	virtual ~Application();
	int run();

	HWND getHWnd() { return m_hWnd.get(); }
	graphics::DGraphics &graph() { return *m_dg.get(); }

protected:
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void render() = 0;

private:
	const UINT_PTR TimerEventId = 0xffff0001;

	using HWndPtr = std::unique_ptr<HWND, hwndDeleter>;
	HWndPtr m_hWnd;
	std::unique_ptr<graphics::DGraphics> m_dg;
	std::unique_ptr<sound::XAudio2> m_ds;
	std::unique_ptr<input::DInput> m_di;

	AppParam m_param;
	graphics::GraphicsParam m_graphParam;
	FrameControl m_frameCtrl;

	void initializeWindow();
	static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT onSize(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void onIdle();
	void updateInternal();
	void renderInternal();
};

}
}
