#pragma once

#include "util.hpp"
#include <dinput.h>
#include <vector>

namespace dx9lib {

	class DInput : private Uncopyable {
	private:
		shared_ptr<IDirectInput8> m_pdi;
		shared_ptr<IDirectInputDevice8> m_pKeyDevice;
		std::vector<DIDEVICEINSTANCE> m_padInstList;
		std::vector<shared_ptr<IDirectInputDevice8> > m_pPadDevs;
		BYTE m_key[256];
		std::vector<DIJOYSTATE> m_pad;
	public:
		static const int AXIS_RANGE = 1000;

		DInput(HWND hwnd);
		~DInput();
		void updateControllers(HWND hwnd);
		void processFrame();
		void getKeys(BYTE (&buf)[256]);
		int getPadCount() const;
		void getPadState(DIJOYSTATE &out, int index) const;
	};

}
