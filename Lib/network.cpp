#include "stdafx.h"
#include "include/network.h"
#include "include/exceptions.h"
#include "include/debug.h"
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

namespace yappy {
namespace network {

using error::WinSockError;

void initialize()
{
	int ret;
	WSADATA wsaData;

	ret = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0) {
		throw WinSockError("WSAStartup() failed", ret);
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		finalize();
		throw WinSockError("WSAStartup() failed", ret);
	}

	debug::writef(L"WinSock2 initialized");
	debug::writef(L"wVersion = %d.%d", LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
	debug::writef(L"wHighVersion = %d.%d", LOBYTE(wsaData.wHighVersion), HIBYTE(wsaData.wHighVersion));
	debug::writef(L"szDescription = %s", util::utf82wc(wsaData.szDescription).c_str());
	debug::writef(L"szSystemStatus = %s", util::utf82wc(wsaData.szSystemStatus).c_str());
}

void finalize() noexcept
{
	int ret = ::WSACleanup();
	if (ret != 0) {
		int err = ::WSAGetLastError();
		debug::writeLine(WinSockError("WSACleanup() failed", err).what());
	}
}

}
}
