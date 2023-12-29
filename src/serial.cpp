#include "serial.h"

/* static */ const std::map<std::string, Serial::Parity> Serial::PARITY_MAP = {
    {"N", Serial::Parity::NONE},
    {"O", Serial::Parity::ODD},
    {"E", Serial::Parity::EVEN}};
/* static */ const std::map<std::string, Serial::StopBits> Serial::STOPBIT_MAP = {
    {"1", Serial::StopBits::ONE},
    {"H", Serial::StopBits::ONE_AND_A_HALF},
    {"2", Serial::StopBits::TWO}};

std::ostream &operator<<(std::ostream &os, const Serial::Parity &par)
{
	for (const auto &entry : Serial::PARITY_MAP) {
		if (entry.second == par) {
			os << entry.first;
			break;
		}
	}

	return os;
}

std::ostream &operator<<(std::ostream &os, const Serial::StopBits &sb)
{
	for (const auto &entry : Serial::STOPBIT_MAP) {
		if (entry.second == sb) {
			os << entry.first;
			break;
		}
	}

	return os;
}
