#pragma once

#include "exceptions.hpp"
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>

namespace dx9lib {

	// Copied from Effective C++
	class Uncopyable {
	protected:
		Uncopyable(){}
		~Uncopyable(){}
	private:
		Uncopyable(const Uncopyable &);
		Uncopyable & operator=(const Uncopyable &);
	};

	// T -> tstring
	template <typename T>
	inline tstring to_s(T val)
	{
		tstringstream ss;
		ss << val;
		return ss.str();
	}

	// tstring -> int
	inline int to_i(const tstring str)
	{
		TCHAR *endp;
		long n = ::_tcstol(str.c_str(), &endp, 10);
		if(str.size() == 0 || *endp != _T('\0'))
			throw FormatError(_T("Number format error"));
		if(errno == ERANGE)
			throw FormatError(_T("Number out of range"));
		return n;
	}

	inline void readCSVLine(std::vector<tstring> &out, tistream &in)
	{
		out.clear();
		array<TCHAR, 1024> str;
		in.getline(&str[0], str.size());
		tstring line(&str[0]);
		unsigned int start = 0;
		unsigned int end;
		do {
			end = line.find(_T(","), start);
			if (end != tstring::npos) {
				out.push_back(line.substr(start, end - start));
			} else{
				out.push_back(line.substr(start));
			}
			start = end + 1;
		} while (end != tstring::npos);
		in.peek();
	}

	struct handle_deleter {
		void operator()(HANDLE h)
		{
			CloseHandle(h);
		}
	};

	struct mapview_deleter {
		void operator()(void *p)
		{
			UnmapViewOfFile(p);
		}
	};

	struct iunknown_deleter {
		void operator()(IUnknown *p)
		{
			p->Release();
		}
	};

}
