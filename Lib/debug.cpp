#include "stdafx.h"
#include <windows.h>
#include "include/debug.h"
#include "include/util.h"

namespace test {
namespace debug {

namespace {

bool s_debugOut = false;
bool s_consoleOut = false;
bool s_fileOut = false;

std::unique_ptr<std::remove_pointer<HANDLE>, decltype(&test::util::handleDeleter)> a(nullptr, test::util::handleDeleter);

}

bool enableDebugOutput() noexcept {
	s_debugOut = true;
	return s_debugOut;
}

bool enableConsoleOutput() noexcept  {
	BOOL ret = ::AllocConsole();
	if (ret) {
		s_consoleOut = true;
	}
	return s_consoleOut;
}

bool enableFileOutput(const wchar_t *fileName) noexcept {
	HANDLE h = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h != INVALID_HANDLE_VALUE) {
		s_fileOut = true;
	}
	return s_fileOut;
}

void shutdownDebugOutput() noexcept {

}



}
}
