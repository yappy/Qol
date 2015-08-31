// ConsoleApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <d3d11.h>
#include <debug.h>
#include <file.h>
#include <exceptions.h>
#include <network.h>
#include <iostream>

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	test::debug::enableDebugOutput();
	test::debug::enableFileOutput(L"log" ".txt");

	try {
		//ASSERT(0);

		test::network::initialize();
		test::network::finalize();

		::LoadLibrary(L"notfound.dll");
		DWORD code = ::GetLastError();
		throw test::DXError("DXError test", D3D11_ERROR_FILE_NOT_FOUND);
		throw test::Win32Error("err test", code);
	}
	catch (std::exception &e) {
		puts(e.what());
	}

	test::debug::shutdownDebugOutput();

    return 0;
}
