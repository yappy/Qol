#include "stdafx.h"
#include "include/exceptions.h"
#include "include/util.h"
#include "dxerr.h"

namespace test {
namespace error {

Win32Error::Win32Error(const std::string &msg, DWORD code) noexcept
	: runtime_error("")
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

const char *Win32Error::what() const {
	return m_what.c_str();
}

DXError::DXError(const std::string &msg, HRESULT hr) noexcept
	: runtime_error("")
{
	const wchar_t *name;
	wchar_t desc[1024];
	name = DXGetErrorString(hr);
	::DXGetErrorDescription(hr, desc, _countof(desc));
	if (desc[0] == L'\0') {
		wcscpy_s(desc, L"<DXGetErrorDescription failed>");
	}

	// "msg (0x???????? errname: errdesc)"
	std::stringstream ss;
	ss << msg << " (0x";
	ss << std::hex << std::setw(8) << std::setfill('0') << hr;
	ss << " " << util::wc2utf8(name) << ": " << util::wc2utf8(desc) << ")";
	m_what = ss.str();
}

const char *DXError::what() const {
	return m_what.c_str();
}

}
}
