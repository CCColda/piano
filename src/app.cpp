#include "app.h"

#include <Logger.h>

#include "serial_parser.h"
#include "windows_serial.h"

#include <chrono>

#include "app_audio_thread.h"
#include "app_serial_thread.h"

namespace {
struct PortValidator : public CLI::Validator {
	PortValidator()
	{
		name_ = "PORT";
		func_ = [](const std::string &str) {
			const auto com_part = str.substr(0, 3);
			const auto number_part = str.substr(3);

			const auto number_part_valid = std::transform_reduce(
			    number_part.begin(), number_part.end(),
			    true, std::logical_and{},
			    [](const char &c) { return (bool)std::isdigit(c); });

			return CLI::detail::to_lower(com_part) == "com" && number_part_valid
			           ? std::string()
			           : "Invalid COM port.";
		};
	}
};

} // namespace

//! @todo strings
PianoApp::PianoApp() : commandLine("Piano app"), data()
{
	data.state = AppState::SETUP;

	arguments.port = "COM8";
	arguments.baud = 115200;
	arguments.serialSettings = Serial::ARDUINO_SETTINGS;
	arguments.volume = 0.3f;
	arguments.playback = Audio::PLAYBACK_SINE;

	Logger::console = Logger::openStaticOutputStream(std::cout);
	Logger::logLevel = Logger::Level::LVL_VERBOSE;
}

bool PianoApp::initCommandLine(int argc, const char *argv[])
{
	commandLine.add_option("-m,--midi,--midifile,--mid", arguments.midi, "The midi file to open")
	    ->check(CLI::ExistingFile);

	commandLine.add_option("--playback", arguments.playback, "The playback mode.")
	    ->transform(CLI::CheckedTransformer(Audio::PLAYBACK_MAP, CLI::ignore_case));

	commandLine.add_option("-p,--port", arguments.port, "The serial port to connect to")
	    ->check(PortValidator());

	commandLine.add_option("-b,--baud", arguments.baud, "The baud rate of the serial connection");

	commandLine.add_option("-s,--stop_bits,--stop", arguments.serialSettings.stop_bits, "The number of stop bits")
	    ->transform(CLI::CheckedTransformer(Serial::STOPBIT_MAP, CLI::ignore_case));

	commandLine.add_option("--par,--parity", arguments.serialSettings.parity, "The parity bit. N for none, E for even, O for odd")
	    ->transform(CLI::CheckedTransformer(Serial::PARITY_MAP, CLI::ignore_case));

	commandLine.add_option("--bs,--byte_size", arguments.serialSettings.byte_size, "The number of bits");
	commandLine.add_option("--volume,-v", arguments.volume, "The volume in the range [0-1]");

	try {
		commandLine.parse(argc, argv);
	}
	catch (const CLI::ParseError &exc) {
		(void)commandLine.exit(exc);
		return false;
	}

	data.state = AppState::RUNNING;
	data.game_state = GameState::SANDBOX;

	return true;
}

bool PianoApp::initAudio()
{
	m_openal_thread_handle = std::thread(openal_thread, &data, arguments.volume, arguments.playback);

	std::unique_lock lock(data.condition_variables.al_done_mutex);
	data.condition_variables.al_done.wait_for(lock, std::chrono::seconds(5));

	return data.state == AppState::RUNNING;
}

bool PianoApp::initGraphics()
{
	//! @todo strings
	return m_graphics.begin("Piano", 800, 600, &data);
}

bool PianoApp::initSerial()
{
	m_serial_thread_handle = std::thread(serial_thread, arguments, &data);

	std::unique_lock lock(data.condition_variables.serial_done_mutex);
	data.condition_variables.serial_done.wait_for(lock, std::chrono::seconds(5));

	return data.state == AppState::RUNNING;
}

void PianoApp::mainLoop()
{
	m_graphics.loop();
}

void PianoApp::cleanup()
{
	data.state = AppState::FINISHED;

	m_serial_thread_handle.join();
	m_openal_thread_handle.join();

	m_graphics.end();
}