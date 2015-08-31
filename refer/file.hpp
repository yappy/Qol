#pragma once

#include "util.hpp"
#include <string>
#include <vector>

namespace dx9lib {

	/*
	class MemoryMappedFile : private Uncopyable {
	private:
		shared_ptr<void> m_hFile;
		shared_ptr<void> m_hMap;
		shared_ptr<void> m_pData;
		unsigned m_size;
		unsigned m_pos;
	public:
		MemoryMappedFile(const WCHAR *fileName, unsigned offset = 0, unsigned len = 0);
		int read();
		void read(void *pOut, unsigned len);
	};
	*/

	class ResourceLoader {
	public:
		virtual void readFile(std::vector<BYTE> &out, const tstring &fileName) = 0;
	};

	class FileResourceLoader : public ResourceLoader {
	public:
		void readFile(std::vector<BYTE> &out, const tstring &fileName);
	};

	// TODO implimentation
	class ArchiveResourceLoader : public ResourceLoader {
	public:
		void readFile(std::vector<BYTE> &out, const tstring &fileName);
	};

}
