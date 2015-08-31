#include "stdafx.h"
#include "debug.hpp"
#include <cstdarg>
#include <cstdio>

namespace dx9lib {

	Debug &debug = Debug::getInstance();

	Debug & Debug::getInstance()
	{
		static Debug instance;
		return instance;
	}

	void Debug::setFile(const TCHAR *filename)
	{
		m_fout.imbue(std::locale(""));
		m_fout.open(filename);
		if(!m_fout) DebugBreak();
	}

	void Debug::print(const TCHAR *str)
	{
		::OutputDebugString(str);
		m_fout << str << std::flush;
	}

	void Debug::print(const tstring &str)
	{
		print(str.c_str());
	}

	void Debug::println(const TCHAR *str)
	{
		print(str);
		print(_T("\n"));
	}

	void Debug::println(const tstring &str)
	{
		println(str.c_str());
	}

	void Debug::printf(const TCHAR *format, ...)
	{
		std::va_list args;
		va_start(args, format);
		TCHAR buf[1024];
		::_vsntprintf_s(buf, sizeof(buf), _TRUNCATE, format, args);
		va_end(args);

		print(buf);
	}

}
