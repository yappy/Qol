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

class XAudio2 : private util::noncopyable {
public:
	XAudio2();
	~XAudio2();

private:
	util::Com m_com;
	std::unique_ptr<IXAudio2, util::IUnknownDeleterType> m_pIXAudio;
	std::unique_ptr<IXAudio2MasteringVoice, VoiceDeleterType> m_pMasterVoice;
};

}
}
