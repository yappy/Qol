#include "stdafx.h"
#include "include\sound.h"

namespace test {
namespace sound {

DSound::DSound()
{
	::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

DSound::~DSound()
{
	::CoUninitialize();
}

}
}
