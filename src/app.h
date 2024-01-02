#ifndef PIANO_APP_H
#define PIANO_APP_H

#include "app_data.h"
#include "serial.h"

#include <CLI/CLI.hpp>

#include "app_graphics.h"

#include <thread>

class PianoApp {
public:
	AppData data;
	CLI::App commandLine;

	AppCommandLine arguments;

private:
	AppGraphics m_graphics;
	std::thread m_serial_thread_handle;
	std::thread m_openal_thread_handle;

public:
	PianoApp();

	bool initCommandLine(int argc, const char *argv[]);
	bool initAudio();
	bool initGraphics();
	bool initSerial();

	void onClick(unsigned x, unsigned y, Platform::ClickType t, Platform::ClickDirection d);

	void mainLoop();

	void cleanup();
};

#endif // !defined(PIANO_APP_H)