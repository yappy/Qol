﻿#pragma once

#include <stdexcept>
#include <memory>
#include <windows.h>

namespace yappy {
/// Exceptions and utilities.
namespace error {

/// Max count of stack trace.
const DWORD MaxStackTrace = 62;

/** @brief Internal use.
 * @details Convert (pointer array) to (string).
 * @param[in]	stackTrace	Pointer array.
 * @param[in]	count		Array size.
 * @return					String.
 */
std::string formatStackTrace(void *(&stackTrace)[MaxStackTrace], uint32_t count);

/** @brief Returns (msg + stacktrace) string.
 * @param[in]	msg	Original string.
 * @return			msg + stacktrace.
 */
__forceinline std::string createStackTraceMsg(const std::string &msg)
{
	// [inline] Capture current back trace
	void *stackTrace[MaxStackTrace];
	USHORT num = ::CaptureStackBackTrace(0, 62, stackTrace, nullptr);

	std::string result = msg;
	result += '\n';
	result += formatStackTrace(stackTrace, num);
	return result;
}


class Win32Error : public std::runtime_error {
public:
	Win32Error(const std::string &msg, DWORD code);
	const char *what() const override;
private:
	std::string m_what;
};

inline void checkWin32Result(bool cond, const std::string &msg)
{
	if (!cond) {
		throw Win32Error(msg, ::GetLastError());
	}
}

class WinSockError : public Win32Error {
public:
	WinSockError(const std::string &msg, int code) :
		Win32Error(msg, static_cast<DWORD>(code))
	{}
};


class MmioError : public std::runtime_error {
public:
	MmioError(const std::string &msg, UINT code);
	const char *what() const override;
private:
	std::string m_what;
};

class OggVorbisError : public std::runtime_error {
public:
	OggVorbisError(const std::string &msg, int code);
	const char *what() const override;
private:
	std::string m_what;
};


template <class T>
inline void checkDXResult(HRESULT hr, const std::string &msg)
{
	static_assert(std::is_base_of<DXError, T>::value, "T must inherit DXError");
	if (FAILED(hr)) {
		throw T(msg, hr);
	}
}

class DXError : public std::runtime_error {
public:
	DXError(const std::string &msg, HRESULT hr);
	const char *what() const override;
private:
	std::string m_what;
};

class D3DError : public DXError {
public:
	D3DError(const std::string &msg, HRESULT hr) :
		DXError(msg, hr)
	{}
};

class DIError : public DXError {
public:
	DIError(const std::string &msg, HRESULT hr) :
		DXError(msg, hr)
	{}
};

class DSError : public DXError {
public:
	DSError(const std::string &msg, HRESULT hr) :
		DXError(msg, hr)
	{}
};

class XAudioError : public DXError {
public:
	XAudioError(const std::string &msg, HRESULT hr) :
		DXError(msg, hr)
	{}
};

}
}
