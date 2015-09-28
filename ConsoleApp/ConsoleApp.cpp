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
	yappy::debug::enableDebugOutput();
	yappy::debug::enableFileOutput(L"log" ".txt");

	try {
		yappy::file::initWithFileSystem(L".");
		//ASSERT(0);

		yappy::network::initialize();
		yappy::network::finalize();

		yappy::input::DInput di(::GetModuleHandle(nullptr), nullptr);

		::LoadLibrary(L"notfound.dll");
		DWORD code = ::GetLastError();
		throw yappy::error::DXError("DXError test", D3D11_ERROR_FILE_NOT_FOUND);
		throw yappy::error::Win32Error("err test", code);
	}
	catch (std::exception &e) {
		yappy::debug::writeLine(e.what());
		puts(e.what());
	}

	{
		yappy::sound::XAudio2 sound;

		sound.playBgm(L"../sampledata/Epoq-Lepidoptera.ogg");

		sound.loadSoundEffect("testwav", L"/C:/Windows/Media/chimes.wav");
		for (int i = 0; i < 4; i++) {
			sound.playSoundEffect("testwav");
			Sleep(300);
		}

		for (int i = 0; i < 60 * 10; i++) {
			sound.processFrame();
			Sleep(16);
		}
	}

	yappy::debug::shutdownDebugOutput();

    return 0;
}
