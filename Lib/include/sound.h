#pragma once

#include "util.h"
#include <xaudio2.h>

namespace test {
namespace sound {

inline void voiceDeleter(IXAudio2Voice *iu)
{
	iu->DestroyVoice();
}
typedef decltype(&voiceDeleter) VoiceDeleterType;

class DSound : private util::noncopyable {
public:
	DSound();
	~DSound();

private:
	std::unique_ptr<IXAudio2, util::IUnknownDeleterType> m_pIXAudio;
	std::unique_ptr<IXAudio2MasteringVoice, VoiceDeleterType> m_pMasterVoice;
};

}
}
