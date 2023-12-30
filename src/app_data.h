#ifndef PIANO_APP_DATA_H
#define PIANO_APP_DATA_H

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "audio.h"
#include "serial.h"
#include "sounds.h"

enum AppState {
	SETUP = 0,
	RUNNING = 1,
	FINISHED = 2
};

struct AppConditionVars {
	std::mutex serial_done_mutex, al_done_mutex;
	std::condition_variable serial_done, al_done;
};

struct AppData {
	Audio audio;
	Sounds sounds;
	AppConditionVars condition_variables;

	std::atomic_int state;
};

struct AppCommandLine {
	std::string port;
	unsigned int baud;
	Serial::Settings serialSettings;
	float volume;
};

#endif // !defined(PIANO_APP_DATA_H)