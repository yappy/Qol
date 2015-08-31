#pragma once

#include "util.hpp"
#include "file.hpp"
#include <dsound.h>
#include <list>

struct OggVorbis_File;

namespace dx9lib {

	class SoundEffect : private Uncopyable {
	private:
		shared_ptr<IDirectSoundBuffer8> m_pBuffer;
	public:
		explicit SoundEffect(shared_ptr<IDirectSoundBuffer8> pBuffer) : m_pBuffer(pBuffer) {}
		shared_ptr<IDirectSoundBuffer8> getBuffer() { return m_pBuffer; }
	};

	class DSound : private Uncopyable {
	private:
		static const unsigned BGM_BUFFER_SIZE;

		shared_ptr<IDirectSound8> m_pds;
		shared_ptr<IDirectSoundBuffer8> createSoundBuffer(const std::vector<BYTE> &src);

		std::list<shared_ptr<IDirectSoundBuffer8>> m_playing;

		shared_ptr<OggVorbis_File> m_povf;
		shared_ptr<IDirectSoundBuffer8> m_pbgmBuf;
		bool m_bgmFlip;
	public:
		explicit DSound(HWND hwnd);

		shared_ptr<SoundEffect> load(ResourceLoader &loader, const tstring &fileName);
		void play(shared_ptr<SoundEffect> se);
		void stopAll();

		void playBGM(ResourceLoader &loader, const tstring &fileName);
		void stopBGM();

		void processFrame();
	};

}
