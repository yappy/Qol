#include "stdafx.h"
#include "include/input.h"
#include "include/debug.h"
#include "include/exceptions.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace test {
namespace input {

DInput::DInput(HWND hwnd) :
	m_pDi(nullptr, util::iunknownDeleter),
	m_pKeyDevice(nullptr, util::iunknownDeleter)
{
	HRESULT hr;

	debug::writeLine("Initializing DirectInput...");

	// Initialize DirectInput
	IDirectInput8 *pdi;
	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	hr = ::DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8,
		reinterpret_cast<void **>(&pdi), NULL);
	if (FAILED(hr)) {
		throw DIError("DirectInput8Create() failed", hr);
	}
	m_pDi.reset(pdi);

	// Key Device
	IDirectInputDevice8 *pKeyDevice;
	hr = m_pDi->CreateDevice(GUID_SysKeyboard, &pKeyDevice, NULL);
	if (FAILED(hr)) {
		throw DIError("IDirectInput8::CreateDevice() failed", hr);
	}
	m_pKeyDevice.reset(pKeyDevice);
	hr = m_pKeyDevice->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr)) {
		throw DIError("IDirectInputDevice8::SetDataFormat() failed", hr);
	}
	hr = m_pKeyDevice->SetCooperativeLevel(hwnd,
		DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY);
	if (FAILED(hr)) {
		throw DIError("IDirectInputDevice8::SetCooperativeLevel() failed", hr);
	}

	// GamePad Device
	updateControllers(hwnd);

	debug::writeLine("Initializing DirectInput OK");
}

DInput::~DInput()
{
	m_pKeyDevice->Unacquire();
	for (auto &dev : m_pPadDevs) {
		dev->Unacquire();
	}
	debug::writeLine("Finalize DirectInput");
}

// file local for callback
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

void DInput::updateControllers(HWND hwnd)
{
	HRESULT hr;

	m_padInstList.clear();
	hr = m_pDi->EnumDevices(DI8DEVCLASS_GAMECTRL, enumDevicesCallback,
		&m_padInstList, DIEDFL_ATTACHEDONLY);
	m_pPadDevs.clear();

	for (const auto &padInst : m_padInstList) {
		IDirectInputDevice8 *pTmp;
		hr = m_pDi->CreateDevice(padInst.guidInstance, &pTmp, nullptr);
		if (FAILED(hr)) {
			throw DIError("IDirectInput8::CreateDevice() failed", hr);
		}
		std::unique_ptr<IDirectInputDevice8, decltype(&util::iunknownDeleter)>
			pDevice(pTmp, util::iunknownDeleter);
		hr = pDevice->SetDataFormat(&c_dfDIJoystick);
		if (FAILED(hr)) {
			throw DIError("IDirectInputDevice8::SetDataFormat() failed", hr);
		}
		hr = pDevice->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
		if (FAILED(hr)) {
			throw DIError("IDirectInputDevice8::SetCooperativeLevel() failed", hr);
		}
		hr = pDevice->EnumObjects(enumAxesCallback, pDevice.get(), DIDFT_AXIS);
		if (FAILED(hr)) {
			throw DIError("IDirectInputDevice8::EnumObjects() failed", hr);
		}
		m_pPadDevs.push_back(std::move(pDevice));
		DIJOYSTATE js = { 0 };
		m_pad.push_back(js);
	}

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
}
