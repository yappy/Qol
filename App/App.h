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

struct ResSetId {
	ResSetId() = delete;
	enum {
		Common,
		Main,
		Count
	};
};

class MyApp : public framework::Application {
public:
	MyApp(const framework::AppParam &appParam,
		const graphics::GraphicsParam &graphParam);
	~MyApp() = default;

	std::unique_ptr<framework::scene::SceneBase> &getScene(SceneId id);
	template <class T>
	T *getSceneAs(SceneId id)
	{
		return static_cast<T *>(getScene(id).get());
	}
	void setScene(SceneId id);

protected:
	void init() override;
	void update() override;
	void render() override;

private:
	// scene instance array
	std::array<std::unique_ptr<framework::scene::SceneBase>,
		static_cast<size_t>(SceneId::Count)> m_scenes;
	// current scene
	framework::scene::SceneBase *m_pCurrentScene = nullptr;
};

class MainScene : public framework::scene::AsyncLoadScene {
public:
	MainScene(MyApp *app, bool debugEnable);
	~MainScene() = default;

	// Scene specific initialization at scene start
	void setup();
	void update() override;
	void render() override;

protected:
	void loadOnSubThread(std::atomic_bool &cancel) override;

private:
	// 4MiB
	static const size_t LuaHeapSize = 4 * 1024 * 1024;
	// instruction execution limit
	static const int LuaInstLimit = 100000;

	MyApp *m_app;
	bool m_loading = false;
	lua::Lua m_lua;
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
