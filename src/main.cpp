#include <CLI/CLI.hpp>
#include <iostream>

#include <cstdint>

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

#include <functional>

#include <unordered_set>

#include "serial_notes.h"
#include "windows_serial.h"

#include "audio.h"

constexpr static const float MAX_NOTE_DURATION = 1.0f * 60.0f * 60.0f;
constexpr static const std::size_t READ_CHUNK_SIZE = 32;

std::unordered_set<Note> sounds;
std::atomic_bool finished(false), running(false);

std::mutex soundMutex;

void serial_parse_line(const std::string &line)
{
	const bool valid =
	    (line.length() == 9 &&
	     line[0] < '6' && line[0] >= '0') &&
	    std::transform_reduce(
	        line.begin() + 1, line.end(),
	        true,
	        std::logical_and{},
	        [](const char &c) { return c == '0' || c == '1'; });

	if (valid) {
		const std::uint16_t octaveNumber = (line[0] - '0') << 8;
		if (octaveNumber == (5 << 8)) {
			std::cerr << line << std::endl;
		}

		const std::uint16_t keyNumber = std::stoi(line.substr(1), nullptr, 0b10);

		decltype(sounds) localSounds, originalSounds;
		soundMutex.lock();
		originalSounds = sounds;
		soundMutex.unlock();

		localSounds = originalSounds;

		for (std::size_t i = 0; i < 8; i++) {
			const std::uint16_t mask = 1u << i;
			const std::uint16_t keyCode = octaveNumber | mask;

			if (KEY_NOTE_PAIRS.count(keyCode) != 0) {
				const bool isDown = (keyNumber & mask) != 0;
				const auto keyLookup = KEY_NOTE_PAIRS.at(keyCode);

				if (isDown) {
					localSounds.insert(keyLookup);
				}
				else { // 101|00101000
					localSounds.erase(keyLookup);
				}
			}
		}

		soundMutex.lock();
		sounds = localSounds;
		soundMutex.unlock();
	}
}

void serial_thread(const std::string &port, unsigned int baud, const Serial::Settings &settings)
{
	std::cout << "Trying to initialize serial on " << port << " at " << baud << "bps with the settings:\n"
	          << "\tbyte size: " << settings.byte_size << "\n"
	          << "\tparity:    " << settings.parity << "\n"
	          << "\tstop bits: " << settings.stop_bits << std::endl;

	WindowsSerial serial;
	if (!serial.begin(port, baud, settings)) {
		std::cerr << "Failed initializing serial port." << std::endl;
		finished = true;
		return;
	}

	running = true;
	std::string line = "";

	while (!finished) {
		std::uint8_t byte = 0x00;
		if (serial.read(&byte, sizeof(byte)) == sizeof(byte)) {
			if (byte == '\n') {
				line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
					           return !std::isspace(ch);
				           }));

				line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
					           return !std::isspace(ch);
				           }).base(),
				           line.end());

				serial_parse_line(line);
				line = "";
			}
			else {
				line += (char)byte;
			}
		}
	}

	serial.end();
}

void stdin_thread()
{
	// std::this_thread::sleep_for(std::chrono::seconds(10));
	// finished = true;
	/* if (!finished) {
	    std::cout << "Press Q to quit." << std::endl;
	}

	while (!finished) {
	    char buf;
	    if (std::cin.readsome(&buf, sizeof(buf)) == 1) {
	        if (buf == 'q' || buf == 'Q') {
	            finished = true;
	        }
	    }
	    else if (std::cin.eof()) {
	        finished = true;
	    }
	    std::this_thread::sleep_for(std::chrono::milliseconds(100));
	} */
}

std::unordered_set<Note> setDifference(const std::unordered_set<Note> &base, const std::unordered_set<Note> &other)
{
	std::unordered_set<Note> result;
	for (const auto &item : base) {
		if (other.count(item) == 0)
			result.insert(item);
	}

	return result;
}

int main(int argc, char *argv[])
{
	CLI::App app{"App description"};

	std::string port = "COM8";
	unsigned int baud = 9600;
	Serial::Settings settings = Serial::ARDUINO_SETTINGS;

	float volume = 0.3f;

	bool test = false;

	app.add_option("-p,--port", port, "The serial port to connect to");
	app.add_option("-b,--baud", baud, "The baud rate of the serial connection");
	app.add_option("-s,--stop_bits,--stop", settings.stop_bits, "The number of stop bits")
	    ->transform(CLI::CheckedTransformer(Serial::STOPBIT_MAP, CLI::ignore_case));
	app.add_option("--par,--parity", settings.parity, "The parity bit. N for none, E for even, O for odd")
	    ->transform(CLI::CheckedTransformer(Serial::PARITY_MAP, CLI::ignore_case));
	app.add_option("--bs,--byte_size", settings.byte_size, "The number of bits");
	app.add_option("--volume,-v", volume, "The volume in the range [0-1]");

	CLI11_PARSE(app, argc, argv);

	Audio audio;
	if (!audio.begin()) {
		std::cerr << "Failed initializing OpenAL." << std::endl;
		return 1;
	}

	audio.setVolume(volume);

	std::thread serial_thread_handle(serial_thread, port, baud, settings);

	while (!running && !finished) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if (running) {
		std::thread stdin_thread_handle(stdin_thread);

		while (!finished) {
			decltype(sounds) localSounds;

			soundMutex.lock();
			localSounds = sounds;
			soundMutex.unlock();

			const auto playedNotes = audio.getActiveNotes();
			std::unordered_set<Note> locallyAvailableButNotPlayed = setDifference(localSounds, playedNotes),
			                         playedButNotLocallyAvailable = setDifference(playedNotes, localSounds);

			for (const auto &noteToStop : playedButNotLocallyAvailable)
				audio.stopNote(noteToStop);

			for (const auto &noteToPlay : locallyAvailableButNotPlayed)
				audio.playNote(noteToPlay);

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		stdin_thread_handle.join();
	}

	serial_thread_handle.join();

	audio.end();

	return 0;
}