#pragma once

#include <memory>
#include <string>
#include <windows.h>

namespace test {
namespace util {

inline void handleDeleter(HANDLE h) {
	if (h != INVALID_HANDLE_VALUE) {
		::CloseHandle(h);
	}
};

inline std::string wc2utf8(const wchar_t *in) {
	int len = ::WideCharToMultiByte(CP_UTF8, 0, in, -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<char[]> pBuf(new char[len]);
	::WideCharToMultiByte(CP_UTF8, 0, in, -1, pBuf.get(), len, nullptr, nullptr);
	return std::string(pBuf.get());
}

}
}
