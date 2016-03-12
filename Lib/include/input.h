#pragma once

#include "util.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <array>
#include <vector>

namespace yappy {
namespace input {

class DInput : private util::noncopyable {
public:
	static const int KEY_NUM = 256;
	static const int AXIS_RANGE = 1000;
	static const int AXIS_THRESHOLD = AXIS_RANGE / 2;

	using KeyData = std::array<bool, KEY_NUM>;

	DInput(HINSTANCE hInst, HWND hWnd, bool foreground = true, bool exclusive = false);
	~DInput();
	void updateControllers(HWND hwnd, bool foreground = true, bool exclusive = false);
	void processFrame();
	KeyData getKeys() const noexcept;
	int getPadCount() const noexcept;
	void getPadState(DIJOYSTATE *out, int index) const noexcept;

private:
	util::IUnknownPtr<IDirectInput8> m_pDi;
	util::IUnknownPtr<IDirectInputDevice8> m_pKeyDevice;
	std::vector<DIDEVICEINSTANCE> m_padInstList;
	std::vector<util::IUnknownPtr<IDirectInputDevice8>> m_pPadDevs;
	KeyData m_key;
	std::vector<DIJOYSTATE> m_pad;
};

const char *dikToString(int dik);
const wchar_t *dikToWString(int dik);

}
}
