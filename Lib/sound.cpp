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
		// TODO exception
		throw std::runtime_error("mmioOpen() failed");
	}
	std::unique_ptr<HMMIO, hmmioDeleter> hMmio(tmpHMmio);

	// enter "WAVE"
	MMRESULT mmRes;
	MMCKINFO riffChunk = { 0 };
	riffChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	mmRes = mmioDescend(hMmio.get(), &riffChunk, nullptr, MMIO_FINDRIFF);
	if (mmRes != MMSYSERR_NOERROR) {
		// TODO exception
		throw std::runtime_error("mmioDescend() failed");
	}

	// enter "fmt "
	MMCKINFO formatChunk = { 0 };
	formatChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
	mmRes = mmioDescend(hMmio.get(), &formatChunk, &riffChunk, MMIO_FINDCHUNK);
	if (mmRes != MMSYSERR_NOERROR) {
		// TODO exception
		throw std::runtime_error("mmioDescend() failed");
	}

	// read "fmt "
	::ZeroMemory(&out->format, sizeof(out->format));
	DWORD fmtSize = std::min(formatChunk.cksize, static_cast<DWORD>(sizeof(out->format)));
	DWORD size = mmioRead(hMmio.get(), reinterpret_cast<HPSTR>(&out->format), fmtSize);
	if (size != fmtSize) {
		// TODO exception
		throw std::runtime_error("mmioRead() failed");
	}

	// leave "fmt "
	mmRes = mmioAscend(hMmio.get(), &formatChunk, 0);
	if (mmRes != MMSYSERR_NOERROR) {
		// TODO exception
		throw std::runtime_error("mmioAscend() failed");
	}

	// enter "data"
	MMCKINFO dataChunk = { 0 };
	dataChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
	mmRes = mmioDescend(hMmio.get(), &dataChunk, &riffChunk, MMIO_FINDCHUNK);
	if (mmRes != MMSYSERR_NOERROR) {
		// TODO exception
		throw std::runtime_error("mmioDescend() failed");
	}

	// read "data"
	if (dataChunk.cksize > SoundFileSizeMax) {
		// TODO exception
		throw std::runtime_error("data size too large");
	}
	out->samples.resize(dataChunk.cksize);
	size = mmioRead(hMmio.get(), reinterpret_cast<HPSTR>(&out->samples[0]), dataChunk.cksize);
	if (size != dataChunk.cksize) {
		// TODO exception
		throw std::runtime_error("mmioRead() failed");
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
	checkDXResult<XAudioError>(hr, "CreateMasteringVoice() failed");
	m_pMasterVoice.reset(ptmpMasterVoice);

	/*
	for (int i = 0; i < 1000; i++) {
		IXAudio2SourceVoice *ptmpSrcVoice;
		WAVEFORMATEX fmt = { WAVE_FORMAT_PCM, 1, 44100, 44100, 1, 8, 0 };
		{
			debug::StopWatch(L"CreateSourceVoice()");
			m_pIXAudio->CreateSourceVoice(&ptmpSrcVoice, &fmt);
		}

		XAUDIO2_BUFFER buffer = { 0 };
		{
			debug::StopWatch(L"vector()");
			std::vector<BYTE> bytes(44100 * 5);
			buffer.AudioBytes = static_cast<UINT32>(bytes.size());
			buffer.pAudioData = &bytes.at(0);
			buffer.Flags = XAUDIO2_END_OF_STREAM;
		}
		{
			debug::StopWatch(L"SubmitSourceBuffer(44.1kHz*5sec)");
			ptmpSrcVoice->SubmitSourceBuffer(&buffer);
		}
		ptmpSrcVoice->DestroyVoice();
	}
	*/
}

XAudio2::~XAudio2() {}

void XAudio2::loadSound(const char *id, const wchar_t *path)
{
	// m_selib[id] <- SoundEffect()
	auto res = m_selib.emplace(std::piecewise_construct,
		std::forward_as_tuple(id), std::forward_as_tuple());
	if (!res.second) {
		throw std::runtime_error("id already exists");
	}
	// ((key, val), success?).first.second
	SoundEffect &value = res.first->second;
	try {
		loadWaveFile(&value, path);
	}
	catch (std::exception) {
		m_selib.erase(res.first);
		throw;
	}
}

void XAudio2::playSound(const char *id)
{
	auto res = m_selib.find(id);
	if (res == m_selib.end()) {
		throw std::runtime_error("id not found");
	}
	SoundEffect &se = res->second;
	XAUDIO2_BUFFER buf = { 0 };
	buf.AudioBytes = se.samples.size();
	buf.pAudioData = &se.samples[0];

	IXAudio2SourceVoice *ptmpSrcVoice;
	m_pIXAudio->CreateSourceVoice(&ptmpSrcVoice, &se.format);
	ptmpSrcVoice->SubmitSourceBuffer(&buf);
	ptmpSrcVoice->Start();
	Sleep(3000);
	ptmpSrcVoice->DestroyVoice();
}

}
}
