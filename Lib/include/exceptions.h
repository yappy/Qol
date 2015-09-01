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

class WinSockError : public Win32Error {
public:
	explicit WinSockError(const std::string &msg, int code) noexcept
		: Win32Error(msg, static_cast<DWORD>(code)) {}
};


class DXError : public std::runtime_error {
public:
	explicit DXError(const std::string &msg, HRESULT hr) noexcept;
	const char *what() const override;
private:
	std::string m_what;
};

class D3DError : public DXError {
public:
	explicit D3DError(const std::string &msg, HRESULT hr) noexcept
		: DXError(msg, hr) {}
};

class DIError : public DXError {
public:
	explicit DIError(const std::string &msg, HRESULT hr) noexcept
		: DXError(msg, hr) {}
};

class DSError : public DXError {
public:
	explicit DSError(const std::string &msg, HRESULT hr) noexcept
		: DXError(msg, hr) {}
};

}
