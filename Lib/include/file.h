#pragma once

#include "util.h"
#include <vector>

namespace test {
namespace file {

void initWithFileSystem(const wchar_t *rootDir);
void initWithArchiveFile(const wchar_t *archiveFile);

using Bytes = std::vector<uint8_t>;
Bytes loadFile(const wchar_t *fileName);

}
}
