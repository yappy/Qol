#pragma once

#include "util.h"
#include <vector>

namespace test {
namespace file {

void initWithFileSystem(const wchar_t *rootDir);
void initWithArchiveFile(const wchar_t *archiveFile);

std::vector<uint8_t> loadFile(const wchar_t *fileName);

}
}
