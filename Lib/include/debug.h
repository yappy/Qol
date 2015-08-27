#pragma once

#define STR2WSTR0(s) L ## s
#define STR2WSTR(s) STR2WSTR0(s)

#define ASSERT(x) ASSERT0(x,								\
	L"Assertion failed: " #x,								\
	STR2WSTR(__FUNCSIG__), __FILEW__,  __LINE__)

#define ASSERT0(x, msg, sig, file, line) do {				\
	if (!(x)) {												\
		test::debug::writeLine(msg);						\
		test::debug::writef(L"%s (%s: %d)", sig, file, line);	\
		::DebugBreak();										\
		throw std::logic_error("Assertion failed.");		\
	}														\
} while (0)

namespace test {
namespace debug {

bool enableDebugOutput() noexcept;
bool enableConsoleOutput() noexcept;
bool enableFileOutput(const wchar_t *fileName) noexcept;
void shutdownDebugOutput() noexcept;

void write(const wchar_t *str, bool newline = false) noexcept;
inline void writeLine(const wchar_t *str) noexcept { write(str, true); }
void writef(const wchar_t *fmt, ...) noexcept;

}
}
