#ifndef PIANO_AUDIO_H
#define PIANO_AUDIO_H

#include <AL.h>
#include <ALC.h>

#include <chrono>
#include <list>
#include <vector>

#include <unordered_map>
#include <unordered_set>

#include "notes.h"

class Audio {
protected:
	ALCcontext *context;
	ALCdevice *device;

	ALuint sine_buffer;

	std::unordered_map<Note, ALuint> activeNotes;

public:
	bool begin();
	void end();

	void setVolume(float volume);

	std::unordered_set<Note> getActiveNotes() const;
	void playNote(Note note);
	void stopNote(Note note);

	bool active() const;
};

#endif // !defined(PIANO_AUDIO_H)