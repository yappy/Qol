/** @file
 * @brief Utilities.
 */

#pragma once

#include <memory>
#include <array>
#include <string>
#include <windows.h>
#include <Unknwn.h>

namespace yappy {
/// Utilities.
namespace util {

/** @brief Noncopyable class.
 * @details Delete copy constructor and assignment operator.
 * @code
 * // How to use:
 * class MyClass : private noncopyable { ... };
 * @endcode
 */
class noncopyable {
public:
	noncopyable() = default;
	noncopyable(const noncopyable &) = delete;
	noncopyable &operator=(const noncopyable &) = delete;
};


/** @brief Fixed size char array for string ID.
 * @see std::hash<yappy::util::IdString>
 */
using IdString = std::array<char, 16>;

/** @brief Create yappy::util::IdString from C-style string.
 */
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


/// Deleter: auto CloseHandle().
struct HandleDeleter {
	using pointer = HANDLE;
	void operator()(HANDLE h)
	{
		if (h != INVALID_HANDLE_VALUE) {
			::CloseHandle(h);
		}
	}
};
/// unique_ptr of HANDLE with HandleDeleter.
using HandlePtr = std::unique_ptr<HANDLE, HandleDeleter>;

/// Deleter: auto HeapDestroy().
struct heapDeleter {
	using pointer = HANDLE;
	void operator()(HANDLE h)
	{
		if (h != INVALID_HANDLE_VALUE) {
			::HeapDestroy(h);
		}
	}
};
/// unique_ptr of HANDLE with heapDeleter.
using HeapPtr = std::unique_ptr<HANDLE, heapDeleter>;

/// Deleter: auto IUnknown::Release().
struct ComDeleter {
	void operator()(IUnknown *p)
	{
		p->Release();
	}
};
/// unique_ptr of IUnknown with ComDeleter.
template<class T>
using ComPtr = std::unique_ptr<T, ComDeleter>;

/// Deleter: auto flose().
struct FileDeleter {
	void operator()(FILE *fp)
	{
		::fclose(fp);
	}
};
/// unique_ptr of FILE with FileDeleter.
using FilePtr = std::unique_ptr<FILE, FileDeleter>;


/** @brief Wide char to UTF-8.
 * @param[in] in	Wide string.
 * @return UTF-8 string.
 */
inline std::unique_ptr<char[]> wc2utf8(const wchar_t *in)
{
	int len = ::WideCharToMultiByte(CP_UTF8, 0, in, -1, nullptr, 0, nullptr, nullptr);
	std::unique_ptr<char[]> pBuf(new char[len]);
	::WideCharToMultiByte(CP_UTF8, 0, in, -1, pBuf.get(), len, nullptr, nullptr);
	return pBuf;
}

/** @brief UTF-8 to wide char.
 * @param[in] in	UTF-8 string.
 * @return Wide char string.
 */
inline std::unique_ptr<wchar_t[]> utf82wc(const char *in)
{
	int len = ::MultiByteToWideChar(CP_UTF8, 0, in, -1, nullptr, 0);
	std::unique_ptr<wchar_t[]> pBuf(new wchar_t[len]);
	::MultiByteToWideChar(CP_UTF8, 0, in, -1, pBuf.get(), len);
	return pBuf;
}

/** @brief Auto CoInitializeEx() and CoUninitialize() class.
*/
class CoInitialize : private noncopyable {
public:
	/// CoInitializeEx(nullptr, COINIT_MULTITHREADED)
	CoInitialize() { ::CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
	/// CoUninitialize()
	~CoInitialize() { ::CoUninitialize(); }
};

}	// namespace util
}	// namespace yappy

namespace std {

/** @brief Hash function object of yappy::util::IdString for std::unordered_map.
 */
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

}	// namespace std
