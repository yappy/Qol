#include "stdafx.h"
#include "include/input.h"
#include "include/debug.h"
#include "include/exceptions.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace test {
namespace input {

using error::checkDXResult;
using error::DIError;

DInput::DInput(HWND hwnd, bool foreground, bool exclusive) :
	m_pDi(nullptr, util::iunknownDeleter),
	m_pKeyDevice(nullptr, util::iunknownDeleter)
{
	debug::writeLine(L"Initializing DirectInput...");

	// Initialize DirectInput
	IDirectInput8 *pdi;
	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	checkDXResult<DIError>(
		::DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8,
			reinterpret_cast<void **>(&pdi), NULL),
		"DirectInput8Create() failed");
	m_pDi.reset(pdi);

	// Key Device
	debug::writeLine(L"Get Keyboard...");
	IDirectInputDevice8 *pKeyDevice;
	checkDXResult<DIError>(
		m_pDi->CreateDevice(GUID_SysKeyboard, &pKeyDevice, NULL),
		"IDirectInput8::CreateDevice() failed");
	m_pKeyDevice.reset(pKeyDevice);
	checkDXResult<DIError>(
		m_pKeyDevice->SetDataFormat(&c_dfDIKeyboard),
		"IDirectInputDevice8::SetDataFormat() failed");
	DWORD coopLevel = DISCL_NOWINKEY;
	coopLevel |= (foreground ? DISCL_FOREGROUND : DISCL_BACKGROUND);
	coopLevel |= (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE);
	checkDXResult<DIError>(
		m_pKeyDevice->SetCooperativeLevel(hwnd, coopLevel),
		"IDirectInputDevice8::SetCooperativeLevel() failed");
	debug::writeLine(L"Get Keyboard OK");

	// GamePad Device
	updateControllers(hwnd, foreground, exclusive);

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
	std::vector<DIDEVICEINSTANCE> *devs;
	devs = reinterpret_cast<std::vector<DIDEVICEINSTANCE> *>(pvRef);
	devs->push_back(*lpddi);
	return DIENUM_CONTINUE;
}

BOOL CALLBACK enumAxesCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
{
	DIPROPRANGE range;
	range.diph.dwSize = sizeof(range);
	range.diph.dwHeaderSize = sizeof(range.diph);
	range.diph.dwHow = DIPH_BYID;
	range.diph.dwObj = lpddoi->dwType;
	range.lMax = DInput::AXIS_RANGE;
	range.lMin = -DInput::AXIS_RANGE;
	IDirectInputDevice8 *pdev = (IDirectInputDevice8 *)pvRef;
	pdev->SetProperty(DIPROP_RANGE, &range.diph);
	return DIENUM_CONTINUE;
}

}

void DInput::updateControllers(HWND hwnd, bool foreground, bool exclusive)
{
	debug::writeLine(L"Get controllers...");

	m_padInstList.clear();
	m_pPadDevs.clear();

	checkDXResult<DIError>(
		m_pDi->EnumDevices(DI8DEVCLASS_GAMECTRL, enumDevicesCallback,
			&m_padInstList, DIEDFL_ATTACHEDONLY),
		"IDirectInput8::EnumDevices() failed");

	for (const auto &padInst : m_padInstList) {
		IDirectInputDevice8 *pTmp;
		checkDXResult<DIError>(
			m_pDi->CreateDevice(padInst.guidInstance, &pTmp, nullptr),
			"IDirectInput8::CreateDevice() failed");
		std::unique_ptr<IDirectInputDevice8, decltype(&util::iunknownDeleter)>
			pDevice(pTmp, util::iunknownDeleter);
		checkDXResult<DIError>(
			pDevice->SetDataFormat(&c_dfDIJoystick),
			"IDirectInputDevice8::SetDataFormat() failed");
		DWORD coopLevel = 0;
		coopLevel |= (foreground ? DISCL_FOREGROUND : DISCL_BACKGROUND);
		coopLevel |= (exclusive ? DISCL_EXCLUSIVE : DISCL_NONEXCLUSIVE);
		checkDXResult<DIError>(
			pDevice->SetCooperativeLevel(hwnd, coopLevel),
			"IDirectInputDevice8::SetCooperativeLevel() failed");
		checkDXResult<DIError>(
			pDevice->EnumObjects(enumAxesCallback, pDevice.get(), DIDFT_AXIS),
			"IDirectInputDevice8::EnumObjects() failed");
		m_pPadDevs.push_back(std::move(pDevice));
		DIJOYSTATE js = { 0 };
		m_pad.push_back(js);
	}

	{
		int i = 0;
		for (const auto &padInst : m_padInstList) {
			debug::writef(L"[Controller %d]", i);
			debug::writef(L"tszInstanceName = %s", padInst.tszInstanceName);
			debug::writef(L"tszProductName  = %s", padInst.tszProductName);
			i++;
		}
		debug::writef(L"%u controller(s) found", m_pPadDevs.size());
	}
}

void DInput::processFrame()
{
	HRESULT hr;
	// Keyboard
	ZeroMemory(&m_key, sizeof(m_key));
	hr = m_pKeyDevice->Acquire();
	if (hr != DIERR_OTHERAPPHASPRIO) {
		checkDXResult<DIError>(hr, "IDirectInputDevice8::Acquire() failed");
		hr = m_pKeyDevice->GetDeviceState(sizeof(m_key), &m_key);
		if (hr != DIERR_INPUTLOST) {
			checkDXResult<DIError>(hr, "IDirectInputDevice8::GetDeviceState() failed");
		}
		else {
			debug::writeLine(L"DIERR_INPUTLOST");
		}
	}
	else {
		// DISCL_FOREGROUND and app is background now
		//debug::writeLine(L"DIERR_OTHERAPPHASPRIO");
	}
	// Pad
	// TODO: think twice about error handling
	// TODO: sudden disconnect?
	for (unsigned i = 0; i < m_pPadDevs.size(); i++) {
		ZeroMemory(&m_pad[i], sizeof(m_pad[i]));
		m_pPadDevs[i]->Acquire();
		m_pPadDevs[i]->Poll();
		m_pPadDevs[i]->GetDeviceState(sizeof(m_pad[i]), &m_pad[i]);
		/*
		for (int k = 0; k < 32; k++) {
			if (m_pad[i].rgbButtons[k]) {
				debug::writef(L"Pad%d Button%d", i, k);
			}
		}
		//*/
	}
}

void DInput::getKeys(BYTE (&buf)[256]) const noexcept
{
	memcpy(buf, m_key, sizeof(m_key));
}

int DInput::getPadCount() const noexcept
{
	ASSERT(m_pad.size() == m_pPadDevs.size());
	return m_pPadDevs.size();
}

void DInput::getPadState(DIJOYSTATE &out, int index) const noexcept
{
	out = m_pad.at(index);
}

}
}
