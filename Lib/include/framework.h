#pragma once

#include "util.h"
#include "debug.h"
#include "graphics.h"
#include "sound.h"
#include "input.h"
#include <functional>

namespace yappy {
namespace framework {

using IdString = util::IdString;

template <class T>
class Resource : private util::noncopyable {
public:
	using PtrType = std::shared_ptr<T>;
	using LoadFuncType = std::function<PtrType()>;

	explicit Resource(LoadFuncType loadFunc_) : m_loadFunc(loadFunc_) {}
	const PtrType &getPtr() const
	{
		return m_ptr;
	}
	void load()
	{
		if (m_ptr == nullptr) {
			m_ptr = m_loadFunc();
		}
	}
	void unload()
	{
		m_ptr.reset();
	}

private:
	PtrType m_ptr;
	LoadFuncType m_loadFunc;
};

/** @brief Resource manager, which is owned by Application.
* @details
*/
class ResourceManager : private util::noncopyable {
public:
	explicit ResourceManager(size_t resSetCount = 1);
	~ResourceManager() = default;

	void addTexture(size_t setId, const char *resId,
		std::function<graphics::DGraphics::TextureResourcePtr()> loadFunc);
	void addFont(size_t setId, const char *resId,
		std::function<graphics::DGraphics::FontResourcePtr()> loadFunc);
	void addSoundEffect(size_t setId, const char *resId,
		std::function<sound::XAudio2::SeResourcePtr()> loadFunc);

	void loadResourceSet(size_t setId);
	void unloadResourceSet(size_t setId);

	const graphics::DGraphics::TextureResourcePtr &getTexture(
		size_t setId, const char *resId);
	const graphics::DGraphics::FontResourcePtr &getFont(
		size_t setId, const char *resId);
	const sound::XAudio2::SeResourcePtr &getSoundEffect(
		size_t setId, const char *resId);

private:
	// int setId -> char[16] resId -> Resource<T>
	template <class T>
	using ResMapVec = std::vector<std::unordered_map<IdString, Resource<T>>>;

	ResMapVec<graphics::DGraphics::TextureResource> m_texMapVec;
	ResMapVec<graphics::DGraphics::FontResource>    m_fontMapVec;
	ResMapVec<sound::XAudio2::SeResource>           m_seMapVec;
};


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
	sound::XAudio2 &sound() { return *m_ds.get(); }
	input::DInput &input() { return *m_di.get(); }

	void addTextureResource(size_t setId, const char *resId, const wchar_t *path);
	void addFontResource(size_t setId, const char *resId,
		const wchar_t *fontName, uint32_t startChar, uint32_t endChar,
		uint32_t w, uint32_t h);
	void addSeResource(size_t setId, const char *resId, const wchar_t *path);
	void loadResourceSet(size_t setId);
	void unloadResourceSet(size_t setId);
	const graphics::DGraphics::TextureResourcePtr &getTexture(
		size_t setId, const char *resId);
	const graphics::DGraphics::FontResourcePtr &getFont(
		size_t setId, const char *resId);
	const sound::XAudio2::SeResourcePtr &getSoundEffect(
		size_t setId, const char *resId);

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
	ResourceManager m_resMgr;

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
