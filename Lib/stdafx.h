// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define NOMINMAX
#include <WinSock2.h>
//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <mmsystem.h>
#include <xaudio2.h>

#include <stdexcept>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
