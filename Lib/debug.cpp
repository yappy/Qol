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

}	// debug

namespace trace {

namespace {

using error::checkWin32Result;

const size_t LINE_DATA_SIZE = 128;
struct LineBuffer {
	LARGE_INTEGER counter;
	uint32_t number;
	char msg[LINE_DATA_SIZE - sizeof(LARGE_INTEGER) - sizeof(uint32_t)];
};
static_assert(sizeof(LineBuffer) == LINE_DATA_SIZE, "invalid struct def");

struct VirtualDeleter {
	void operator()(void *p)
	{
		BOOL ret = ::VirtualFree(p, 0, MEM_RELEASE);
		if (!ret) {
			debug::writeLine(L"VirtualFree() failed");
		}
	}
};

std::unique_ptr<LineBuffer[], VirtualDeleter> s_buf;
size_t s_size;
uint32_t s_number;

}	// namespace

void initialize(size_t bufsize)
{
	void *p = ::VirtualAlloc(nullptr, bufsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	checkWin32Result(p != nullptr, "VirtualAlloc() failed");
	s_buf.reset(static_cast<LineBuffer *>(p));
	s_size = bufsize / sizeof(LineBuffer);
	s_number = 0;

	std::memset(s_buf.get(), 0, bufsize);
}

void output()
{
	wchar_t tempPath[MAX_PATH];
	DWORD dret = GetTempPath(MAX_PATH, tempPath);
	checkWin32Result(dret != 0, "GetTempPath() failed");
	wchar_t filePath[MAX_PATH];
	UINT uret = GetTempFileName(tempPath, L"trc", 0, filePath);
	checkWin32Result(uret != 0, "GetTempFileName() failed");
	HANDLE tmpFile = CreateFile(filePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	checkWin32Result(tmpFile != INVALID_HANDLE_VALUE, "GetTempFileName() failed");
	util::HandlePtr hFile(tmpFile);

	LARGE_INTEGER freq = { 0 };
	::QueryPerformanceFrequency(&freq);

	LARGE_INTEGER prevCount = { 0 };
	for (size_t i = 0; i < s_size; i++) {
		size_t ind = (s_number + i) % s_size;
		const LineBuffer &line = s_buf[ind];
		if (line.counter.QuadPart == 0) {
			continue;
		}
		auto writeFunc = [&hFile](const char *str) {
			DWORD len = static_cast<DWORD>(std::strlen(str));
			DWORD writeSize = 0;
			BOOL bret = WriteFile(hFile.get(), str, len, &writeSize, nullptr);
			checkWin32Result(bret != 0, "WriteFile() failed");
		};
		{
			double sec = 0.0;
			if (prevCount.QuadPart != 0) {
				sec = static_cast<double>(line.counter.QuadPart - prevCount.QuadPart) / freq.QuadPart;
			}
			prevCount = s_buf[ind].counter;
			char buf[64];
			::sprintf_s(buf, "[%08x +%07.4fms] ", s_buf[ind].number, sec * 1e3);
			writeFunc(buf);
		}
		writeFunc(line.msg);
		writeFunc("\r\n");
	}

	// close file
	hFile.reset();
	// open with notepad
	HINSTANCE hret = ::ShellExecute(
		nullptr, nullptr, L"notepad", filePath, nullptr, SW_NORMAL);
	checkWin32Result(reinterpret_cast<ULONG_PTR>(hret) > 32, "ShellExecute() failed");
}

void write(const char *str)
{
	LineBuffer &line = s_buf[s_number % s_size];
	::QueryPerformanceCounter(&line.counter);
	line.number = s_number;

	::strncpy_s(line.msg, str, sizeof(line.msg) - 1);
	s_number++;
}

}	// trace
}
