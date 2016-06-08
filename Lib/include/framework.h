/** @mainpage
 * @section はじめに
 * これは、QOL(生活の質)を高めるためのライブラリです。
 * @section つぎに
 * @ref framework.h の Application クラスとか。
 * @ref debug.h , @ref util.h もおすすめです。
 * Lua へエクスポートされている関数仕様は @ref yappy::lua::export にあります。
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

using error::throwTrace;
using error::FrameworkError;

/// Random number framework.
namespace random {

/** @brief Generate nondeterministic random number for seed.
 * @details
 * Uses std::random_device.
 * Probably uses win32 CryptGenRandom().
 * This function will be slow.
 * @return random number
 */
unsigned int generateRandomSeed();

/** @brief Set random seed.
 * @param[in]	seed	Random seed.
 * @sa @ref generateRandomSeed()
 */
void setSeed(unsigned int seed);

/** @brief Get next uint32 random number.
 * @return [0x00000000, 0xffffffff]
 */
unsigned int nextRawUInt32();

/** @brief Get next int random number.
 * @param[in]	a	min value (inclusive)
 * @param[in]	b	max value (inclusive)
 * @return [a, b]
 */
int nextInt(int a = 0, int b = std::numeric_limits<int>::max());

/** @brief Get next double random number.
 * @param[in]	a	min value (inclusive)
 * @param[in]	b	max value (exclusive)
 * @return [a, b)
 */
double nextDouble(double a = 0.0, double max = 1.0);

}	// namespace random

/// Scene class.
namespace scene {

/** @brief Simple scene class base.
 * @details Noncopyable, and has common functions.
 */
class SceneBase : private util::noncopyable {
public:
	/// Constructor (default).
	SceneBase() = default;
	/// Virtual Destructor (default).
	virtual ~SceneBase() = default;

	/// Frame update.
	virtual void update() = 0;
	/// Rendering.
	virtual void render() = 0;
};

/** @brief Scene class with an async loading task.
*/
class AsyncLoadScene : public SceneBase {
public:
	/// Constructor (default).
	AsyncLoadScene() = default;
	/** @brief Destructor.
	 * @details
	 * If async task is being processed, sets cancel flag to true and
	 * blocks until task function will return.
	 */
	virtual ~AsyncLoadScene() override;
	/** @brief Check the state of sub thread and then call @ref updateOnMainThread().
	 */
	virtual void update() final override;

protected:
	/** @brief User-defined async task.
	 * @details
	 * This function will run on a separated thread and
	 * can take a long time to complete.
	 * If this class object is destructed while running,
	 * cancel flag which is passed by parameter will set to be true.
	 * @param[in]	cancel	Cancel signal.
	 */
	virtual void loadOnSubThread(std::atomic_bool &cancel) = 0;
	/** @brief Called in @ref update()
	 */
	virtual void updateOnMainThread() = 0;

	/** @brief Starts async load task on another thread.
	 * @pre Async task is not running. (@ref isLoading() returns false.)
	 */
	void startLoadThread();
	/** @brief Poll the state of the async task.
	 * @ref updateLoadStatus() is needed.
	 */
	bool isLoading() const;

private:
	std::atomic_bool m_cancel = false;
	std::future<void> m_future;

	void updateLoadStatus();
};

}	// namespace scene


/** @brief Command line utility.
 * @return Parsed result vector. (argc-argv compatible)
 */
std::vector<std::wstring> parseCommandLine();

/** @brief Get key state by GetAsyncKeyState().
 * @param[in]	vKey	VK_XXX
 * @return		true if the key is pressed.
 */
inline bool keyPressedAsync(int vKey)
{
	short ret = ::GetAsyncKeyState(vKey);
	return (ret & 0x8000) != 0;
}


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
			throwTrace<FrameworkError>("Resource not loaded");
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
				throwTrace<FrameworkError>("Multiple load async detected");
			}
			m_loading = false;
			m_resPtr = res;
		}
	}
	void unload() {
		std::lock_guard<std::mutex> lock(m_lock);
		if (!m_loading) {
			throwTrace<FrameworkError>("Unload resource while loading");
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
	void addBgm(size_t setId, const char *resId,
		std::function<sound::XAudio2::BgmResourcePtr()> loadFunc);

	void setSealed(bool sealed);
	bool isSealed();

	void loadResourceSet(size_t setId, std::atomic_bool &cancel);
	void unloadResourceSet(size_t setId);

	const graphics::DGraphics::TextureResourcePtr getTexture(
		size_t setId, const char *resId) const;
	const graphics::DGraphics::FontResourcePtr getFont(
		size_t setId, const char *resId) const;
	const sound::XAudio2::SeResourcePtr getSoundEffect(
		size_t setId, const char *resId) const;
	const sound::XAudio2::BgmResourcePtr getBgm(
		size_t setId, const char *resId) const;

private:
	bool m_sealed = true;

	// int setId -> char[16] resId -> Resource<T>
	template <class T>
	using ResMapVec = std::vector<std::unordered_map<IdString, Resource<T>>>;

	ResMapVec<graphics::DGraphics::TextureResource>	m_texMapVec;
	ResMapVec<graphics::DGraphics::FontResource>	m_fontMapVec;
	ResMapVec<sound::XAudio2::SeResource>			m_seMapVec;
	ResMapVec<sound::XAudio2::BgmResource>			m_bgmMapVec;
};

class FrameControl : private util::noncopyable {
public:
	FrameControl(uint32_t fps, uint32_t skipCount);
	~FrameControl() = default;
	bool shouldSkipFrame() const;
	void endFrame();
	double getFramePerSec() const;

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
 * @details Each field has a default value.
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
	 * Valid resource serId is: 0 .. resSetCount-1.
	 * @param[in] appParam		%Application parameters.
	 * @param[in] graphParam	Graphics parameters.
	 * @param[in] resSetCount	Count of resource set.
	 */
	Application(const AppParam &appParam, const graphics::GraphicsParam &graphParam,
		size_t resSetCount);
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
	HWND getHWnd() { return m_hWnd; }
	/** @brief Get graphics parameters.
	 */
	void getGraphicsParam(graphics::GraphicsParam *param) { *param = m_graphParam; }
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
	/** @brief Register BGM resource.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 * @param[in] path	File path.
	 */
	void addBgmResource(size_t setId, const char *resId, const wchar_t *path);

	/** @brief Set the lock state of resources.
	 * @details If resource manager is sealed, addXXXResource() will be failed.
	 * @param[in] seal	new state.
	 * @sa @ref UnsealResource
	 */
	void sealResource(bool seal);

	/** @brief Load resources by resource set ID.
	 * @details This function blocks until all resource is loaded.
	 * Processing can be cancelled by writing true to cancel from another thread.
	 * @param[in] setId		%Resource set ID.
	 * @param[in] cancel	async cancel atomic
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
		size_t setId, const char *resId) const;
	/** @brief Get font resource pointer.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 */
	const graphics::DGraphics::FontResourcePtr getFont(
		size_t setId, const char *resId) const;
	/** @brief Get sound effect resource pointer.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 */
	const sound::XAudio2::SeResourcePtr getSoundEffect(
		size_t setId, const char *resId) const;
	/** @brief Get BGM resource pointer.
	 * @param[in] setId	%Resource set ID.
	 * @param[in] resId	%Resource ID.
	 */
	const sound::XAudio2::BgmResourcePtr getBgm(
		size_t setId, const char *resId) const;

protected:
	/** @brief User initialization code.
	 * @details Called at the beginning of @ref run().
	 */
	virtual void init() = 0;
	/** @brief Process frame.
	 * @details Called at the beginning of every frame.
	 */
	virtual void update() = 0;
	/** @brief Process rendering.
	 * @details Called after @ref update(), unless frame skipping.
	 */
	virtual void render() = 0;

private:
	const UINT_PTR TimerEventId = 0xffff0001;

	HWND m_hWnd;
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

/** @brief Auto re-seal helper
 * @sa @ref Application::sealResource()
 */
class UnsealResource : util::noncopyable {
public:
	explicit UnsealResource(Application &app) : m_app(app)
	{
		m_app.sealResource(false);
	}
	~UnsealResource()
	{
		m_app.sealResource(true);
	}

private:
	Application &m_app;
};

}	// namespace framework
}	// namespace yappy
