#pragma once

#include <memory>
#include <array>
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


using IdString = std::array<char, 16>;

template <size_t N>
inline void createFixedString(std::array<char, N> *out, const char *src) {
	size_t len = std::strlen(src);
	if (len >= N) {
		throw std::invalid_argument(std::string("String size too long: ") + src);
	}
	// copy non-'\0' part
	std::memcpy(out->data(), src, len);
	// fills the remainder with '\0'
	std::memset(out->data() + len, '\0', N - len);
}


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
template<class T>
using IUnknownSharedPtr = std::shared_ptr<T>;

inline void fileDeleter(FILE *fp)
{
	::fclose(fp);
}
using FilePtr = std::unique_ptr<FILE, decltype(&fileDeleter)>;


inline std::unique_ptr<char[]> wc2utf8(const wchar_t *in)
{
	int len = ::WideCharToMultiByte(CP_UTF8, 0, in, -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<char[]> pBuf(new char[len]);
	::WideCharToMultiByte(CP_UTF8, 0, in, -1, pBuf.get(), len, nullptr, nullptr);
	return pBuf;
}

inline std::unique_ptr<wchar_t[]> utf82wc(const char *in)
{
	int len = ::MultiByteToWideChar(CP_UTF8, 0, in, -1, nullptr, 0);
	std::unique_ptr<wchar_t[]> pBuf(new wchar_t[len]);
	::MultiByteToWideChar(CP_UTF8, 0, in, -1, pBuf.get(), len);
	return pBuf;
}

class Com : private noncopyable {
public:
	Com() { ::CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
	~Com() { ::CoUninitialize(); }
};

}
}

namespace std {

template <>
struct hash<yappy::util::IdString> {
	size_t operator()(const yappy::util::IdString &key) const
	{
		size_t hashCode = 0;
		const char *p = key.data();
		for (size_t i = 0; i < key.size(); i++) {
			hashCode = hashCode * 31 + p[i];
		}
		return hashCode;
	}
};

}

