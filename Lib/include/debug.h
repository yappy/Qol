#pragma once

#include "util.h"
#include "exceptions.h"

#define STR2WSTR0(s) L ## s
#define STR2WSTR(s) STR2WSTR0(s)

#define ASSERT(x) ASSERT0(x,								\
	L"Assertion failed: " #x,								\
	STR2WSTR(__FUNCSIG__), __FILEW__,  __LINE__)

#define ASSERT0(x, msg, sig, file, line) do {				\
	if (!(x)) {												\
		yappy::debug::writeLine(msg);						\
		yappy::debug::writef(L"%s (%s: %d)", sig, file, line);	\
		::DebugBreak();										\
	}														\
} while (0)

namespace yappy {
namespace debug {

bool enableDebugOutput() noexcept;
bool enableConsoleOutput() noexcept;
bool enableFileOutput(const wchar_t *fileName) noexcept;
void shutdownDebugOutput() noexcept;

void write(const wchar_t *str, bool newline = false) noexcept;
inline void writeLine(const wchar_t *str) noexcept { write(str, true); }
inline void writeLine(const char *str) noexcept { write(util::utf82wc(str).c_str(), true); }
void writef(const wchar_t *fmt, ...) noexcept;


class StopWatch : private util::noncopyable {
public:
	explicit StopWatch(const wchar_t *msg) : m_msg(msg)
	{
		BOOL b = ::QueryPerformanceCounter(&m_begin);
		error::checkWin32Result(b != 0, "QueryPerformanceCounter() failed");
	}
	~StopWatch()
	{
		BOOL b;
		LARGE_INTEGER end, freq;
		b = ::QueryPerformanceCounter(&end);
		error::checkWin32Result(b != 0, "QueryPerformanceCounter() failed");
		b = ::QueryPerformanceFrequency(&freq);
		error::checkWin32Result(b != 0, "QueryPerformanceFrequency() failed");
		double sec = static_cast<double>(end.QuadPart - m_begin.QuadPart) / freq.QuadPart;
		writef(L"%s: %.3f us", m_msg, sec * 1e6);
	}
private:
	const wchar_t *m_msg;
	LARGE_INTEGER m_begin;
};

}
}
