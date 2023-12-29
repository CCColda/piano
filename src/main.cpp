#include <CLI/CLI.hpp>
#include <iostream>

#include <cstdint>

#include <thread>

#include "global.h"
#include "serial_parser.h"
#include "windows_serial.h"

#include "audio.h"

void serial_thread(const std::string &port, unsigned int baud, const Serial::Settings &settings, Global *global)
{
	std::cout << "Trying to initialize serial on " << port << " at " << baud << "bps with the settings:\n"
	          << "\tbyte size: " << settings.byte_size << "\n"
	          << "\tparity:    " << settings.parity << "\n"
	          << "\tstop bits: " << settings.stop_bits << std::endl;

	WindowsSerial serial;
	if (!serial.begin(port, baud, settings)) {
		std::cerr << "Failed initializing serial port." << std::endl;
		global->state = AppState::FINISHED;
		return;
	}

	global->state = AppState::RUNNING;

	SerialParser parser(&serial, &(global->sounds));

	while (global->state != AppState::FINISHED) {
		parser.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	serial.end();
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

	Global global;
	global.state = AppState::SETUP;

	Audio audio;
	if (!audio.begin()) {
		std::cerr << "Failed initializing OpenAL." << std::endl;
		return 1;
	}

	audio.setVolume(volume);

	std::thread serial_thread_handle(serial_thread, port, baud, settings, &global);

	while (global.state == SETUP) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	while (!global.state != FINISHED) {
		auto localSounds = global.sounds.getCopy();

		const auto playedNotes = audio.getActiveNotes();
		std::unordered_set<Note> locallyAvailableButNotPlayed = setDifference(localSounds, playedNotes),
		                         playedButNotLocallyAvailable = setDifference(playedNotes, localSounds);

		for (const auto &noteToStop : playedButNotLocallyAvailable)
			audio.stopNote(noteToStop);

		for (const auto &noteToPlay : locallyAvailableButNotPlayed)
			audio.playNote(noteToPlay);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	serial_thread_handle.join();

	audio.end();

	return 0;
}