#pragma once

#include "util.h"
#include "graphics.h"
#include "sound.h"
#include "input.h"
#include <functional>

namespace yappy {
namespace framework {

using IdString = util::IdString;

template <class T>
struct Resource : private util::noncopyable {
	using PtrType = std::shared_ptr<T>;
	using LoadFuncType = std::function<PtrType()>;

	PtrType ptr;
	LoadFuncType loadFunc;

	explicit Resource(LoadFuncType loadFunc_) : loadFunc(loadFunc_) {}
	void load()
	{
		if (ptr == nullptr) {
			ptr = LoadFunc();
		}
	}
	void unload()
	{
		ptr.reset();
	}
};

/** @brief Resource manager, which is owned by Application.
* @details
*/
class ResourceManager : private util::noncopyable {
public:
	explicit ResourceManager(int resSetCount = 1) : m_texMap(resSetCount) {}
	~ResourceManager() = default;

	void addTexture(size_t setId, const char *resId,
		std::function<graphics::DGraphics::TextureResource()> loadFunc)
	{
		IdString fixedResId;
		util::createFixedString(&fixedResId, resId);
		auto &map = m_texMap.at(setId);
		if (map.count(fixedResId) != 0) {
			throw std::invalid_argument(std::string("Resource ID already exists : ") + resId);
		}
		map.emplace(std::piecewise_construct,
			// key = IdString(fixedResId)
			std::forward_as_tuple(fixedResId),
			// value = Resource(loadfunc)
			std::forward_as_tuple(loadFunc));
	}
	void addFont(size_t setId, const char *resId,
		std::function<graphics::DGraphics::FontResource()> loadFunc);
	void addSoundEffect(size_t setId, const char *resId,
		std::function<sound::XAudio2::SeResource> loadFunc);

	// auto static_cast(enum->size_t) sample
	template <class T>
	void addTexture(T setId, const char *resId,
		std::function<std::shared_ptr<graphics::Texture>()> loadFunc)
	{
		static_assert(std::is_enum<T>::value || std::is_integral<T>::value,
			"T must be enum or integral");
		addTexture(static_cast<size_t>(setId), resId, loadFunc);
	}

private:
	// int setId -> char[16] resId -> Resource<T>
	template <class T>
	using ResMap = std::vector<std::unordered_map<IdString, Resource<T>>>;

	ResMap<graphics::DGraphics::TextureResource::element_type> m_texMap;
	ResMap<graphics::DGraphics::FontResource::element_type>    m_fontMap;
	ResMap<sound::XAudio2::SeResource::element_type>           m_seMap;
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

	void addResource(size_t setId, const char *resId, const wchar_t *fileName)
	{
		std::wstring fileNameForCopy(fileName);
		m_resMgr.addTexture(setId, resId, [this, fileNameForCopy]() {
			return m_dg->loadTexture(fileNameForCopy.c_str());
		});
	}

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
