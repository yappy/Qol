#include "stdafx.h"
#include "include/sound.h"
#include "include/debug.h"
#include "include/exceptions.h"

namespace test {
namespace sound {

using error::checkDXResult;
using error::XAudioError;

XAudio2::XAudio2() :
	m_pIXAudio(nullptr, util::iunknownDeleter),
	m_pMasterVoice(nullptr, voiceDeleter)
{
	HRESULT hr;


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

	for (int i = 0; i < 1000; i++) {
		IXAudio2SourceVoice *ptmpSrcVoice;
		WAVEFORMATEX fmt = { WAVE_FORMAT_PCM, 1, 44100, 44100, 1, 8, 0 };
		{
			debug::StopWatch(L"CreateSourceVoice()");
			m_pIXAudio->CreateSourceVoice(&ptmpSrcVoice, &fmt);
		}

		XAUDIO2_BUFFER buffer = { 0 };
		{
			debug::StopWatch(L"vector()");
			std::vector<BYTE> bytes(44100 * 5);
			buffer.AudioBytes = static_cast<UINT32>(bytes.size());
			buffer.pAudioData = &bytes.at(0);
			buffer.Flags = XAUDIO2_END_OF_STREAM;
		}
		{
			debug::StopWatch(L"SubmitSourceBuffer(44.1kHz*5sec)");
			ptmpSrcVoice->SubmitSourceBuffer(&buffer);
		}
		ptmpSrcVoice->DestroyVoice();
	}
}

XAudio2::~XAudio2() {}

}
}
