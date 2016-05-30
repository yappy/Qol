#include "stdafx.h"
#include "include/file.h"
#include "include/exceptions.h"
#include <windows.h>
#include <memory>
#include <array>
#include <string>

namespace yappy {
namespace file {

using error::checkWin32Result;

namespace {

class FileLoader : private util::noncopyable {
public:
	FileLoader() = default;
	virtual ~FileLoader() = default;
	virtual std::vector<uint8_t> loadFile(const wchar_t *fileName) = 0;
};

class FsFileLoader : public FileLoader {
public:
	FsFileLoader(const wchar_t *rootDir) : m_rootDir(rootDir) {}
	virtual ~FsFileLoader() override {}
	virtual std::vector<uint8_t> loadFile(const wchar_t *fileName) override;
private:
	std::wstring m_rootDir;
};

class ArchiveFileLoader : public FileLoader {
	// TODO: impl
};

// impls
Bytes FsFileLoader::loadFile(const wchar_t *fileName)
{
	std::wstring path;
	if (fileName[0] == L'/') {
		// Debug only, abs path
		path = &fileName[1];
	}
	else if (fileName[0] == L'@') {
		// Debug only, relative to executable
		std::array<wchar_t, MAX_PATH> dir;
		DWORD ret = ::GetModuleFileName(nullptr, dir.data(), static_cast<DWORD>(dir.size()));
		checkWin32Result(ret != 0, "GetModuleFileName() failed");
		*(::wcsrchr(dir.data(), L'\\') + 1) = L'\0';
		path = dir.data();
		path += &fileName[1];
	}
	else {
		path += m_rootDir;
		path += L'/';
		path += fileName;
	}

	// open
	HANDLE tmphFile = ::CreateFile(
		path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	checkWin32Result(tmphFile != INVALID_HANDLE_VALUE, "CreateFile() failed");
	util::HandlePtr hFile(tmphFile);

	BOOL b = FALSE;
	// get size to avoid realloc
	LARGE_INTEGER fileSize = { 0 };
	b = ::GetFileSizeEx(hFile.get(), &fileSize);
	checkWin32Result(b != 0, "GetFileSizeEx() failed");
	// 2GiB check
	checkWin32Result(fileSize.HighPart == 0, "File size is too large");
	checkWin32Result(fileSize.LowPart < 0x80000000, "File size is too large");
	// read
	Bytes bin(fileSize.LowPart);
	DWORD readSize = 0;
	b = ::ReadFile(hFile.get(), bin.data(), fileSize.LowPart, &readSize, nullptr);
	error::checkWin32Result(b != 0, "ReadFile() failed");
	error::checkWin32Result(fileSize.LowPart == readSize, "Read size is strange");

	// move return
	return bin;
}

// variables
std::unique_ptr<FileLoader> s_fileLoader(nullptr);

}	// namespace


void initWithFileSystem(const wchar_t *rootDir)
{
	s_fileLoader.reset(new FsFileLoader(rootDir));
}

void initWithArchiveFile(const wchar_t *archiveFile)
{
	// TODO
	throw std::logic_error("Not implemented");
}

std::vector<uint8_t> loadFile(const wchar_t *fileName)
{
	if (s_fileLoader == nullptr) {
		throw std::logic_error("FileLoader is not initialized.");
	}
	return s_fileLoader->loadFile(fileName);
}

}	// namespace file
}	// namespace yappy
