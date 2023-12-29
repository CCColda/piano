#ifndef PIANO_WINDOWS_SERIAL_H
#define PIANO_WINDOWS_SERIAL_H

#include "serial.h"
#include <windows.h>

class WindowsSerial : public Serial {
protected:
	HANDLE serialHandle;

public:
	virtual bool begin(const std::string &port, unsigned int baud, Serial::Settings settings) override;
	virtual void end() override;

	virtual std::size_t read(std::uint8_t *out, size_t size) override;
};

#endif // !defined(PIANO_WINDOWS_SERIAL_H)