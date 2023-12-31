#ifndef PIANO_AUDIO_H
#define PIANO_AUDIO_H

#if PIANO_AL_ENABLED
#include <AL.h>
#include <ALC.h>
#endif

#include <chrono>
#include <list>
#include <vector>

#include <map>
#include <unordered_map>
#include <unordered_set>

#include "notes.h"

#if PIANO_MIDI_ENABLED
#include <fluidsynth.h>
#endif

class Audio {
public:
	constexpr static const std::size_t MIDI_BUFFER_COUNT = 3;

	enum Playback : std::uint8_t {
		PLAYBACK_SINE,
		PLAYBACK_SQUARE,
		PLAYBACK_TRIANGLE,
		PLAYBACK_MIDI
	};

protected:
#if PIANO_AL_ENABLED
	ALCcontext *context;
	ALCdevice *device;

	ALuint note_buffer;
#endif

#if PIANO_MIDI_ENABLED
	fluid_synth_t *synth;
	fluid_audio_driver_t *driver;
	fluid_settings_t *settings;
#endif

	std::unordered_map<Note, unsigned int> activeNotes;

	Audio::Playback playback;

public:
	static std::map<std::string, Playback> PLAYBACK_MAP;

public:
	bool begin(Audio::Playback playback, std::string soundfont);
	void end();

	void setVolume(float volume);

	std::unordered_set<Note> getActiveNotes() const;
	void playNote(Note note);
	void stopNote(Note note);

	bool active() const;
};

std::ostream &operator<<(std::ostream &o, const Audio::Playback &p);

#endif // !defined(PIANO_AUDIO_H)