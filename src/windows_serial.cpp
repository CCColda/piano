#include "windows_serial.h"

/* virtual */ bool WindowsSerial::begin(const std::string &port, unsigned int baud, Serial::Settings settings) /* override */
{
	//! @see https://stackoverflow.com/a/15795522

	const auto port_path = std::string("\\\\.\\") + port;

	serialHandle = CreateFileA(port_path.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (serialHandle == INVALID_HANDLE_VALUE)
		return false;

	// Do some basic settings
	DCB serialParams = {0};
	serialParams.DCBlength = sizeof(serialParams);

	if (!GetCommState(serialHandle, &serialParams)) {
		CloseHandle(serialHandle);
		return false;
	}

	serialParams.BaudRate = baud;
	serialParams.ByteSize = settings.byte_size;
	serialParams.StopBits = settings.stop_bits;
	serialParams.Parity = settings.parity;

	if (!SetCommState(serialHandle, &serialParams)) {
		CloseHandle(serialHandle);
		return false;
	}

	// Set timeouts
	COMMTIMEOUTS timeout = {0};
	GetCommTimeouts(serialHandle, &timeout);

	timeout.ReadIntervalTimeout = 0;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(serialHandle, &timeout)) {
		CloseHandle(serialHandle);
		return false;
	}

	return true;
}

/* virtual */ void WindowsSerial::end() /* override */
{
	if (serialHandle != INVALID_HANDLE_VALUE)
		CloseHandle(serialHandle);
}

/* virtual */ std::size_t WindowsSerial::read(std::uint8_t *out, size_t size) /* override */
{
	DWORD bytes_read = 0;
	ReadFile(serialHandle, out, static_cast<DWORD>(size), &bytes_read, NULL);

	return static_cast<std::size_t>(bytes_read);
}
