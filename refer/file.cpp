#include "stdafx.h"
#include "file.hpp"
#include "exceptions.hpp"

namespace dx9lib {

	/*
	MemoryMappedFile::MemoryMappedFile(const WCHAR *fileName, unsigned offset, unsigned len) : 
	m_hFile(), m_hMap(), m_pData(), m_size(0), m_pos(0)
	{
		HANDLE hFile = ::CreateFile(
			fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
		);
		if(hFile == INVALID_HANDLE_VALUE)
			Win32Error::throwError("File open failed");
		m_hFile.reset(hFile, handle_deleter());
		m_size = ::GetFileSize(m_hFile.get(), NULL);

		HANDLE hMap = ::CreateFileMapping(m_hFile.get(), NULL, PAGE_READONLY, 0, 0, NULL);
		if(hMap == NULL)
			Win32Error::throwError("File open failed");
		m_hMap.reset(hMap, handle_deleter());

		void *pData = ::MapViewOfFile(m_hMap.get(), FILE_MAP_READ, 0, 0, 0);
		if(pData == NULL)
			Win32Error::throwError("File mapping failed");
		m_pData.reset(pData, mapview_deleter());
	}

	int MemoryMappedFile::read()
	{
		if(m_pos >= m_size){
			return -1;
		}
		return (reinterpret_cast<BYTE *>(m_pData.get()))[m_pos++];
	}

	void MemoryMappedFile::read(void *pOut, unsigned int len)
	{
		for(unsigned i=0; i<len; i++){
			int t = read();
			if(t == -1)
				throw IOError("EOF detected");
			(reinterpret_cast<BYTE *>(pOut))[i] = t;
		}
	}
	*/


	void FileResourceLoader::readFile(std::vector<BYTE> &out, const tstring &fileName)
	{
		HANDLE hTmpFile = ::CreateFile(
			fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if(hTmpFile == INVALID_HANDLE_VALUE)
			Win32Error::throwError(_T("CreateFile failed."));
		shared_ptr<void> hFile(hTmpFile, handle_deleter());

		LARGE_INTEGER size;
		if(!::GetFileSizeEx(hFile.get(), &size))
			Win32Error::throwError(_T("GetFileSizeEx failed."));
		if(size.HighPart != 0)
			Win32Error::throwError(_T("File is too large."));
		out.resize(size.LowPart);
		if(size.LowPart != 0){
			DWORD read;
			if(!::ReadFile(hFile.get(), &out.at(0), out.size(), &read, NULL))
				Win32Error::throwError(_T("ReadFile failed."));
		}
	}

}
