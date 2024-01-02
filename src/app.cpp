#include "app.h"

#include <Logger.h>

#include "serial_parser.h"
#include "windows_serial.h"

#include <chrono>

#include "app_audio_thread.h"
#include "app_serial_thread.h"

#include <MidiFile.h>

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

std::vector<AppGraphics::MidiNote> loadNotesFromFile(const std::string &path, int transpose)
{
	smf::MidiFile midifile;
	midifile.read(path);

	midifile.doTimeAnalysis();
	midifile.linkNotePairs();

	std::vector<AppGraphics::MidiNote> result;

	for (int track = 0; track < midifile.getTrackCount(); track++) {
		result.reserve(midifile[track].size());

		for (int ev = 0; ev < midifile[track].size(); ev++) {
			if (midifile[track][ev].isNoteOn()) {
				const Note note = Note::fromMidi(midifile[track][ev][1] + transpose);

				if (note >= AppGraphics::STARTING_NOTE && note <= AppGraphics::ENDING_NOTE) {
					result.push_back(AppGraphics::MidiNote{
					    note,
					    (float)midifile[track][ev].seconds,
					    (float)midifile[track][ev].getDurationInSeconds()});
				}
				else {
					std::cerr << "Note " << note << " on track " << track << " (event " << ev << ") is invalid." << std::endl;
				}
			}
		}
	}

	result.shrink_to_fit();

	return result;
}

} // namespace

//! @todo strings
PianoApp::PianoApp() : commandLine("Piano app"), data()
{
	data.state = AppState::SETUP;

	arguments.port = "COM8";
	arguments.baud = 115200;
	arguments.serialSettings = Serial::ARDUINO_SETTINGS;
	arguments.volume = 0.3f;
#if PIANO_AL_ENABLED
	arguments.playback = Audio::PLAYBACK_SINE;
#else
	arguments.playback = Audio::PLAYBACK_MIDI;
#endif
	arguments.countdown = 3;
	arguments.yscale = 100.0f;
	arguments.midi_transpose = 0;

	Logger::console = Logger::openStaticOutputStream(std::cout);
	Logger::logLevel = Logger::Level::LVL_INFO;
}

bool PianoApp::initCommandLine(int argc, const char *argv[])
{
	commandLine.add_option("-m,--midi,--midifile,--mid", arguments.midi, "The midi file to open")
	    ->check(CLI::ExistingFile);

	commandLine.add_option("-f,--sf,--soundfont", arguments.soundfont, "The soundfont file to open")
	    ->check(CLI::ExistingFile);

	commandLine.add_option("--midi-transpose,--transpose", arguments.midi_transpose, "The number of midi notes to transpose by");

	commandLine.add_option("--countdown", arguments.countdown, "The countdown before starting the song")
	    ->check(CLI::Range(0u, 9u, "COUNTDOWN"));

	commandLine.add_option("--yscale", arguments.yscale, "The vertical stretching of bars. Given in the units of pixel/second.")
	    ->check(CLI::Range(1.0f, INFINITY, "YSCALE"));

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
	m_openal_thread_handle = std::thread(openal_thread, &data, arguments.volume, arguments.playback, arguments.soundfont);

	std::unique_lock lock(data.condition_variables.al_done_mutex);
	data.condition_variables.al_done.wait_for(lock, std::chrono::seconds(5));

	return data.state == AppState::RUNNING;
}

bool PianoApp::initGraphics()
{
	//! @todo strings
	m_graphics.onClick = [this](unsigned x, unsigned y, Platform::ClickType t, Platform::ClickDirection d) {
		this->onClick(x, y, t, d);
	};

	return m_graphics.begin("Piano", 800, 600, arguments.countdown, arguments.yscale, &data);
}

bool PianoApp::initSerial()
{
	m_serial_thread_handle = std::thread(serial_thread, arguments, &data);

	std::unique_lock lock(data.condition_variables.serial_done_mutex);
	data.condition_variables.serial_done.wait_for(lock, std::chrono::seconds(5));

	return data.state == AppState::RUNNING;
}

void PianoApp::onClick(unsigned x, unsigned y, Platform::ClickType t, Platform::ClickDirection d)
{
	std::cout << "Click: " << x << " " << y << " t=" << (int)t << " d=" << (int)d << std::endl;

	if (d == Platform::ClickDirection::DOWN && t == Platform::ClickType::LEFT) {
		if (!m_graphics.isGameActive()) {
			m_graphics.setNotes(loadNotesFromFile(arguments.midi, arguments.midi_transpose));
			m_graphics.beginCountdown();
		}
	}
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