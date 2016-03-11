#pragma once

#include <framework.h>
#include <script.h>

using namespace yappy;

enum class SceneId {
	Main,
	Sub,
	Count
};

class MyApp : public framework::Application {
public:
	MyApp(const framework::AppParam &appParam,
		const graphics::GraphicsParam &graphParam);
	~MyApp() = default;

	std::unique_ptr<framework::SceneBase> &getScene(SceneId id);
	void setScene(SceneId id);

protected:
	void init() override;
	void update() override;
	void render() override;

private:
	// scene instance array
	std::array<std::unique_ptr<framework::SceneBase>,
		static_cast<size_t>(SceneId::Count)> m_scenes;
	// current scene
	framework::SceneBase *m_pCurrentScene = nullptr;
};

class MainScene : public framework::AsyncLoadScene {
public:
	explicit MainScene(MyApp *app);
	~MainScene() = default;

	// Scene specific initialization at scene start
	void setup();

protected:
	void loadOnSubThread(std::atomic_bool &cancel) override;
	void update() override;
	void render() override;

private:
	MyApp *m_app;
	bool m_loading = false;
	lua::Lua m_lua;
};
