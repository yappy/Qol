#include "stdafx.h"
#include "include/file.h"
#include "include/exceptions.h"
#include <windows.h>
#include <memory>
#include <string>

namespace test {
namespace file {

namespace {

class FileLoader : private test::util::noncopyable {
public:
	FileLoader() = default;
	virtual ~FileLoader() {}
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
	else {
		path += m_rootDir;
		path += L'/';
		path += fileName;
	}

	// open
	HANDLE tmphFile = ::CreateFile(
		path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	error::checkWin32Result(tmphFile != INVALID_HANDLE_VALUE, "CreateFile() failed");
	std::unique_ptr<HANDLE, util::handleDeleter> hFile(tmphFile);

	BOOL b;
	// get size for avoiding realloc
	LARGE_INTEGER fileSize;
	b = ::GetFileSizeEx(hFile.get(), &fileSize);
	error::checkWin32Result(b != 0, "GetFileSizeEx() failed");
	error::checkWin32Result(fileSize.HighPart == 0, "File size is too large");
	// read
	Bytes data(fileSize.LowPart);
	DWORD readSize;
	b = ::ReadFile(hFile.get(), &data[0], fileSize.LowPart, &readSize, nullptr);
	error::checkWin32Result(b != 0, "ReadFile() failed");
	error::checkWin32Result(fileSize.LowPart == readSize, "Read size is strange");

	// move return
	return data;
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
}	// namespace test
