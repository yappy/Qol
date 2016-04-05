/** @file
* @brief Debug utilities.
*/

#pragma once

#include "util.h"
#include "exceptions.h"

#define STR2WSTR0(s) L ## s
#define STR2WSTR(s) STR2WSTR0(s)

/** @brief Assertion which uses debug framework.
 * @details If x is false, prints information(func, file, line) and
 * calls DebugBreak() API.
 * @param[in] x	A value which must be true.
 */
#define ASSERT(x) ASSERT0(x,									\
	L"Assertion failed: " #x,									\
	STR2WSTR(__FUNCSIG__), __FILEW__,  __LINE__)

#define ASSERT0(x, msg, sig, file, line) do {					\
	if (!(x)) {													\
		yappy::debug::writeLine(msg);							\
		yappy::debug::writef(L"%s (%s: %d)", sig, file, line);	\
		::DebugBreak();											\
	}															\
} while (0)

namespace yappy {
/// Debug utilities.
namespace debug {

/** @brief Enables writing to OutputDebugString().
 */
bool enableDebugOutput() noexcept;
/** @brief Shows a console window and enables writing to it.
 */
bool enableConsoleOutput() noexcept;
/** @brief Enables writing to a file.
 * @param[in] fileName	File name.
 */
bool enableFileOutput(const wchar_t *fileName) noexcept;
/** @brief Flush buffers and free resources.
 */
void shutdownDebugOutput() noexcept;

/** @brief Write debug string.
 * @param[in] str		Debug message string.
 * @param[in] newline	true if new line after str.
 */
void write(const wchar_t *str, bool newline = false) noexcept;
/** @brief Write debug string.
 * @param[in] str		Debug message string.
 * @param[in] newline	true if new line after str.
 */
inline void write(const char *str, bool newline = false) noexcept { write(util::utf82wc(str).get(), newline); }
/** @brief Write debug string and new line.
 * @param[in] str		Debug message string.
 * @param[in] newline	true if new line after str.
 */
inline void writeLine(const wchar_t *str = L"") noexcept { write(str, true); }
/** @brief Write debug string and new line.
 * @param[in] str		Debug message string.
 * @param[in] newline	true if new line after str.
 */
inline void writeLine(const char *str) noexcept { write(util::utf82wc(str).get(), true); }
/** @brief Write debug message using format string like printf.
* @param[in] fmt	Format string.
* @param[in] ...	Additional params.
*/
void writef(const wchar_t *fmt, ...) noexcept;
/** @brief Write debug message using format string like printf.
* @param[in] fmt	Format string.
* @param[in] ...	Additional params.
*/
void writef(const char *fmt, ...) noexcept;
/** @brief Write debug message using format string like printf.
* @param[in] fmt	Format string.
* @param[in] ...	Additional params.
*/
void writef_nonl(const wchar_t *fmt, ...) noexcept;
/** @brief Write debug message using format string like printf.
* @param[in] fmt	Format string.
* @param[in] ...	Additional params.
*/
void writef_nonl(const char *fmt, ...) noexcept;


/** @brief Stop watch utility for performance measurement.
 * @details Time is measured by QueryPerformanceCounter() API.
 */
class StopWatch : private util::noncopyable {
public:
	/** @brief Constructor. Starts the timer.
	 * @param[in] msg	String message for printing result.
	 */
	explicit StopWatch(const wchar_t *msg) : m_msg(msg)
	{
		BOOL b = ::QueryPerformanceCounter(&m_begin);
		error::checkWin32Result(b != 0, "QueryPerformanceCounter() failed");
	}
	/** @brief Destructor. Stop the timer.
	 * @details Result will be printed automatically.
	 */
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
