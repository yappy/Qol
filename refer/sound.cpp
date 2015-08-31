#include "stdafx.h"
#include "sound.hpp"
#include "debug.hpp"
#include <cstdlib>
#include <vector>
#include <algorithm>

#include <vorbis/vorbisfile.h>
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "libogg_static.lib")
#pragma comment(lib, "libvorbis_static.lib")
#pragma comment(lib, "libvorbisfile_static.lib")

namespace dx9lib {

	const unsigned DSound::BGM_BUFFER_SIZE = 4096 * 32;

	namespace {

		void memread(const std::vector<BYTE> &src, unsigned *pos, void *buf, unsigned len)
		{
			if(*pos + len > src.size())
				throw DSoundError(_T("Invalid wav file"));
			std::memcpy(buf, &src[*pos], len);
			*pos += len;
		}

		void readWaveFile(const std::vector<BYTE> &src, WAVEFORMATEX *format, std::vector<BYTE> &out)
		{
			unsigned pos = 0;

			BYTE riff[4];
			memread(src, &pos, &riff, sizeof(riff));
			if(std::memcmp(riff, "RIFF", sizeof(riff)) != 0)
				throw DSoundError(_T("Invalid wav file"));
			DWORD riffSize;
			memread(src, &pos, &riffSize, sizeof(riffSize));

			BYTE type[4];
			memread(src, &pos, &type, sizeof(type));
			if(std::memcmp(type, "WAVE", sizeof(type)) != 0)
				throw DSoundError(_T("Invalid wav file"));

			memread(src, &pos, &riff, sizeof(riff));
			if(std::memcmp(riff, "fmt ", sizeof(riff)) != 0)
				throw DSoundError(_T("Invalid wav file"));
			memread(src, &pos, &riffSize, sizeof(riffSize));
			if(riffSize > sizeof(WAVEFORMATEX))
				throw DSoundError(_T("Invalid wav file"));
			ZeroMemory(format, sizeof(WAVEFORMATEX));
			memread(src, &pos, format, riffSize);

			memread(src, &pos, &riff, sizeof(riff));
			if(std::memcmp(riff, "data", sizeof(riff)) != 0)
				throw DSoundError(_T("Invalid wav file"));
			memread(src, &pos, &riffSize, sizeof(riffSize));
			out.resize(riffSize);
			memread(src, &pos, &out.front(), riffSize);
		}

	}

	namespace {

		struct ovf_deleter {
			void operator()(OggVorbis_File *p)
			{
				ov_clear(p);
				delete p;
			}
		};

		void get_pcm(OggVorbis_File *ovf, void *buf, unsigned len, double loopPos)
		{
			unsigned pos = 0;
			while(1) {
				unsigned request = std::min(4096u, len - pos);
				int bitstream;
				long ret = ov_read(ovf, static_cast<char *>(buf) + pos, request, 0, 2, 1, &bitstream);
				// EOF
				if (ret == 0) {
					ov_time_seek(ovf, loopPos);
				}
				pos += ret;
				if (pos >= len) {
					break;
				}
			}
		}

		class OggInMemory : dx9lib::Uncopyable {
		private:
			std::vector<BYTE> m_data;
			long m_pos;

			static size_t read(void *buffer, size_t size, size_t maxCount, void *user);
			static int seek(void *user, __int64 offset, int flag);
			static long tell(void *user);
			static int close(void *user);
		public:
			OggInMemory(const std::vector<BYTE> &data) : m_data(data), m_pos(0) {}
			shared_ptr<OggVorbis_File> open();
		};

		shared_ptr<OggVorbis_File> OggInMemory::open()
		{
			shared_ptr<OggVorbis_File> povf(new OggVorbis_File, ovf_deleter());
			ov_callbacks callbacks = {
				read, seek, close, tell
			};
			if (ov_open_callbacks(this, povf.get(), 0, 0, callbacks) != 0)
				throw DSoundError(_T("ov_open_callbacks failed"));
			return povf;
		}

		size_t OggInMemory::read(void *buffer, size_t size, size_t maxCount, void *user)
		{
			OggInMemory *obj = static_cast<OggInMemory *>(user);
			int resSize = obj->m_data.size() - obj->m_pos;
			size_t count = resSize / size;
			count = std::min(count, maxCount);
			if (count == 0) {
				return 0;
			}
			std::memcpy(buffer, &obj->m_data[obj->m_pos], size * count);
			obj->m_pos += size * count;
			return count;
		}

		int OggInMemory::seek(void *user, __int64 offset, int flag)
		{
			OggInMemory *obj = static_cast<OggInMemory *>(user);
			switch (flag) {
			case SEEK_CUR:
				obj->m_pos += static_cast<long>(offset);
				break;
			case SEEK_END:
				obj->m_pos = obj->m_data.size() + static_cast<long>(offset);
				break;
			case SEEK_SET:
				obj->m_pos = static_cast<long>(offset);
				break;
			default:
				return -1;
			}
			if (obj->m_pos > static_cast<long>(obj->m_data.size())) {
				obj->m_pos = obj->m_data.size();
				return -1;
			} else if (obj->m_pos < 0) {
				obj->m_pos = 0;
				return -1;
			}
			return 0;
		}

		long OggInMemory::tell(void *user)
		{
			OggInMemory *obj = static_cast<OggInMemory *>(user);
			return obj->m_pos;
		}

		int OggInMemory::close(void *user)
		{
			OggInMemory *obj = static_cast<OggInMemory *>(user);
			delete obj;
			return 0;
		}

	}

	DSound::DSound(HWND hwnd) : m_bgmFlip(false)
	{
		debug.println("Initializing DirectSound...");

		IDirectSound8 *pDS = 0;
		DSoundError::check(DirectSoundCreate8(NULL, &pDS, NULL));
		m_pds.reset(pDS, iunknown_deleter());
		DSoundError::check(m_pds->SetCooperativeLevel(hwnd, DSSCL_NORMAL));

		debug.println("Initializing DirectSound OK.");
	}

	shared_ptr<IDirectSoundBuffer8> DSound::createSoundBuffer(const std::vector<BYTE> &src)
	{
		WAVEFORMATEX waveFormat;
		std::vector<BYTE> buffer;
		readWaveFile(src, &waveFormat, buffer);

		DSBUFFERDESC desc = {0};
		desc.dwSize = sizeof(desc);
		desc.dwBufferBytes = buffer.size();
		desc.lpwfxFormat = &waveFormat;
		desc.guid3DAlgorithm = GUID_NULL;

		IDirectSoundBuffer *ptmp;
		IDirectSoundBuffer8 *ptmp8;
		DSoundError::check(m_pds->CreateSoundBuffer(&desc, &ptmp, NULL));
		DSoundError::check(
			ptmp->QueryInterface(IID_IDirectSoundBuffer8 , reinterpret_cast<void **>(&ptmp8))
		);
		ptmp->Release();

		shared_ptr<IDirectSoundBuffer8> pBuffer(ptmp8, iunknown_deleter());
		void *pbuf;
		DWORD size;
		DSoundError::check(
			pBuffer->Lock(0, 0, &pbuf, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER)
		);
		std::memcpy(pbuf, &buffer.front(), std::min<unsigned>(buffer.size(), size));
		DSoundError::check(
			pBuffer->Unlock(pbuf, size, NULL, 0)
		);

		return pBuffer;
	}

	shared_ptr<SoundEffect> DSound::load(ResourceLoader &loader, const tstring &fileName)
	{
		std::vector<BYTE> src;
		loader.readFile(src, fileName);
		shared_ptr<IDirectSoundBuffer8> pbuf = createSoundBuffer(src);
		return shared_ptr<SoundEffect>(new SoundEffect(pbuf));
	}

	void DSound::play(shared_ptr<SoundEffect> se)
	{
		shared_ptr<IDirectSoundBuffer8> porg = se->getBuffer();
		IDirectSoundBuffer *ptmp;
		IDirectSoundBuffer8 *ptmp8;
		DSoundError::check(m_pds->DuplicateSoundBuffer(porg.get(), &ptmp));
		DSoundError::check(
			ptmp->QueryInterface(IID_IDirectSoundBuffer8 , reinterpret_cast<void **>(&ptmp8))
		);
		ptmp->Release();
		shared_ptr<IDirectSoundBuffer8> pcopy(ptmp8, iunknown_deleter());
		DSoundError::check(pcopy->Play(0, 0, 0));
		m_playing.push_back(pcopy);
	}

	void DSound::playBGM(ResourceLoader &loader, const tstring &fileName)
	{
		stopBGM();

		std::vector<BYTE> src;
		loader.readFile(src, fileName);

		OggInMemory *pogg = new OggInMemory(src);
		m_povf = pogg->open();
		vorbis_info *info = ov_info(m_povf.get(), -1);

		WAVEFORMATEX waveFormat = {0};
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = info->channels;
		waveFormat.nSamplesPerSec = info->rate;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		DSBUFFERDESC desc = {0};
		desc.dwSize = sizeof(desc);
		desc.dwBufferBytes = BGM_BUFFER_SIZE * 2;
		desc.lpwfxFormat = &waveFormat;
		desc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		desc.guid3DAlgorithm = GUID_NULL;

		IDirectSoundBuffer *ptmp;
		IDirectSoundBuffer8 *ptmp8;
		DSoundError::check(m_pds->CreateSoundBuffer(&desc, &ptmp, NULL));
		DSoundError::check(
			ptmp->QueryInterface(IID_IDirectSoundBuffer8 , reinterpret_cast<void **>(&ptmp8))
		);
		ptmp->Release();

		shared_ptr<IDirectSoundBuffer8> pBuffer(ptmp8, iunknown_deleter());
		void *pbuf;
		DWORD size;
		DSoundError::check(
			pBuffer->Lock(0, 0, &pbuf, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER)
		);
		get_pcm(m_povf.get(), pbuf, size, 0.0);

		DSoundError::check(
			pBuffer->Unlock(pbuf, size, NULL, 0)
		);
		DSoundError::check(pBuffer->Play(0, 0, DSBPLAY_LOOPING));

		m_pbgmBuf = pBuffer;
	}

	void DSound::stopBGM()
	{
		m_bgmFlip = false;
		if (m_pbgmBuf) {
			m_pbgmBuf.reset();
			m_povf.reset();
		}
	}

	namespace {
		struct BufferRemoveFunc : std::unary_function<shared_ptr<IDirectSoundBuffer8>, bool>
		{
			bool operator ()(shared_ptr<IDirectSoundBuffer8> &pbuf)
			{
				DWORD status;
				DSoundError::check(pbuf->GetStatus(&status));
				return !(status & DSBSTATUS_PLAYING);
			}
		};
	}
	void DSound::processFrame()
	{
		// SE
		m_playing.remove_if(BufferRemoveFunc());

		// BGM
		if (m_pbgmBuf) {
			DWORD cur;
			void *p1, *p2;
			DWORD s1, s2;
			DSoundError::check(m_pbgmBuf->GetCurrentPosition(&cur, NULL));
			if (!m_bgmFlip && cur >= BGM_BUFFER_SIZE) {
				DSoundError::check(m_pbgmBuf->Lock(0, BGM_BUFFER_SIZE, &p1, &s1, &p2, &s2, 0));
				get_pcm(m_povf.get(), p1, s1, 0.0f);
				DSoundError::check(m_pbgmBuf->Unlock(p1, s1, p2, s2));
				m_bgmFlip = true;
			} else if(m_bgmFlip && cur < BGM_BUFFER_SIZE) {
				DSoundError::check(m_pbgmBuf->Lock(BGM_BUFFER_SIZE, BGM_BUFFER_SIZE, &p1, &s1, &p2, &s2, 0));
				get_pcm(m_povf.get(), p1, s1, 0.0f);
				DSoundError::check(m_pbgmBuf->Unlock(p1, s1, p2, s2));
				m_bgmFlip = false;
			}
		}
		
	}

	namespace {
		struct StopFunc {
			void operator ()(shared_ptr<IDirectSoundBuffer8> &pbuf)
			{
				pbuf->Stop();
			}
		};
	}
	void DSound::stopAll()
	{
		std::for_each(m_playing.begin(), m_playing.end(), StopFunc());
		m_playing.clear();
	}

}
