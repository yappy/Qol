#pragma once

#include "util.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <array>
#include <vector>

namespace test {
namespace input {

class DInput : private util::noncopyable {
public:
	static const int KEY_NUM = 256;
	static const int AXIS_RANGE = 1000;
	static const int AXIS_THRESHOLD = AXIS_RANGE / 2;

	DInput(HWND hwnd, bool foreground = true, bool exclusive = false);
	~DInput();
	void updateControllers(HWND hwnd, bool foreground = true, bool exclusive = false);
	void processFrame();
	std::array<bool, 256> getKeys() const noexcept;
	int getPadCount() const noexcept;
	void getPadState(DIJOYSTATE *out, int index) const noexcept;

private:
	std::unique_ptr<IDirectInput8, util::IUnknownDeleterType> m_pDi;
	std::unique_ptr<IDirectInputDevice8, util::IUnknownDeleterType> m_pKeyDevice;
	std::vector<DIDEVICEINSTANCE> m_padInstList;
	std::vector<std::unique_ptr<IDirectInputDevice8, util::IUnknownDeleterType>> m_pPadDevs;
	std::array<bool, 256> m_key;
	std::vector<DIJOYSTATE> m_pad;
};

}
}
