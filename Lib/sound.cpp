#include "stdafx.h"
#include "include/sound.h"
#include "include/file.h"
#include "include/debug.h"
#include "include/exceptions.h"
#include <mmsystem.h>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

namespace test {
namespace sound {

using error::MmioError;
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

void XAudio2::stopAllSoundEffect()
{
	for (auto &p : m_playingSeList) {
		p.reset();
	}
}

XAudio2::SourceVoicePtr *XAudio2::findFreeSeEntry()
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

}
}
