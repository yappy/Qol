#pragma once

#include "util.h"
#include <vector>
#include <limits>

namespace yappy {
namespace file {

// 0x7fffffff = 2GiB
const uint32_t FileSizeMax = std::numeric_limits<int32_t>::max();

void initWithFileSystem(const wchar_t *rootDir);
void initWithArchiveFile(const wchar_t *archiveFile);

using Bytes = std::vector<uint8_t>;
Bytes loadFile(const wchar_t *fileName);

}
}
