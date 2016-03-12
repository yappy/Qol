#include "stdafx.h"
#include "App.h"

SubScene::SubScene(MyApp *app) : m_app(app)
{
	m_app->addSeResource(1, "testse", L"/C:/Windows/Media/chimes.wav");
	m_app->loadResourceSet(1, std::atomic_bool());
}

void SubScene::setup()
{

}

void SubScene::update()
{
	// input test
	auto testse = m_app->getSoundEffect(ResSetId::Common, "testse");

	auto keys = m_app->input().getKeys();
	for (size_t i = 0U; i < keys.size(); i++) {
		if (keys[i]) {
			debug::writef(L"Key 0x%02x", i);
			m_app->sound().playSoundEffect(testse);
		}
	}
	for (int i = 0; i < m_app->input().getPadCount(); i++) {
		DIJOYSTATE state;
		m_app->input().getPadState(&state, i);
		for (int b = 0; b < 32; b++) {
			if (state.rgbButtons[b] & 0x80) {
				debug::writef(L"pad[%d].button%d", i, b);
				m_app->sound().playSoundEffect(testse);
			}
		}
		{
			// left stick
			if (std::abs(state.lX) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].x=%ld", i, state.lX);
			}
			if (std::abs(state.lY) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].y=%ld", i, state.lY);
			}
			// right stick
			if (std::abs(state.lZ) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].z=%ld", i, state.lZ);
			}
			if (std::abs(state.lRz) > input::DInput::AXIS_THRESHOLD) {
				debug::writef(L"pad[%d].rz=%ld", i, state.lRz);
			}
		}
		for (int b = 0; b < 4; b++) {
			if (state.rgdwPOV[b] != -1) {
				debug::writef(L"pad[%d].POV%d=%u", i, b, state.rgdwPOV[b]);
			}
		}
	}
}

void SubScene::render()
{
	
}