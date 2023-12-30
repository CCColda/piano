#ifndef PIANO_APP_H
#define PIANO_APP_H

#include "app_data.h"
#include "serial.h"

#include <CLI/CLI.hpp>
#include <win32.hpp>

#include <thread>

class PianoApp {
public:
	AppData data;
	CLI::App commandLine;

	AppCommandLine arguments;

private:
	std::thread m_serial_thread_handle;
	std::thread m_openal_thread_handle;
	Platform::Win32::PlatformContext m_platform_context;

public:
	PianoApp();

	bool initCommandLine(int argc, const char *argv[]);
	bool initAudio();
	bool initGraphics();
	bool initSerial();

	void mainLoop();

	void cleanup();
};

#endif // !defined(PIANO_APP_H)