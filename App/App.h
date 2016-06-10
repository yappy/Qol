#pragma once

#include <framework.h>
#include <config.h>
#include <script.h>

using namespace yappy;

// global
extern config::ConfigFile g_config;

enum class SceneId {
	Main,
	Sub,
	Count
};
const size_t SceneCount = static_cast<size_t>(SceneId::Count);

struct ResSetId {
	ResSetId() = delete;
	enum {
		Common,
		Main,
		Sub,
		Count
	};
};

class MyApp : public framework::Application {
public:
	MyApp(const framework::AppParam &appParam,
		const graphics::GraphicsParam &graphParam);
	~MyApp() = default;

	framework::scene::SceneBase *getScene(SceneId id);
	template <class T>
	T *getSceneAs(SceneId id)
	{
		return static_cast<T *>(getScene(id));
	}
	void setNextScene(SceneId id);

protected:
	void init() override;
	void update() override;
	void render() override;

private:
	// scene instance array
	std::array<std::unique_ptr<framework::scene::SceneBase>, SceneCount> m_scenes;
	// current scene
	framework::scene::SceneBase *m_pCurrentScene = nullptr;
	// next scene (no change if nullptr)
	framework::scene::SceneBase *m_pNextScene = nullptr;
};

class MainScene : public framework::scene::AsyncLoadScene {
public:
	MainScene(MyApp *app, bool luaDebug);
	~MainScene() = default;

	// Scene specific initialization at scene start
	void setup();
	void updateOnMainThread() override;
	void render() override;

protected:
	void loadOnSubThread(std::atomic_bool &cancel) override;

private:
	// 4MiB
	static const size_t LuaHeapSize = 4 * 1024 * 1024;
	// instruction execution limit
	static const int LuaInstLimit = 100000;

	MyApp *m_app;
	// The first update() call after setup()
	bool m_luaStartCalled = false;
	int m_speed = 1;
	// nullptr indicates lua error state
	std::unique_ptr<lua::Lua> m_lua;
	bool m_luaDebug;

	void initializeLua();
	void reloadLua();
	void luaErrorHandler(const lua::LuaError &err, const wchar_t *msg);
};

class SubScene : public framework::scene::SceneBase {
public:
	explicit SubScene(MyApp *app);
	~SubScene() = default;

	// Scene specific initialization at scene start
	void setup();
	void update() override;
	void render() override;

private:
	MyApp *m_app;
	input::DInput::KeyData m_keyData;
	int m_supressSeFrame;
};
