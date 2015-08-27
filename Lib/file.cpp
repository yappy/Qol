#include "stdafx.h"
#include "include/file.h"
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
	virtual ~FsFileLoader() override;
	virtual std::vector<uint8_t> loadFile(const wchar_t *fileName) override;
private:
	std::wstring m_rootDir;
};

class ArchiveFileLoader : public FileLoader {

};

std::unique_ptr<FileLoader> s_fileLoader(nullptr);

}	// namespace

void initWithFileSystem(const wchar_t *rootDir) {
	s_fileLoader.reset(new FsFileLoader(rootDir));
}

void initWithArchiveFile(const wchar_t *archiveFile) {
	// TODO
	throw std::logic_error("Not implemented");
}

std::vector<uint8_t> loadFile(const wchar_t *fileName) {
	if (!s_fileLoader) {
		throw std::logic_error("FileLoader is not initialized.");
	}
	// TODO
	return std::vector<uint8_t>();
}

}	// namespace file
}	// namespace test
