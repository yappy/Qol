#include "stdafx.h"
#include <windows.h>
#include "include/debug.h"
#include "include/util.h"

namespace yappy {
namespace debug {

namespace {

bool s_debugOut = false;
bool s_consoleOut = false;
bool s_fileOut = false;

HANDLE s_hFile = INVALID_HANDLE_VALUE;

}

bool enableDebugOutput() noexcept
{
	s_debugOut = true;
	return s_debugOut;
}

bool enableConsoleOutput() noexcept 
{
	if (s_consoleOut) {
		return s_consoleOut;
	}
	BOOL ret = ::AllocConsole();
	if (ret) {
		HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
		COORD maxSize = GetLargestConsoleWindowSize(hOut);
		COORD bufSize = { 80, maxSize.Y * 10 };
		SMALL_RECT rect = { 0, 0, bufSize.X - 1, maxSize.Y * 3 / 4 };
		SetConsoleScreenBufferSize(hOut, bufSize);
		SetConsoleWindowInfo(hOut, TRUE, &rect);
		s_consoleOut = true;
	}
	return s_consoleOut;
}

bool enableFileOutput(const wchar_t *fileName) noexcept
{
	if (s_fileOut) {
		return s_fileOut;
	}
	HANDLE h = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h != INVALID_HANDLE_VALUE) {
		s_fileOut = true;
		s_hFile = h;
	}
	return s_fileOut;
}

void shutdownDebugOutput() noexcept
{
	s_debugOut = false;

	if (s_consoleOut) {
		::FreeConsole();
	}
	s_consoleOut = false;

	if (s_hFile != INVALID_HANDLE_VALUE) {
		::CloseHandle(s_hFile);
	}
	s_fileOut = false;
}


void write(const wchar_t *str, bool newline) noexcept
{
	// Debug Out
	if (s_debugOut) {
		::OutputDebugString(str);
		if (newline) {
			::OutputDebugString(L"\n");
		}
	}
	// Console Out
	if (s_consoleOut) {
		HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD len = static_cast<DWORD>(wcslen(str));
		DWORD written = 0;
		::WriteConsole(hOut, str, len, &written, NULL);
		if (newline) {
			::WriteConsole(hOut, L"\n", 1, &written, NULL);
		}
	}
	// File Out
	if (s_fileOut) {
		auto mbstr = util::wc2utf8(str);
		DWORD written = 0;
		::WriteFile(s_hFile, mbstr.get(), static_cast<DWORD>(strlen(mbstr.get())), &written, nullptr);
		if (newline) {
			::WriteFile(s_hFile, "\n", 1, &written, nullptr);
		}
	}
}

void writef(const wchar_t *fmt, ...) noexcept
{
	va_list args;
	va_start(args, fmt);
	wchar_t buf[1024];
	_vsnwprintf_s(buf, _TRUNCATE, fmt, args);
	va_end(args);

	write(buf, true);
}

void writef(const char *fmt, ...) noexcept
{
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsnprintf_s(buf, _TRUNCATE, fmt, args);
	va_end(args);

	write(buf, true);
}

void writef_nonl(const wchar_t *fmt, ...) noexcept
{
	va_list args;
	va_start(args, fmt);
	wchar_t buf[1024];
	_vsnwprintf_s(buf, _TRUNCATE, fmt, args);
	va_end(args);

	write(buf, false);
}

void writef_nonl(const char *fmt, ...) noexcept
{
	va_list args;
	va_start(args, fmt);
	char buf[1024];
	vsnprintf_s(buf, _TRUNCATE, fmt, args);
	va_end(args);

	write(buf, false);
}

}
}
