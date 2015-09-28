#pragma once

#include <memory>
#include <string>
#include <windows.h>
#include <Unknwn.h>

namespace yappy {
namespace util {

class noncopyable {
public:
	noncopyable() = default;
	noncopyable(const noncopyable &) = delete;
	noncopyable &operator=(const noncopyable &) = delete;
};


struct handleDeleter {
	using pointer = HANDLE;
	void operator()(HANDLE h)
	{
		if (h != INVALID_HANDLE_VALUE) {
			::CloseHandle(h);
		}
	}
};
using HandlePtr = std::unique_ptr<HANDLE, handleDeleter>;

inline void iunknownDeleter(IUnknown *iu)
{
	iu->Release();
}
typedef decltype(&iunknownDeleter) IUnknownDeleterType;
template<class T>
using IUnknownPtr = std::unique_ptr<T, IUnknownDeleterType>;


inline std::string wc2utf8(const wchar_t *in)
{
	int len = ::WideCharToMultiByte(CP_UTF8, 0, in, -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<char[]> pBuf(new char[len]);
	::WideCharToMultiByte(CP_UTF8, 0, in, -1, pBuf.get(), len, nullptr, nullptr);
	return std::string(pBuf.get());
}

inline std::wstring utf82wc(const char *in)
{
	int len = ::MultiByteToWideChar(CP_UTF8, 0, in, -1, nullptr, 0);
	std::unique_ptr<wchar_t[]> pBuf(new wchar_t[len]);
	::MultiByteToWideChar(CP_UTF8, 0, in, -1, pBuf.get(), len);
	return std::wstring(pBuf.get());
}


class Com : private noncopyable {
public:
	Com() { ::CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
	~Com() { ::CoUninitialize(); }
};

}
}
