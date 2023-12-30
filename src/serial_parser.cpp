#include "serial_parser.h"

#include "serial_notes.h"

#include <functional>
#include <numeric>
#include <string>

#include <iostream>

namespace {
void trimString(std::string &str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
		          return !std::isspace(ch);
	          }));

	str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
		          return !std::isspace(ch);
	          }).base(),
	          str.end());
}
} // namespace

void SerialParser::processLine()
{
	const bool valid =
	    (m_line.length() == 9 &&
	     m_line[0] < '6' && m_line[0] >= '0') &&
	    std::transform_reduce(
	        m_line.begin() + 1, m_line.end(),
	        true,
	        std::logical_and{},
	        [](const char &c) { return c == '0' || c == '1'; });

	if (valid) {
		const std::uint16_t octaveNumber = (m_line[0] - '0') << 8;

		const std::uint16_t keyNumber = std::stoi(m_line.substr(1), nullptr, 0b10);

		for (std::size_t i = 0; i < 8; i++) {

			const std::uint16_t mask = 1u << i;
			const std::uint16_t keyCode = octaveNumber | mask;

			if (KEY_NOTE_PAIRS.count(keyCode) != 0) {
				const bool isDown = (keyNumber & mask) != 0;
				const auto keyLookup = KEY_NOTE_PAIRS.at(keyCode);

				m_sounds->safeToggleNote(keyLookup, isDown);
			}
		}
	}
}

void SerialParser::update()
{
	std::uint8_t byte = 0x00;
	if (m_serial->read(&byte, sizeof(byte)) == sizeof(byte)) {
		if (byte == '\n') {
			trimString(m_line);
			processLine();
			m_line = "";
		}
		else {
			m_line += (char)byte;
		}
	}
}