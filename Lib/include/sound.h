#pragma once

#include "util.h"
#include <xaudio2.h>
#include <unordered_map>

namespace test {
namespace sound {

// 3MiB
const size_t SoundFileSizeMax = 3 * 1024 * 1024;

struct hmmioDeleter {
	using pointer = HMMIO;
	void operator()(HMMIO hMmio)
	{
		::mmioClose(hMmio, 0);
	}
};

inline void voiceDeleter(IXAudio2Voice *iu)
{
	iu->DestroyVoice();
}
typedef decltype(&voiceDeleter) VoiceDeleterType;

struct SoundEffect : private util::noncopyable {
	WAVEFORMATEX format;
	std::vector<uint8_t> samples;
	SoundEffect() = default;
};

class XAudio2 : private util::noncopyable {
public:
	XAudio2();
	~XAudio2();
	void loadSound(const char *id, const wchar_t *path);
	void playSound(const char *id);
	void stopAllSound();

private:
	util::Com m_com;
	std::unique_ptr<IXAudio2, util::IUnknownDeleterType> m_pIXAudio;
	std::unique_ptr<IXAudio2MasteringVoice, VoiceDeleterType> m_pMasterVoice;

	std::unordered_map<std::string, SoundEffect> m_selib;
};

}
}
