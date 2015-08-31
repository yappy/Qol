#include "stdafx.h"
#include "input.hpp"
#include "debug.hpp"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace dx9lib {

	namespace {

		BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
		{
			std::vector<DIDEVICEINSTANCE> *devs;
			devs = (std::vector<DIDEVICEINSTANCE> *)pvRef;
			devs->push_back(*lpddi);
			return DIENUM_CONTINUE;
		}

		BOOL CALLBACK EnumAxesCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef)
		{
			DIPROPRANGE range;
			range.diph.dwSize = sizeof(range);
			range.diph.dwHeaderSize = sizeof(range.diph);
			range.diph.dwHow = DIPH_BYID;
			range.diph.dwObj = lpddoi->dwType;
			range.lMax = DInput::AXIS_RANGE;
			range.lMin = - DInput::AXIS_RANGE;
			IDirectInputDevice8 *pdev = (IDirectInputDevice8 *)pvRef;
			pdev->SetProperty(DIPROP_RANGE, &range.diph);
			return DIENUM_CONTINUE;
		}

	}

	DInput::DInput(HWND hwnd) : m_pdi()
	{
		debug.println("Initializing DirectInput...");

		ZeroMemory(&m_key, sizeof(m_key));
		// Initialize DirectInput
		IDirectInput8 *pdi;
		HINSTANCE hInst = GetModuleHandle(NULL);
		DInputError::check(
			DirectInput8Create(
				hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&pdi, NULL
			)
		);
		m_pdi.reset(pdi, iunknown_deleter());

		// Key Device
		IDirectInputDevice8 *pKeyDevice;
		DInputError::check(
			m_pdi->CreateDevice(GUID_SysKeyboard, &pKeyDevice, NULL)
		);
		m_pKeyDevice.reset(pKeyDevice, iunknown_deleter());
		DInputError::check(
			m_pKeyDevice->SetDataFormat(&c_dfDIKeyboard)
		);
		DInputError::check(
			m_pKeyDevice->SetCooperativeLevel(
				hwnd,
				DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY
			)
		);

		// GamePad Device
		updateControllers(hwnd);

		debug.println("Initializing DirectInput OK.");
	}

	DInput::~DInput()
	{
		m_pKeyDevice->Unacquire();
		for(unsigned i=0; i<m_pPadDevs.size(); i++){
			m_pPadDevs[i]->Unacquire();
		}
	}

	void DInput::updateControllers(HWND hwnd)
	{
		m_padInstList.clear();
		DInputError::check(
			m_pdi->EnumDevices(
				DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, &m_padInstList, DIEDFL_ATTACHEDONLY
			)
		);
		m_pPadDevs.clear();
		for(unsigned i=0; i<m_padInstList.size(); i++){
			IDirectInputDevice8 *pTmp;
			DInputError::check(
				m_pdi->CreateDevice(m_padInstList[i].guidInstance, &pTmp, NULL)
			);
			shared_ptr<IDirectInputDevice8> pDevice(pTmp, iunknown_deleter());
			DInputError::check(
				pDevice->SetDataFormat(&c_dfDIJoystick)
			);
			DInputError::check(
				pDevice->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)
			);
			DInputError::check(
				pDevice->EnumObjects(EnumAxesCallback, pDevice.get(), DIDFT_AXIS)
			);
			m_pPadDevs.push_back(pDevice);
			DIJOYSTATE js = {0};
			m_pad.push_back(js);
		}

		debug.printf("%u controller(s) found.\n", m_pPadDevs.size());
	}

	void DInput::processFrame()
	{
		HRESULT hr;
		// Keyboard
		ZeroMemory(&m_key, sizeof(m_key));
		hr = m_pKeyDevice->Acquire();
		if(hr != DIERR_OTHERAPPHASPRIO){
			DInputError::check(hr);
			hr = m_pKeyDevice->GetDeviceState(sizeof(m_key), &m_key);
			if(hr != DIERR_INPUTLOST){
				DInputError::check(hr);
			}
		}
		// Pad
		for(unsigned i=0; i<m_pPadDevs.size(); i++){
			ZeroMemory(&m_pad[i], sizeof(m_pad[i]));
			m_pPadDevs[i]->Acquire();
			m_pPadDevs[i]->Poll();
			m_pPadDevs[i]->GetDeviceState(sizeof(m_pad[i]), &m_pad[i]);
			/*for(int k=0; k<32; k++){
				if(m_pad[i].rgbButtons[k]){
					debug.printf("Pad%d Button%d\n", i, k);
				}
			}*/
		}
	}

	void DInput::getKeys(BYTE (&buf)[256])
	{
		memcpy(buf, m_key, sizeof(m_key));
	}

	int DInput::getPadCount() const
	{
		return m_pPadDevs.size();
	}

	void DInput::getPadState(DIJOYSTATE &out, int index) const
	{
		out = m_pad.at(index);
	}

}
