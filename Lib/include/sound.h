#pragma once

#include "util.h"
#include <xaudio2.h>

namespace test {
namespace sound {

class DSound : private util::noncopyable {
public:
	DSound();
	~DSound();

private:
	std::unique_ptr<IXAudio2, util::IUnknownDeleterType> m_pIXAudio(nullptr_t);
};

}
}
