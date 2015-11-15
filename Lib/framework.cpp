#include "stdafx.h"
#include "include/framework.h"
#include "include/debug.h"
#include "include/exceptions.h"

namespace yappy {
namespace framework {

using error::checkWin32Result;

///////////////////////////////////////////////////////////////////////////////
// class FrameControl impl
///////////////////////////////////////////////////////////////////////////////
#pragma region FrameControl

namespace {

inline int64_t getTimeCounter()
{
	LARGE_INTEGER cur;
	BOOL b = ::QueryPerformanceCounter(&cur);
	checkWin32Result(b != 0, "QueryPerformanceCounter() failed");
	return cur.QuadPart;
}

}	// namespace

FrameControl::FrameControl(uint32_t fps, uint32_t skipCount) :
	m_skipCount(skipCount),
	m_fpsPeriod(fps)
{
	// counter/sec
	LARGE_INTEGER freq;
	BOOL b = ::QueryPerformanceFrequency(&freq);
	checkWin32Result(b != FALSE, "QueryPerformanceFrequency() failed");
	m_freq = freq.QuadPart;

	// counter/frame = (counter/sec) / (frame/sec)
	//               = m_freq / fps
	m_counterPerFrame = m_freq / fps;
}

bool FrameControl::shouldSkipFrame()
{
	return m_frameCount != 0;
}

void FrameControl::endFrame()
{
	// for fps calc
	if (!shouldSkipFrame()) {
		m_fpsFrameAcc++;
	}

	int64_t target = m_base + m_counterPerFrame;
	int64_t cur = getTimeCounter();
	if (m_base == 0) {
		// force OK
		m_base = cur;
	}
	else if (cur < target) {
		// OK, wait for next frame
		while (cur < target) {
			::Sleep(1);
			cur = getTimeCounter();
		}
		// may overrun a little bit, add it to next frame
		m_base = target;
	}
	else {
		// too late, probably immediately right after vsync
		m_base = cur;
	}

	// m_frameCount++;
	// m_frameCount %= (#draw(=1) + #skip)
	m_frameCount = (m_frameCount + 1) % (1 + m_skipCount);

	// for fps calc
	m_fpsCount++;
	if (m_fpsCount >= m_fpsPeriod) {
		double sec = static_cast<double>(cur - m_fpsBase) / m_freq;
		m_fps = m_fpsFrameAcc / sec;

		m_fpsCount = 0;
		m_fpsFrameAcc = 0;
		m_fpsBase = cur;
	}
}

double FrameControl::getFramePerSec()
{
	return m_fps;
}

#pragma endregion

///////////////////////////////////////////////////////////////////////////////
// class Application impl
///////////////////////////////////////////////////////////////////////////////
#pragma region Application

Application::Application(const AppParam &appParam, const graphics::GraphicsParam &graphParam):
	m_param(appParam),
	m_graphParam(graphParam),
	m_frameCtrl(graphParam.refreshRate, appParam.frameSkip)
{
	// window
	initializeWindow();
	m_graphParam.hWnd = m_hWnd.get();

	// DirectGraphics
	auto *tmpDg = new graphics::DGraphics(m_graphParam);
	m_dg.reset(tmpDg);
	// XAudio2
	auto *tmpXa2 = new sound::XAudio2();
	m_ds.reset(tmpXa2);
	// DirectInput
	auto *tmpDi = new input::DInput(m_param.hInstance, m_hWnd.get());
	m_di.reset(tmpDi);
}

void Application::initializeWindow()
{
	debug::writeLine(L"Initializing window...");

	// Register class
	WNDCLASSEX cls = { 0 };
	cls.cbSize = sizeof(WNDCLASSEX);
	cls.style = 0;
	cls.lpfnWndProc = wndProc;
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = m_param.hInstance;
	cls.hIcon = m_param.hIcon;
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	cls.lpszMenuName = nullptr;
	cls.lpszClassName = m_param.wndClsName;
	cls.hIconSm = m_param.hIconSm;
	checkWin32Result(::RegisterClassEx(&cls) != 0, "RegisterClassEx() failed");

	const DWORD wndStyle = WS_OVERLAPPEDWINDOW & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX;

	RECT rc = { 0, 0, m_graphParam.w, m_graphParam.h };
	checkWin32Result(::AdjustWindowRect(&rc, wndStyle, FALSE) != FALSE,
		"AdjustWindowRect() failed");

	HWND hWnd = ::CreateWindow(m_param.wndClsName, m_param.title, wndStyle,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		nullptr, nullptr, m_param.hInstance, this);
	checkWin32Result(hWnd != nullptr, "CreateWindow() failed");
	m_hWnd.reset(hWnd);

	// for frame processing while window drugging etc.
	::SetTimer(m_hWnd.get(), TimerEventId, 1, nullptr);

	// Show cursor
	if (m_param.showCursor) {
		while (::ShowCursor(TRUE) < 0);
	}
	else {
		while (::ShowCursor(FALSE) >= 0);
	}

	debug::writeLine(L"Initializing window OK");
}

Application::~Application()
{
	debug::writeLine(L"Finalize Application Window");
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
	return m_dg->onSize(hWnd, msg, wParam, lParam);
}

// main work
// update(), then draw() if FrameControl allowed
void Application::onIdle()
{
	updateInternal();
	if (!m_frameCtrl.shouldSkipFrame()) {
		renderInternal();
	}
	m_frameCtrl.endFrame();

	// fps
	wchar_t buf[256] = { 0 };
	swprintf_s(buf, L"%s fps=%.2f (%d)", m_param.title,
		m_frameCtrl.getFramePerSec(), m_param.frameSkip);
	::SetWindowText(m_hWnd.get(), buf);
}

void Application::updateInternal()
{
	// Call user code
	update();
}

void Application::renderInternal()
{
	// TEST: frame skip
	// if skip == 3, draw once per 4F
	// So fps should be around 15.00
	/*
	uint32_t skip = 3;
	::Sleep(17 * skip);
	//*/

	// Call user code
	render();

	m_dg->render();
}

int Application::run()
{
	// Call user code
	init();

	::ShowWindow(m_hWnd.get(), m_param.nCmdShow);
	::UpdateWindow(m_hWnd.get());

	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else {
			// main work
			onIdle();
		}
	}
	return static_cast<int>(msg.wParam);
}

#pragma endregion

}
}
