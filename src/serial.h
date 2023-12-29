#ifndef PIANO_SERIAL_H
#define PIANO_SERIAL_H

#include <istream>
#include <map>
#include <ostream>
#include <string>

#include <cstdint>

class Serial {
public:
	enum Parity {
		NONE = 0,
		ODD = 1,
		EVEN = 2
	};

	enum StopBits {
		ONE = 0,
		ONE_AND_A_HALF = 1,
		TWO = 2
	};

	struct Settings {
		Parity parity;
		StopBits stop_bits;
		unsigned int byte_size;
	};

	constexpr static const Settings ARDUINO_SETTINGS = {
	    Parity::NONE,
	    StopBits::ONE,
	    8};

	static const std::map<std::string, Parity> PARITY_MAP;
	static const std::map<std::string, StopBits> STOPBIT_MAP;

public:
	virtual bool begin(const std::string &port, unsigned int baud, Settings settings) = 0;
	virtual void end() = 0;

	virtual std::size_t read(std::uint8_t *output, std::size_t count) = 0;
};

std::ostream &operator<<(std::ostream &os, const Serial::Parity &par);
std::ostream &operator<<(std::ostream &os, const Serial::StopBits &sb);

#endif // !defined(PIANO_SERIAL_H)