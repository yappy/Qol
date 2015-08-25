#pragma once

#include <stdexcept>
#include <memory>
#include <sstream>
#include <iomanip>
#include <windows.h>

namespace test {

class Win32Error : public std::runtime_error {
public:
	explicit Win32Error(const std::string &msg, DWORD code) noexcept;
	const char *what() const override;
private:
	std::string m_what;
};

class DXError : public std::runtime_error {
public:
	explicit DXError(const std::string &msg, HRESULT hr) noexcept;
	const char *what() const override;
private:
	std::string m_what;
};

}
