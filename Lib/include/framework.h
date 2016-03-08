/** @mainpage
 * @section はじめに
 * これは、QOL(生活の質)を高めるためのライブラリです。
 * @section つぎに
 * @ref framework.h の Application クラスとか。
 * @ref debug.h , @ref util.h もおすすめです。
 * @section おわりに
 * ホワイト。
 */

/** @file
 * @brief Game application main framework classes.
 */

#pragma once

#include "util.h"
#include "debug.h"
#include "graphics.h"
#include "sound.h"
#include "input.h"
#include <atomic>
#include <future>
#include <functional>

namespace yappy {
/// Game application main framework.
namespace framework {

/** @brief Command line utility.
* @return Parsed result vector. (argc-argv compatible)
*/
std::vector<std::wstring> parseCommandLine();

/// %Resource ID is fixed-length string; char[16].
using IdString = util::IdString;

template <class T>
class Resource : private util::noncopyable {
public:
	using PtrType = std::shared_ptr<T>;
	using LoadFuncType = std::function<PtrType()>;

	explicit Resource(LoadFuncType loadFunc_) : m_loadFunc(loadFunc_) {}
	~Resource() = default;

	const PtrType getPtr() const
	{
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_resPtr == nullptr) {
			throw std::runtime_error("Resource not loaded");
		}
		return m_resPtr;
	}
	void load()
	{
		{
			std::lock_guard<std::mutex> lock(m_lock);
			if (m_resPtr != nullptr) {
				return;
			}
			m_loading = true;
		}
		auto res = m_loadFunc();
		{
			std::lock_guard<std::mutex> lock(m_lock);
			if (!m_loading) {
				throw std::runtime_error("Multiple load async detected");
			}
			m_loading = false;
			m_resPtr = res;
		}
	}
	void unload() {
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_loading) {
			throw std::runtime_error("Unload resource while loading");
		}
		m_resPtr.reset();
	}

private:
	mutable std::mutex m_lock;
	bool m_loading = false;
	PtrType m_resPtr;
	LoadFuncType m_loadFunc;
};


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

	void loadResourceSet(size_t setId, std::atomic_bool &cancel);
	void unloadResourceSet(size_t setId);

	const graphics::DGraphics::TextureResourcePtr getTexture(
		size_t setId, const char *resId);
	const graphics::DGraphics::FontResourcePtr getFont(
		size_t setId, const char *resId);
	const sound::XAudio2::SeResourcePtr getSoundEffect(
		size_t setId, const char *resId);

private:
	// int setId -> char[16] resId -> Resource<T>
	template <class T>
	using ResMapVec = std::vector<std::unordered_map<IdString, Resource<T>>>;

	ResMapVec<graphics::DGraphics::TextureResource> m_texMapVec;
	ResMapVec<graphics::DGraphics::FontResource>    m_fontMapVec;
	ResMapVec<sound::XAudio2::SeResource>           m_seMapVec;
};


class SceneBase : private util::noncopyable {
public:
	SceneBase() = default;
	virtual ~SceneBase()
	{
		// set cancel flag
		m_cancel.store(true);
		// m_future destructor will wait for sub thread
	}

	void startLoadThread()
	{
		// move assign
		m_future = std::async(std::launch::async, [this]() {
			// can throw an exception
			loadOnSubThread(m_cancel);
		});
	}
	void updateLoadStatus()
	{
		if (m_future.valid()) {
			auto status = m_future.wait_for(std::chrono::seconds(0));
			switch (status) {
			case std::future_status::ready:
				// complete or exception
				// make m_future invalid
				// if an exception is thrown in sub thread, throw it
				m_future.get();
				// load complete event
				onLoadComplete();
				break;
			case std::future_status::timeout:
				// not yet
				break;
			default:
				ASSERT(false);
			}
		}
	}
	bool isLoadCompleted()
	{
		return !m_future.valid();
	}

	virtual void update() = 0;
	virtual void render() = 0;

protected:
	virtual void loadOnSubThread(std::atomic_bool &cancel) = 0;
	virtual void onLoadComplete() = 0;

private:
	std::atomic_bool m_cancel = false;
	std::future<void> m_future;
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

/** @brief Application parameters.
 * @details Each field has default value.
 */
struct AppParam {
	/// hInstance from WinMain.
	HINSTANCE hInstance = nullptr;
	/// nCmdShow from WinMain.
	int nCmdShow = SW_SHOW;
	/// Window class name.
	const wchar_t *wndClsName = L"GameWndCls";
	/// Window title string.
	const wchar_t *title = L"GameApp";
	/// hIcon for window class.
	HICON hIcon = nullptr;
	/// hIconSm for window class.
	HICON hIconSm = nullptr;
	/// Frame skip count.
	uint32_t frameSkip = 0;
	/// Whether shows cursor or not.
	bool showCursor = false;
};

/** @brief User application base, which manages a window and DirectX objects.
* @details Please inherit this class and override protected methods.
*/
class Application : private util::noncopyable {
public:
	/** @brief Constructor.
	 * @details This needs AppParam and graphics::GraphicsParam.
	 * @param[in] appParam		%Application parameters.
	 * @param[in] graphParam	Graphics parameters.
	 */
	Application(const AppParam &appParam, const graphics::GraphicsParam &graphParam);
	/** @brief Destructor.
	 */
	virtual ~Application();
	/** @brief Start main loop.
	 * @details Call init() .
	 * @return msg.wParam, which should be returned from WinMain().
	 */
	int run();

	/** @brief Get HWND of the window managed by this class.
	 */
	HWND getHWnd() { return m_hWnd.get(); }
	/** @brief Get DirectGraphics manager.
	 */
	graphics::DGraphics &graph() { return *m_dg.get(); }
	/** @brief Get XAudio2 manager.
	 */
	sound::XAudio2 &sound() { return *m_ds.get(); }
	/** @brief Get DirectInput manager.
	 */
	input::DInput &input() { return *m_di.get(); }

	/** @brief Register texture image resource.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 * @param[in] path	File path.
	 */
	void addTextureResource(size_t setId, const char *resId, const wchar_t *path);
	/** @brief Register font image resource.
	 * @param[in] setId		%Resource set ID.
	 * @param[in] resId		%Resource ID.
	 * @param[in] fontName	Font family name.
	 * @param[in] startChar	The first character.
	 * @param[in] endChar	The last character.
	 * @param[in] w			Font image width.
	 * @param[in] h			Font image height.
	 */
	void addFontResource(size_t setId, const char *resId,
		const wchar_t *fontName, uint32_t startChar, uint32_t endChar,
		uint32_t w, uint32_t h);
	/** @brief Register sound effect resource.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 * @param[in] path	File path.
	 */
	void addSeResource(size_t setId, const char *resId, const wchar_t *path);

	/** @brief Load resources by resource set ID.
	 * @param[in] setId	%Resource set ID.
	 */
	void loadResourceSet(size_t setId, std::atomic_bool &cancel);
	/** @brief Unload resources by resource set ID.
	 * @param[in] setId	%Resource set ID.
	 */
	void unloadResourceSet(size_t setId);

	/** @brief Get texture resource pointer.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 */
	const graphics::DGraphics::TextureResourcePtr getTexture(
		size_t setId, const char *resId);
	/** @brief Get texture resource pointer.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 */
	const graphics::DGraphics::FontResourcePtr getFont(
		size_t setId, const char *resId);
	/** @brief Get texture resource pointer.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 */
	const sound::XAudio2::SeResourcePtr getSoundEffect(
		size_t setId, const char *resId);

protected:
	/** @brief User initialization code.
	 * @details Called at the beginning of run().
	 */
	virtual void init() = 0;
	/** @brief Process frame.
	 * @details Called at the beginning of every frame.
	 */
	virtual void update() = 0;
	/** @brief Process rendering.
	 * @details Called after update(), unless frame skipping.
	 */
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
