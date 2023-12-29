#ifndef PIANO_GLOBAL_H
#define PIANO_GLOBAL_H

#include <atomic>

#include "sounds.h"

enum AppState {
	SETUP = 0,
	RUNNING = 1,
	FINISHED = 2
};

struct Global {
	Sounds sounds;
	std::atomic_int state;
};

#endif // !defined(PIANO_GLOBAL_H)