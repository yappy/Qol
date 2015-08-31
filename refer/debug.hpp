#pragma once

#include "util.hpp"

namespace dx9lib {

	class Debug {
	private:
		Debug() {}
		Debug(const Debug &);
		~Debug() {}
		Debug & operator =(const Debug &);

		tofstream m_fout;

	public:
		static Debug & getInstance();

		void setOutputDebugString(bool enabled);
		void setFile(const TCHAR *filename);

		void print(const TCHAR *str);
		void print(const tstring &str);
		void println(const TCHAR *str);
		void println(const tstring &str);
		void printf(const TCHAR *format, ...);
	};

	extern Debug &debug;

}
