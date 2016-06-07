#include "stdafx.h"
#include "include/exceptions.h"
#include "include/util.h"
#include <psapi.h>
#include <sstream>
#include <iomanip>

namespace yappy {
namespace error {

std::string createStackTraceMsg(const std::string &msg)
{
	std::string result = msg;
	result += '\n';

	// Max count of stack trace
	const DWORD MaxStackTrace = 62;
	// Capture current back trace
	void *stackTrace[MaxStackTrace];
	USHORT numStackTrace = ::CaptureStackBackTrace(0, 62, stackTrace, nullptr);

	// Get module list of the current process
	BOOL bret = FALSE;
	HANDLE hProc = ::GetCurrentProcess();
	DWORD neededSize = 0;
	bret = ::EnumProcessModules(hProc, nullptr, 0, &neededSize);
	if (!bret) {
		return std::string("<???>");
	}
	DWORD modCount = neededSize / sizeof(HMODULE);
	std::vector<HMODULE> hMods(modCount);
	bret = ::EnumProcessModules(hProc, hMods.data(), neededSize, &neededSize);
	if (!bret) {
		return std::string("<???>");
	}
	hMods.resize(neededSize / sizeof(HMODULE));

	// Sort modules by load address
	auto hmoduleComp = [](HMODULE a, HMODULE b) {
		return reinterpret_cast<uintptr_t>(a) < reinterpret_cast<uintptr_t>(b);
	};
	std::sort(hMods.begin(), hMods.end(), hmoduleComp);

	// Get module name list
	std::vector<std::string> modNames;
	modNames.reserve(hMods.size());
	for (DWORD i = 0; i < modCount; i++) {
		char baseName[256];
		DWORD ret = ::GetModuleBaseNameA(hProc, hMods[i], baseName, sizeof(baseName));
		if (ret == 0) {
			::strcpy_s(baseName, "???");
		}
		modNames.emplace_back(baseName);
	}

	// Create string
	// [address] +diff [module base]
	for (uint32_t i = 0; i < numStackTrace; i++) {
		char str[32];
		sprintf_s(str, "%p", stackTrace[i]);
		result += str;
		result += ' ';

		// Binary search trace_element[i] from module list
		auto it = std::upper_bound(hMods.begin(), hMods.end(),
			(HMODULE)stackTrace[i], hmoduleComp);
		size_t ind = std::distance(hMods.begin(), it);
		if (ind > 0) {
			uintptr_t diff = reinterpret_cast<uintptr_t>(stackTrace[i])
				- reinterpret_cast<uintptr_t>(hMods.at(ind - 1));
			sprintf_s(str, "%p", reinterpret_cast<void *>(diff));
			result += '+';
			result += str;
			result += ' ';
			result += modNames.at(ind - 1);
		}
		else {
			result += "???";
		}
		result += "\n";
	}

	return result;
}


Win32Error::Win32Error(const std::string &msg, DWORD code) :
	runtime_error("")
{
	char *lpMsgBuf = NULL;
	DWORD ret = ::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		nullptr, code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		reinterpret_cast<LPSTR>(&lpMsgBuf), 0, nullptr);
	auto del = [](char *p) {
		::LocalFree(p);
	};
	std::unique_ptr<char, decltype(del)> pMsgBuf(lpMsgBuf, del);
	const char *info = (ret != 0) ? lpMsgBuf : "<FormatMessageA failed>";

	// "msg (0x????????: errdesc)"
	std::stringstream ss;
	ss << msg << " (0x";
	ss << std::hex << std::setw(8) << std::setfill('0') << code;
	ss << ": " << info << ")";
	m_what = ss.str();
}

const char *Win32Error::what() const
{
	return m_what.c_str();
}

MmioError::MmioError(const std::string &msg, UINT code) :
	runtime_error("")
{
	std::stringstream ss;
	ss << msg << " (" << code << ")";
	m_what = ss.str();
}

const char *MmioError::what() const
{
	return m_what.c_str();
}

OggVorbisError::OggVorbisError(const std::string &msg, int code) :
	runtime_error("")
{
	// signed decimal
	std::stringstream ss;
	ss << msg << " (" << code << ")";
	m_what = ss.str();
}

const char *OggVorbisError::what() const
{
	return m_what.c_str();
}

DXError::DXError(const std::string &msg, HRESULT hr) :
	runtime_error("")
{
	// "msg (0x????????)"
	std::stringstream ss;
	ss << msg << " (0x";
	ss << std::hex << std::setw(8) << std::setfill('0') << hr;
	ss << ")";
	m_what = ss.str();
}

const char *DXError::what() const
{
	return m_what.c_str();
}

}	// namespace error
}	// namespace yappy
