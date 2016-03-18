#pragma once

#include "util.h"
#include <vector>
#include <limits>

namespace yappy {
/// File abstract layer.
namespace file {

/// 0x7fffffff = 2GiB
const uint32_t FileSizeMax = std::numeric_limits<int32_t>::max();

/** @brief Uses real file system.
 * @param[in] rootDir	Root directory path.
 */
void initWithFileSystem(const wchar_t *rootDir);
/** @brief Uses archive file.
 * @param[in] archiveFile	Archive file path.
 * @warning Not implemented...
 */
void initWithArchiveFile(const wchar_t *archiveFile);

/// File byte sequence. Vector of uint8_t.
using Bytes = std::vector<uint8_t>;

/** @brief Load file from abstract file system.
 * @details Library uses this function.
 * initXXX() function must be called at first.
 * @param[in] fileName	File name.
 */
Bytes loadFile(const wchar_t *fileName);

}
}
