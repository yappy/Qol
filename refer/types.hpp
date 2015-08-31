#pragma once

#include <array>
#include <memory>
#include <tchar.h>
#include <string>
#include <fstream>

namespace dx9lib {

	using std::array;
	using std::shared_ptr;
	using std::make_shared;

	typedef std::basic_string<TCHAR> tstring;
	typedef std::basic_istream<TCHAR> tistream;
	typedef std::basic_ifstream<TCHAR> tifstream;
	typedef std::basic_ofstream<TCHAR> tofstream;
	typedef std::basic_stringstream<TCHAR> tstringstream;

}
