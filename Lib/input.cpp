#include "stdafx.h"
#include "include/input.h"
#include "include/debug.h"
#include "include/exceptions.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace yappy {
namespace input {

using error::checkDXResult;
using error::DIError;

DInput::DInput(HINSTANCE hInst, HWND hWnd, bool foreground, bool exclusive) :
	m_pDi(nullptr, util::iunknownDeleter),
	m_pKeyDevice(nullptr, util::iunknownDeleter)
{
	debug::writeLine(L"Initializing DirectInput...");

	HRESULT hr = S_OK;

	// Initialize DirectInput
	IDirectInput8 *ptmpDi;
	hr = ::DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void **>(&ptmpDi), NULL);
	checkDXResult<DIError>(hr, "DirectInput8Create() failed");
	m_pDi.reset(ptmpDi);

	// Key Device
	debug::writeLine(L"Get Keyboard...");
	IDirectInputDevice8 *ptmpKeyDevice;
	hr = m_pDi->CreateDevice(GUID_SysKeyboard, &ptmpKeyDevice, NULL);
	checkDXResult<DIError>(hr, "IDirectInput8::CreateDevice() failed");
	m_pKeyDevice.reset(ptmpKeyDevice);

	hr = m_pKeyDevice->SetDataFormat(&c_dfDIKeyboard);
	checkDXResult<DIError>(hr, "IDirectInputDevice8::SetDataFormat() failed");
	DWORD coopLevel = DISCL_NOWINKEY;
	coopLevel |= (foreground ? DISCL_FOREGROUND : DISCL_BACKGROUND);
	coopLevel |= (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE);
	hr = m_pKeyDevice->SetCooperativeLevel(hWnd, coopLevel);
	checkDXResult<DIError>(hr, "IDirectInputDevice8::SetCooperativeLevel() failed");
	debug::writeLine(L"Get Keyboard OK");

	// GamePad Device
	updateControllers(hWnd, foreground, exclusive);

	debug::writeLine(L"Initializing DirectInput OK");
}

DInput::~DInput()
{
	// Unacquire (ignore errors)
	m_pKeyDevice->Unacquire();
	for (auto &dev : m_pPadDevs) {
		dev->Unacquire();
	}
	debug::writeLine(L"Finalize DirectInput");
}

// file local functions for callback
namespace {

BOOL CALLBACK enumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	std::vector<DIDEVICEINSTANCE> *devs = reinterpret_cast<decltype(devs)>(pvRef);
	devs->emplace_back(*lpddi);
	return DIENUM_CONTINUE;
}

BOOL CALLBACK enumAxesCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	DIPROPRANGE range = { 0 };
	range.diph.dwSize = sizeof(range);
	range.diph.dwHeaderSize = sizeof(range.diph);
	range.diph.dwHow = DIPH_BYID;
	range.diph.dwObj = lpddoi->dwType;
	range.lMax = DInput::AXIS_RANGE;
	range.lMin = -DInput::AXIS_RANGE;
	IDirectInputDevice8 *pdev = reinterpret_cast<decltype(pdev)>(pvRef);
	pdev->SetProperty(DIPROP_RANGE, &range.diph);
	return DIENUM_CONTINUE;
}

}

void DInput::updateControllers(HWND hwnd, bool foreground, bool exclusive)
{
	debug::writeLine(L"Get controllers...");

	HRESULT hr = S_OK;

	m_padInstList.clear();
	m_pPadDevs.clear();

	hr = m_pDi->EnumDevices(DI8DEVCLASS_GAMECTRL, enumDevicesCallback,
		&m_padInstList, DIEDFL_ATTACHEDONLY);
	checkDXResult<DIError>(hr, "IDirectInput8::EnumDevices() failed");

	for (const auto &padInst : m_padInstList) {
		IDirectInputDevice8 *ptmpDevice;
		hr = m_pDi->CreateDevice(padInst.guidInstance, &ptmpDevice, nullptr);
		checkDXResult<DIError>(hr, "IDirectInput8::CreateDevice() failed");
		util::IUnknownPtr<IDirectInputDevice8> pDevice(ptmpDevice, util::iunknownDeleter);

		hr = pDevice->SetDataFormat(&c_dfDIJoystick);
		checkDXResult<DIError>(hr, "IDirectInputDevice8::SetDataFormat() failed");
		DWORD coopLevel = 0;
		coopLevel |= (foreground ? DISCL_FOREGROUND : DISCL_BACKGROUND);
		coopLevel |= (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE);
		hr = pDevice->SetCooperativeLevel(hwnd, coopLevel);
		checkDXResult<DIError>(hr, "IDirectInputDevice8::SetCooperativeLevel() failed");
		hr = pDevice->EnumObjects(enumAxesCallback, pDevice.get(), DIDFT_AXIS);
		checkDXResult<DIError>(hr, "IDirectInputDevice8::EnumObjects() failed");

		m_pPadDevs.emplace_back(std::move(pDevice));
		DIJOYSTATE js = { 0 };
		m_pad.emplace_back(js);
	}

	{
		int i = 0;
		for (const auto &padInst : m_padInstList) {
			debug::writef(L"[Controller %d]", i);
			debug::writef(L"tszInstanceName = %s", padInst.tszInstanceName);
			debug::writef(L"tszProductName  = %s", padInst.tszProductName);
			i++;
		}
		debug::writef(L"%zu controller(s) found", m_pPadDevs.size());
	}
}

void DInput::processFrame()
{
	HRESULT hr = S_OK;

	// Keyboard
	m_key.fill(0);
	do {
		hr = m_pKeyDevice->Acquire();
	} while (hr == DIERR_INPUTLOST);
	if (hr != DIERR_OTHERAPPHASPRIO) {
		BYTE tmp[256];
		hr = m_pKeyDevice->GetDeviceState(sizeof(tmp), &tmp);
		if (SUCCEEDED(hr)) {
			for (int i = 0; i < 256; i++) {
				m_key[i] = ((tmp[i] & 0x80) != 0);
			}
		}
	}
	else {
		// DISCL_FOREGROUND and app is background now
		//debug::writeLine(L"DIERR_OTHERAPPHASPRIO");
	}

	// Pad
	// TODO: think twice about error handling
	// TODO: sudden disconnect?
	for (size_t i = 0; i < m_pPadDevs.size(); i++) {
		ZeroMemory(&m_pad[i], sizeof(m_pad[i]));
		memset(m_pad[i].rgdwPOV, -1, sizeof(m_pad[i].rgdwPOV));

		do {
			hr = m_pPadDevs[i]->Acquire();
		} while (hr == DIERR_INPUTLOST);
		if (hr == DIERR_OTHERAPPHASPRIO) {
			// ISCL_FOREGROUND and app is background now
			continue;
		}
		hr = m_pPadDevs[i]->Poll();
		hr = m_pPadDevs[i]->GetDeviceState(sizeof(m_pad[i]), &m_pad[i]);
		// if error, m_pad[i] will be cleared state
	}
}

DInput::KeyData DInput::getKeys() const noexcept
{
	return m_key;
}

int DInput::getPadCount() const noexcept
{
	ASSERT(m_pad.size() == m_pPadDevs.size());
	return static_cast<int>(m_pPadDevs.size());
}

void DInput::getPadState(DIJOYSTATE *out, int index) const noexcept
{
	*out = m_pad.at(index);
}

}
}
