#pragma once

#include "util.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <vector>

namespace test {
namespace input {

class DInput : private util::noncopyable {
public:
	static const int AXIS_RANGE = 1000;

	DInput(HWND hwnd, bool foreground = true, bool exclusive = false);
	~DInput();
	void updateControllers(HWND hwnd, bool foreground = true, bool exclusive = false);
	void processFrame();
	void getKeys(BYTE (&buf)[256]) const noexcept;
	int getPadCount() const noexcept;
	void getPadState(DIJOYSTATE &out, int index) const noexcept;

private:
	std::unique_ptr<IDirectInput8, decltype(&util::iunknownDeleter)> m_pDi;
	std::unique_ptr<IDirectInputDevice8, decltype(&util::iunknownDeleter)> m_pKeyDevice;
	std::vector<DIDEVICEINSTANCE> m_padInstList;
	std::vector<std::unique_ptr<IDirectInputDevice8, decltype(&util::iunknownDeleter)>> m_pPadDevs;
	BYTE m_key[256] = { 0 };
	std::vector<DIJOYSTATE> m_pad;
};

}
}
