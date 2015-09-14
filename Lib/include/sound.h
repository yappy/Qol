#pragma once

#include "util.h"
#include "file.h"
#include <xaudio2.h>
#include <unordered_map>

namespace test {
namespace sound {

// 3MiB
const size_t SoundFileSizeMax = 3 * 1024 * 1024;
const size_t SoundEffectPlayMax = 64;

struct hmmioDeleter {
	using pointer = HMMIO;
	void operator()(HMMIO hMmio)
	{
		::mmioClose(hMmio, 0);
	}
};

inline void voiceDeleter(IXAudio2Voice *pv)
{
	pv->DestroyVoice();
}
typedef decltype(&voiceDeleter) VoiceDeleterType;

struct SoundEffect : private util::noncopyable {
	WAVEFORMATEX format;
	file::Bytes samples;
	SoundEffect() = default;
};


class XAudio2 : private util::noncopyable {
public:
	XAudio2();
	~XAudio2();

	// Sound Effect
	void loadSoundEffect(const char *id, const wchar_t *path);
	void playSoundEffect(const char *id);
	bool isPlayingAny() const noexcept;
	void stopAllSoundEffect();

	// BGM
	void playBgm(const wchar_t *path);
	void processFrame();

private:
	using IXAudio2Ptr = std::unique_ptr<IXAudio2, util::IUnknownDeleterType>;
	using SourceVoicePtr = std::unique_ptr<IXAudio2SourceVoice, VoiceDeleterType>;
	using MasterVoicePtr = std::unique_ptr<IXAudio2MasteringVoice, VoiceDeleterType>;

	util::Com m_com;
	IXAudio2Ptr m_pIXAudio;
	MasterVoicePtr m_pMasterVoice;

	std::unordered_map<std::string, SoundEffect> m_seMap;
	std::vector<SourceVoicePtr> m_playingSeList;

	SourceVoicePtr *findFreeSeEntry() noexcept;

	// for ogg file callback (datasource==this)
	static size_t read(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int seek(void *datasource, int64_t offset, int whence);
	static long tell(void *datasource);
	static int close(void *datasource);
};

}
}
