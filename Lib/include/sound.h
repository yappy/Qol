#pragma once

#include "util.h"
#include "file.h"
#include <xaudio2.h>
#include <vorbis/vorbisfile.h>
#include <unordered_map>

namespace yappy {
namespace sound {

// 3MiB
const uint32_t SoundFileSizeMax = 3 * 1024 * 1024;
const uint32_t SoundEffectPlayMax = 64;

const uint32_t BgmOvReadSize = 4096;
const uint32_t BgmBufferSize = 4096 * 16;
const uint32_t BgmBufferCount = 2;

#pragma region Deleters
struct hmmioDeleter {
	using pointer = HMMIO;
	void operator()(HMMIO hMmio)
	{
		::mmioClose(hMmio, 0);
	}
};

struct voiceDeleter {
	void operator()(IXAudio2Voice *pv)
	{
		pv->DestroyVoice();
	}
};

struct oggFileDeleter {
	void operator()(OggVorbis_File *ovf)
	{
		ov_clear(ovf);
	}
};
#pragma endregion

struct SoundEffect : private util::noncopyable {
	WAVEFORMATEX format;
	file::Bytes samples;
	SoundEffect() = default;
	~SoundEffect() = default;
};

class XAudio2 : private util::noncopyable {
public:
	using SeResource = const SoundEffect;
	using SeResourcePtr = std::shared_ptr<SeResource>;

	XAudio2();
	~XAudio2();

	void processFrame();

	// Sound Effect
	SeResourcePtr loadSoundEffect(const wchar_t *path);
	void playSoundEffect(const SeResourcePtr &se);
	bool isPlayingAnySoundEffect() const noexcept;
	void stopAllSoundEffect();

	// BGM
	void playBgm(const wchar_t *path);
	void stopBgm() noexcept;

private:
	using IXAudio2Ptr		= util::ComPtr<IXAudio2>;
	using SourceVoicePtr	= std::unique_ptr<IXAudio2SourceVoice, voiceDeleter>;
	using MasterVoicePtr	= std::unique_ptr<IXAudio2MasteringVoice, voiceDeleter>;
	using OggFilePtr		= std::unique_ptr<OggVorbis_File, oggFileDeleter>;

	util::Com m_com;
	IXAudio2Ptr m_pIXAudio;
	MasterVoicePtr m_pMasterVoice;

	// Sound Effect
	using PyaingSeElem = std::tuple<SeResourcePtr, SourceVoicePtr>;
	std::array<PyaingSeElem, SoundEffectPlayMax> m_playingSeList;

	PyaingSeElem *findFreeSeEntry() noexcept;
	void checkSePlayEnd() noexcept;

	// BGM
	// raw wave buffer, which must be deleted after m_pBgmVoice destruction
	std::unique_ptr<char[]> m_pBgmBuffer;
	// play m_pBgmBuffer at another thread
	SourceVoicePtr m_pBgmVoice;
	OggFilePtr m_pBgmFile;
	file::Bytes m_ovFileBin;
	uint32_t m_readPos;
	uint32_t m_writePos;
	// for ogg file callback (datasource == this)
	static size_t read(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int seek(void *datasource, int64_t offset, int whence);
	static long tell(void *datasource);
	static int close(void *datasource);
};

}
}
