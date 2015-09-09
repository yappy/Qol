#include "stdafx.h"
#include "include/sound.h"
#include "include/exceptions.h"

namespace test {
namespace sound {

using error::checkDXResult;
using error::XAudioError;

DSound::DSound() :
	m_pIXAudio(nullptr, util::iunknownDeleter),
	m_pMasterVoice(nullptr, voiceDeleter)
{
	HRESULT hr;

	::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	IXAudio2 *ptmpIXAudio2;
	UINT32 flags = 0;
#ifdef _DEBUG
	flags = XAUDIO2_DEBUG_ENGINE;
#endif
	hr = ::XAudio2Create(&ptmpIXAudio2, flags);
	checkDXResult<XAudioError>(hr, "XAudio2Create() failed");
	m_pIXAudio.reset(ptmpIXAudio2);

	IXAudio2MasteringVoice *ptmpMasterVoice;
	hr = m_pIXAudio->CreateMasteringVoice(&ptmpMasterVoice);
	checkDXResult<XAudioError>(hr, "CreateMasteringVoice() failed");
	m_pMasterVoice.reset(ptmpMasterVoice);
}

DSound::~DSound()
{
	::CoUninitialize();
}

}
}
