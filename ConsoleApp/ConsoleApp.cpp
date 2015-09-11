// ConsoleApp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <debug.h>
#include <file.h>
#include <input.h>
#include <sound.h>
#include <exceptions.h>
#include <network.h>
#include <iostream>

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	test::debug::enableDebugOutput();
	test::debug::enableFileOutput(L"log" ".txt");

	try {
		test::file::initWithFileSystem(L".");
		//ASSERT(0);

		test::network::initialize();
		test::network::finalize();

		test::input::DInput di(nullptr);

		::LoadLibrary(L"notfound.dll");
		DWORD code = ::GetLastError();
		throw test::error::DXError("DXError test", D3D11_ERROR_FILE_NOT_FOUND);
		throw test::error::Win32Error("err test", code);
	}
	catch (std::exception &e) {
		test::debug::writeLine(e.what());
		puts(e.what());
	}

	{
		test::sound::XAudio2 sound;
		sound.loadSoundEffect("testwav", L"/C:/Windows/Media/chimes.wav");
		sound.playSoundEffect("testwav");
	}

	test::debug::shutdownDebugOutput();

    return 0;
}
