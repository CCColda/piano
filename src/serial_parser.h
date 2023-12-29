#ifndef PIANO_SERIAL_PARSER_H
#define PIANO_SERIAL_PARSER_H

#include "sounds.h"

#include "serial.h"

class SerialParser {
private:
	Serial *m_serial;
	Sounds *m_sounds;

	std::string m_line;

private:
	void processLine();

public:
	inline SerialParser(Serial *serial, Sounds *sounds)
	    : m_serial(serial), m_sounds(sounds), m_line() {}

	~SerialParser() = default;

	void update();
};

#endif // !defined(PIANO_SERIAL_PARSER_H)