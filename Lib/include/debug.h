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

void dwrite(const wchar_t *str);
void dwriteLine(const wchar_t *str);
void dprintf(const wchar_t *fmt, ...);

}
}
