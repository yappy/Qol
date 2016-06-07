#pragma once

#include "util.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <array>
#include <vector>

namespace yappy {
/// Input library.
namespace input {

/** @brief DirectInput8 wrapper.
 */
class DInput : private util::noncopyable {
public:
	/// Keyboard array count.
	static const int KEY_NUM = 256;
	/// Pad analog value max.
	static const int AXIS_RANGE = 1000;
	/// Pad analog value threshold.
	static const int AXIS_THRESHOLD = AXIS_RANGE / 2;

	/** @brief Keyboard data: array[@ref KEY_NUM] of bool.
	 * @details Index is DIK_XXX in dinput.h.
	 */
	using KeyData = std::array<bool, KEY_NUM>;

	/** @brief Initialize DirectInput8.
	 * @param[in]	hInst		Instance handle.
	 * @param[in]	hWnd		Window handle.
	 * @param[in]	foreground	Can obtain input data only when the window is active.
	 * @param[in]	exclusive	Obtain exclusive access to the device.
	 */
	DInput(HINSTANCE hInst, HWND hWnd, bool foreground = true, bool exclusive = false);
	/** @brief Finalize DirectInput8.
	 */
	~DInput();

	/** @brief Update pad list.
	 * @details Also called in the constructor.
	 * @param[in]	hInst		Instance handle.
	 * @param[in]	hWnd		Window handle.
	 * @param[in]	foreground	Can obtain input data only when the window is active.
	 * @param[in]	exclusive	Obtain exclusive access to the device.
	 */
	void updateControllers(HWND hwnd, bool foreground = true, bool exclusive = false);
	/** @brief Must be called at every frame.
	 * @details Update the key and pad input status.
	 */
	void processFrame();
	/** @brief Get keyboard input data.
	 * @return Keyboard input data.
	 */
	KeyData getKeys() const;
	/** @brief Get connected pad count.
	 */
	int getPadCount() const;
	/** @brief Get pad input data.
	 * @param[out]	state	Current input state.
	 * @param[in]	index	Pad index.
	 * @pre 0 <= index < @ref getPadCount()
	 */
	void getPadState(DIJOYSTATE *state, int index) const;

private:
	util::ComPtr<IDirectInput8> m_pDi;
	util::ComPtr<IDirectInputDevice8> m_pKeyDevice;
	std::vector<DIDEVICEINSTANCE> m_padInstList;
	std::vector<util::ComPtr<IDirectInputDevice8>> m_pPadDevs;
	KeyData m_key;
	std::vector<DIJOYSTATE> m_pad;
};

/// Convert DIK_XXX to string.
const char *dikToString(int dik);
/// Convert DIK_XXX to wide-string.
const wchar_t *dikToWString(int dik);

}	// namespace input
}	// namespace yappy
