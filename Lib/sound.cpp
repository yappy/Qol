#include "stdafx.h"
#include "include/sound.h"
#include "include/debug.h"
#include "include/exceptions.h"
#include <algorithm>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace test {
namespace sound {

using error::MmioError;
using error::OggVorbisError;
using error::checkDXResult;
using error::XAudioError;

namespace {

void loadWaveFile(SoundEffect *out, const wchar_t *path)
{
	file::Bytes data = file::loadFile(path);

	MMIOINFO mmioInfo = { 0 };
	mmioInfo.pchBuffer = reinterpret_cast<HPSTR>(&data[0]);
	mmioInfo.fccIOProc = FOURCC_MEM;
	mmioInfo.cchBuffer = static_cast<LONG>(data.size());

	HMMIO tmpHMmio = mmioOpen(0, &mmioInfo, MMIO_READ);
	if (tmpHMmio == nullptr) {
		throw MmioError("mmioOpen() failed", mmioInfo.wErrorRet);
	}
	std::unique_ptr<HMMIO, hmmioDeleter> hMmio(tmpHMmio);

	// enter "WAVE"
	MMRESULT mmRes;
	MMCKINFO riffChunk = { 0 };
	riffChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	mmRes = mmioDescend(hMmio.get(), &riffChunk, nullptr, MMIO_FINDRIFF);
	if (mmRes != MMSYSERR_NOERROR) {
		throw MmioError("mmioDescend() failed", mmRes);
	}
	// enter "fmt "
	MMCKINFO formatChunk = { 0 };
	formatChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmRes = mmioDescend(hMmio.get(), &formatChunk, &riffChunk, MMIO_FINDCHUNK);
	if (mmRes != MMSYSERR_NOERROR) {
		throw MmioError("mmioDescend() failed", mmRes);
	}
	// read "fmt "
	::ZeroMemory(&out->format, sizeof(out->format));
	DWORD fmtSize = std::min(formatChunk.cksize, static_cast<DWORD>(sizeof(out->format)));
	DWORD size = mmioRead(hMmio.get(), reinterpret_cast<HPSTR>(&out->format), fmtSize);
	if (size != fmtSize) {
		throw MmioError("mmioRead() failed", size);
	}
	// leave "fmt "
	mmRes = mmioAscend(hMmio.get(), &formatChunk, 0);
	if (mmRes != MMSYSERR_NOERROR) {
		throw MmioError("mmioAscend() failed", mmRes);
	}
	// enter "data"
	MMCKINFO dataChunk = { 0 };
	dataChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmRes = mmioDescend(hMmio.get(), &dataChunk, &riffChunk, MMIO_FINDCHUNK);
	if (mmRes != MMSYSERR_NOERROR) {
		throw MmioError("mmioDescend() failed", mmRes);
	}
	// read "data"
	if (dataChunk.cksize > SoundFileSizeMax) {
		throw MmioError("data size too large", 0);
	}
	out->samples.resize(dataChunk.cksize);
	size = mmioRead(hMmio.get(), reinterpret_cast<HPSTR>(&out->samples[0]), dataChunk.cksize);
	if (size != dataChunk.cksize) {
		throw MmioError("mmioRead() failed", size);
	}
}

}	// namespace

XAudio2::XAudio2() :
	m_pIXAudio(nullptr, util::iunknownDeleter),
	m_pMasterVoice(nullptr, voiceDeleter),
	m_pBgmVoice(nullptr, voiceDeleter),
	m_pBgmBuffer(new char[BgmBufferSize * BgmBufferCount]),
	m_pBgmFile(nullptr, oggFileDeleter)
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
	checkDXResult<XAudioError>(hr, "IXAudio2::CreateMasteringVoice() failed");
	m_pMasterVoice.reset(ptmpMasterVoice);

	// m_playingSeList[SoundEffectPlayMax] filled by nullptr
	m_playingSeList.reserve(SoundEffectPlayMax);
	for (size_t i = 0; i < SoundEffectPlayMax; i++) {
		m_playingSeList.emplace_back(SourceVoicePtr(nullptr, voiceDeleter));
	}
}

XAudio2::~XAudio2() {}

void XAudio2::loadSoundEffect(const char *id, const wchar_t *path)
{
	// m_selib[id] <- SoundEffect()
	auto res = m_seMap.emplace(std::piecewise_construct,
		std::forward_as_tuple(id), std::forward_as_tuple());
	if (!res.second) {
		throw std::runtime_error("id already exists");
	}
	// ((key, val), success?).first.second
	SoundEffect &value = res.first->second;
	try {
		loadWaveFile(&value, path);
	}
	catch (...) {
		m_seMap.erase(res.first);
		throw;
	}
}

void XAudio2::playSoundEffect(const char *id)
{
	// find (WAVEFORMATEX, SampleBytes) entry
	auto res = m_seMap.find(id);
	if (res == m_seMap.end()) {
		throw std::runtime_error("id not found");
	}
	SoundEffect &se = res->second;

	// find playing src voice list entry
	SourceVoicePtr *ppEntry = findFreeSeEntry();
	if (ppEntry == nullptr) {
		debug::writeLine("Warning: SE playing list is full!");
		return;
	}
	ASSERT(*ppEntry == nullptr);

	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = static_cast<UINT32>(se.samples.size());
	buffer.pAudioData = &se.samples[0];
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	// create source voice
	HRESULT hr;
	IXAudio2SourceVoice *ptmpSrcVoice;
	hr = m_pIXAudio->CreateSourceVoice(&ptmpSrcVoice, &se.format);
	checkDXResult<XAudioError>(hr, "IXAudio2::CreateSourceVoice() failed");

	// submit source buffer
	std::unique_ptr<IXAudio2SourceVoice, VoiceDeleterType>
		pSrcVoice(ptmpSrcVoice, voiceDeleter);
	hr = pSrcVoice->SubmitSourceBuffer(&buffer);
	checkDXResult<XAudioError>(hr, "IXAudio2SourceVoice::SubmitSourceBuffer() failed");
	hr = pSrcVoice->Start();
	checkDXResult<XAudioError>(hr, "IXAudio2SourceVoice::Start() failed");

	// set source voice element
	*ppEntry = std::move(pSrcVoice);
}

bool XAudio2::isPlayingAny() const noexcept
{
	for (const auto &pSrcVoice : m_playingSeList) {
		if (pSrcVoice == nullptr) {
			continue;
		}
		// check if playing is end
		XAUDIO2_VOICE_STATE state;
		pSrcVoice->GetState(&state);
		if (state.BuffersQueued != 0) {
			return false;
		}
	}
	return true;
}

void XAudio2::stopAllSoundEffect()
{
	for (auto &pSrcVoice : m_playingSeList) {
		pSrcVoice.reset();
	}
}

XAudio2::SourceVoicePtr *XAudio2::findFreeSeEntry() noexcept
{
	for (auto &p : m_playingSeList) {
		if (p == nullptr) {
			return &p;
		}
		// check if playing is end
		XAUDIO2_VOICE_STATE state;
		p->GetState(&state);
		if (state.BuffersQueued == 0) {
			// DestroyVoice(), set nullptr, return
			p.reset();
			return &p;
		}
	}
	return nullptr;
}


void XAudio2::playBgm(const wchar_t *path)
{
	int ret;
	HRESULT hr;

	debug::writef(L"playBgm: %s", path);

	stopBgm();

	m_ovFileBin = file::loadFile(path);
	m_readPos = 0;

	// ovfile open
	static OggVorbis_File fp;
	ov_callbacks callbacks = { read, seek, close, tell };
	ret = ::ov_open_callbacks(this, &fp, nullptr, 0, callbacks);
	if (ret != 0) {
		throw OggVorbisError("ov_open_callbacks() failed", ret);
	}
	m_pBgmFile.reset(&fp);
	// ovinfo
	vorbis_info *info = ov_info(m_pBgmFile.get(), -1);
	if (info == nullptr) {
		throw OggVorbisError("ov_info() failed", 0);
	}
	debug::writef(L"ov_info: channels=%d, rate=%ld", info->channels, info->rate);
	// WAVEFORMAT from ovinfo
	WAVEFORMATEX format = { 0 };
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = info->channels;
	format.nSamplesPerSec = info->rate;
	format.wBitsPerSample = 16;
	format.nBlockAlign = info->channels * format.wBitsPerSample / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	format.cbSize = 0;

	// create source voice using WAVEFORMAT
	IXAudio2SourceVoice *ptmpSrcVoice;
	hr = m_pIXAudio->CreateSourceVoice(&ptmpSrcVoice, &format);
	checkDXResult<XAudioError>(hr, "IXAudio2::CreateSourceVoice() failed");
	m_pBgmVoice.reset(ptmpSrcVoice);

	hr = m_pBgmVoice->Start();
	checkDXResult<XAudioError>(hr, "IXAudio2SourceVoice::Start() failed");

	debug::writeLine(L"playBgm OK");
}

void XAudio2::stopBgm() noexcept
{
	// DestroyVoice(), set nullptr
	m_pBgmVoice.reset();
	m_pBgmFile.reset();
}

void XAudio2::processFrame()
{
	HRESULT hr;
	if (m_pBgmVoice == nullptr) {
		return;
	}
	XAUDIO2_VOICE_STATE state;
	m_pBgmVoice->GetState(&state);
	if (state.BuffersQueued >= BgmBufferCount) {
		debug::writeLine(L"not yet!");
		return;
	}

	long readSum = 0;
	size_t base = m_writePos * BgmBufferSize;
	while (BgmBufferSize - readSum >= BgmOvReadSize) {
		long size = ::ov_read(m_pBgmFile.get(),
			&m_pBgmBuffer[base + readSum],
			BgmOvReadSize, 0, 2, 1, nullptr);
		if (size < 0) {
			throw OggVorbisError("ov_read() failed", size);
		}
		readSum += size;
		if (size == 0) {
			// stream end
			break;
		}
	}
	// TODO: readsum == 0 (stream end)

	// submit source buffer
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = static_cast<UINT32>(readSum);
	buffer.pAudioData = reinterpret_cast<BYTE *>(&m_pBgmBuffer[base]);
	hr = m_pBgmVoice->SubmitSourceBuffer(&buffer);
	checkDXResult<XAudioError>(hr, "IXAudio2SourceVoice::SubmitSourceBuffer() failed");
	debug::writeLine(L"push back!");
}

size_t XAudio2::read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	XAudio2 *obj = static_cast<XAudio2 *>(datasource);
	size_t remainSize = obj->m_ovFileBin.size() - obj->m_readPos;
	size_t count = std::min(remainSize / size, nmemb);
	::memcpy(ptr, &obj->m_ovFileBin[obj->m_readPos], size * count);
	obj->m_readPos += size * count;
	return count;
}
int XAudio2::seek(void *datasource, int64_t offset, int whence)
{
	return -1;
}
long XAudio2::tell(void *datasource)
{
	return -1;
}
int XAudio2::close(void *datasource)
{
	XAudio2 *obj = static_cast<XAudio2 *>(datasource);
	return 0;
}

}
}
