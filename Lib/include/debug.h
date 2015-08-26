#pragma once

#define ASSERT(x) //TODO

#define ASSERT0(x, file, line) do {	\
	if (!(x)) {						\
		::DebugBreak();				\
		::ExitProcess(1);			\
	}								\
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
