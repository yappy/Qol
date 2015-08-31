#pragma once

#include "types.hpp"
#include <stdexcept>
#include <sstream>
#include <windows.h>
#include <dxerr.h>
#pragma comment(lib, "dxerr.lib")

namespace dx9lib {

	class texception {
	private:
		dx9lib::tstring m_what;
	public:
		texception() {}
		explicit texception(const tstring &msg) : m_what(msg) {}
		virtual ~texception() {}
		virtual const TCHAR *what() const { return m_what.c_str(); }
	};

	class Dx9libError : public texception {
	public:
		explicit Dx9libError(const tstring &msg) : texception(msg) {}
	};

	class Win32Error : public Dx9libError {
	public:
		explicit Win32Error(const tstring &msg) : Dx9libError(msg) {}
		static void throwError(const tstring &msg)
		{
			TCHAR *buf;
			DWORD code = ::GetLastError();
			::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr, ::GetLastError(), LANG_SYSTEM_DEFAULT,
				reinterpret_cast<TCHAR *>(&buf), 0, nullptr);
			Win32Error ex(buf);
			::LocalFree(buf);
			throw ex;
		}
	};

	template <class T>
	class DXError : public Dx9libError {
	public:
		explicit DXError(const tstring &msg) : Dx9libError(msg) {}
		static void check(HRESULT hr)
		{
			if(FAILED(hr)){
				array<TCHAR, 1024> buf;
				const TCHAR *str = ::DXGetErrorString(hr);
				const TCHAR *desc = ::DXGetErrorDescription(hr);
				::wsprintf(buf.data(), "%s: %s", str, desc);
				throw T(buf.data());
			}
		}
	};

	class DGraphicsError : public DXError<DGraphicsError> {
	public:
		explicit DGraphicsError(const tstring &msg) : DXError(msg) {}
	};

	class DSoundError : public DXError<DSoundError> {
	public:
		explicit DSoundError(const tstring &msg) : DXError(msg) {}
	};

	class DInputError : public DXError<DInputError> {
	public:
		explicit DInputError(const tstring &msg) : DXError(msg) {}
	};

	class FormatError : public Dx9libError {
	public:
		explicit FormatError(const tstring &msg) : Dx9libError(msg) {}
	};

	class IOError : public Dx9libError {
	public:
		explicit IOError(const tstring &msg) : Dx9libError(msg) {}
	};

	class MathError : public Dx9libError {
	public:
		explicit MathError(const tstring &msg) : Dx9libError(msg) {}
	};

}
