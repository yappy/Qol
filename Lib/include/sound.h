#pragma once

#include "util.h"
#include "file.h"
#include <xaudio2.h>
#include <vorbis/vorbisfile.h>
#include <unordered_map>

namespace yappy {
/// Sound effect and BGM library.
namespace sound {

/// SE file size limit (wave data part): 3MiB
const uint32_t SoundEffectSizeMax = 3 * 1024 * 1024;
/// Playing SE at the same time limit.
const uint32_t SoundEffectPlayMax = 64;

/// BGM ov_read unit.
const uint32_t BgmOvReadSize = 4096;
/// BGM one buffer size. (Consumes @ref BgmBufferSize * @ref BgmBufferCount bytes)
const uint32_t BgmBufferSize = 4096 * 16;
/// BGM buffer count.
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

/// Sound effect resource.
struct SoundEffect : private util::noncopyable {
	WAVEFORMATEX format;
	file::Bytes samples;
	SoundEffect() = default;
	~SoundEffect() = default;
};

/// BGM resource.
struct Bgm : private util::noncopyable {
	using OggFilePtr = std::unique_ptr<OggVorbis_File, oggFileDeleter>;

	Bgm(file::Bytes &&ovFileBin);
	~Bgm() = default;

	OggVorbis_File *ovFp() const { return m_ovFp.get(); }

private:
	file::Bytes m_ovFileBin;
	uint32_t m_readPos;
	OggVorbis_File m_ovFile;
	// for public interface (auto close pointer to m_ovFile)
	OggFilePtr m_ovFp;
	// for OggVorbis_File callback (datasource == this)
	static size_t read(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int seek(void *datasource, int64_t offset, int whence);
	static long tell(void *datasource);
	static int close(void *datasource);
};

/// XAudio2 manager.
class XAudio2 : private util::noncopyable {
public:
	using SeResource = const SoundEffect;
	using SeResourcePtr = std::shared_ptr<SeResource>;
	using BgmResource = Bgm;
	using BgmResourcePtr = std::shared_ptr<BgmResource>;

	/// Initialize XAudio2.
	XAudio2();
	/// Finalize XAudio2.
	~XAudio2();

	/** @brief Application must call this function every frames.
	 * @details
	 * @li Free SE entry which has done.
	 * @li Decode ogg partly and write to BGM buffer.
	 */
	void processFrame();

	/// @name Sound Effect
	//@{
	/** @brief Load a sound effect resource.
	 * @details This function may take time.
	 * @param[in]	path	(abstract file layer) File path.
	 * @return				shared_ptr to sound effect resource.
	 * @sa @ref yappy::file
	 */
	SeResourcePtr loadSoundEffect(const wchar_t *path);

	/** @brief Starts playing a sound effect.
	 * @param[in]	se	Sound effect resource.
	 */
	void playSoundEffect(const SeResourcePtr &se);

	/** @brief Returns whether any sound effects are playing.
	 * @return true if one or more sound effects are playing.
	 */
	bool isPlayingAnySoundEffect() const;

	/** @brief Stops all sound effects.
	 */
	void stopAllSoundEffect();
	//@}

	/// @name BGM
	//@{
	/** @brief Load a BGM resource.
	 * @details This function may take time.
	 * @param[in]	path	(abstract file layer) File path.
	 * @return				shared_ptr to BGM resource.
	 * @sa @ref yappy::file
	 */
	BgmResourcePtr loadBgm(const wchar_t *path);

	/** @brief Starts playing a BGM.
	 * @details
	 * Only one BGM can be played at the same time.
	 * This function will stop the current BGM and then start the new BGM.
	 * @param[in]	se	Sound effect resource.
	 */
	void playBgm(const BgmResourcePtr &bgm);

	/** @brief Stops playing a BGM.
	 */
	void stopBgm();
	//@}

private:
	using IXAudio2Ptr = util::ComPtr<IXAudio2>;
	using SourceVoicePtr = std::unique_ptr<IXAudio2SourceVoice, voiceDeleter>;
	using MasterVoicePtr = std::unique_ptr<IXAudio2MasteringVoice, voiceDeleter>;

	util::CoInitialize m_coInit;
	IXAudio2Ptr m_pIXAudio;
	MasterVoicePtr m_pMasterVoice;

	// Sound Effect
	// keep reference to playing buffer in resource
	using PyaingSeElem = std::tuple<SeResourcePtr, SourceVoicePtr>;
	std::array<PyaingSeElem, SoundEffectPlayMax> m_playingSeList;

	PyaingSeElem *findFreeSeEntry();
	void processFrameSe();

	// BGM
	// raw wave buffer, which must be deleted after m_pBgmVoice
	std::unique_ptr<char[]> m_pBgmBuffer;
	uint32_t m_writePos;
	// play m_pBgmBuffer at another thread
	SourceVoicePtr m_pBgmVoice;
	// keep reference to resource struct
	BgmResourcePtr m_playingBgm;

	void processFrameBgm();
};

}	// namespace sound
}	// namespace yappy
