#include <iostream>

#include "app.h"

#include <MidiFile.h>

int main(int argc, char *argv[])
{
	PianoApp app;

	if (!app.initCommandLine(argc, const_cast<const char **>(argv))) {
		return 1;
	}

	if (!app.initAudio()) {
		app.cleanup();
		std::cerr << "Failed initializing OpenAL." << std::endl;
		return 2;
	}

	if (!app.initSerial()) {
		app.cleanup();
		std::cerr << "Failed initializing serial port." << std::endl;
		return 3;
	}

	if (!app.initGraphics()) {
		app.cleanup();
		std::cerr << "Failed initializing graphics." << std::endl;
		return 4;
	}

	app.mainLoop();

	app.cleanup();

	return 0;
}