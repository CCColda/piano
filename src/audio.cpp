#include "audio.h"

#include <cstdint>

#define _USE_MATH_DEFINES
#include <math.h>

#include <functional>

namespace {
#if PIANO_AL_ENABLED
constexpr static const size_t WAVE_RES = 11025;
constexpr static const double BASE_FREQUENCY = 440.0;
ALuint makeSineBuffer()
{
	ALuint buf = 0;
	alGenBuffers(1, &buf);

	std::int16_t sine_wave[WAVE_RES];
	for (unsigned int i = 0; i < WAVE_RES; i++) {
		sine_wave[i] = std::int16_t(sin((M_PI * 2.0 * BASE_FREQUENCY) / (double)WAVE_RES * (double)i) * 32760.0);
	}

	alBufferData(buf, AL_FORMAT_MONO16, sine_wave, sizeof(sine_wave), WAVE_RES);

	return buf;
}

ALuint makeSquareBuffer()
{
	ALuint buf = 0;
	alGenBuffers(1, &buf);

	std::int16_t square_wave[WAVE_RES];
	for (unsigned int i = 0; i < WAVE_RES; i++) {
		square_wave[i] = std::int16_t((((int(i) / int(BASE_FREQUENCY)) % 2) - 1) * 32760.0);
	}

	alBufferData(buf, AL_FORMAT_MONO16, square_wave, sizeof(square_wave), WAVE_RES);

	return buf;
}

ALuint makeTriangleBuffer()
{
	ALuint buf = 0;
	alGenBuffers(1, &buf);

	std::int16_t triangle_wave[WAVE_RES];
	for (unsigned int i = 0; i < WAVE_RES; i++) {
		triangle_wave[i] = std::int16_t(((i % int(BASE_FREQUENCY)) - (int(BASE_FREQUENCY) / 2)) * 32760.0);
	}

	alBufferData(buf, AL_FORMAT_MONO16, triangle_wave, sizeof(triangle_wave), WAVE_RES);

	return buf;
}

ALuint makeBufferedSource(ALuint buf, float pitch)
{
	ALuint src = 0;
	alGenSources(1, &src);

	alSourcei(src, AL_BUFFER, buf);
	alSourcei(src, AL_LOOPING, 1);
	alSourcef(src, AL_PITCH, pitch);

	return src;
}
#endif
} // namespace

/* static */ std::map<std::string, Audio::Playback> Audio::PLAYBACK_MAP = {
#if PIANO_MIDI_ENABLED
    {"midi", Audio::Playback::PLAYBACK_MIDI},
#endif
#if PIANO_AL_ENABLED
    {"square", Audio::Playback::PLAYBACK_SQUARE},
    {"sine", Audio::Playback::PLAYBACK_SINE},
    {"triangle", Audio::Playback::PLAYBACK_TRIANGLE}
#endif
};

bool Audio::begin(Playback iplayback, std::string soundfont)
{
#if PIANO_AL_ENABLED
	if (iplayback != PLAYBACK_MIDI) {
		if (context != nullptr)
			return false;

		device = alcOpenDevice(nullptr);
		if (!device)
			return false;

		context = alcCreateContext(device, nullptr);
		if (!context) {
			alcCloseDevice(device);
			return false;
		}

		alcMakeContextCurrent(context);
	}
#endif

	playback = iplayback;

	switch (playback) {
#if PIANO_AL_ENABLED
		case PLAYBACK_SINE:
			note_buffer = makeSineBuffer();
			break;
		case PLAYBACK_SQUARE:
			note_buffer = makeSquareBuffer();
			break;
		case PLAYBACK_TRIANGLE:
			note_buffer = makeTriangleBuffer();
			break;
#endif
#if PIANO_MIDI_ENABLED
		case PLAYBACK_MIDI:
			settings = new_fluid_settings();

			synth = new_fluid_synth(settings);
			driver = new_fluid_audio_driver(settings, synth);
			fluid_synth_sfload(synth, soundfont.c_str(), 1);
			break;
#endif
	}

	return true;
}

void Audio::end()
{
#if PIANO_AL_ENABLED
	if (playback != PLAYBACK_MIDI) {
		if (context == nullptr)
			return;

		for (const auto &activeNote : activeNotes) {
			alSourceStop(activeNote.second);
			alDeleteSources(1, &(activeNote.second));
		}
		activeNotes.clear();

		alDeleteBuffers(1, &note_buffer);
		note_buffer = 0;

		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);

		context = nullptr;
	}
#endif

#if PIANO_MIDI_ENABLED
	if (playback == PLAYBACK_MIDI) {
		fluid_synth_all_notes_off(synth, 0);

		delete_fluid_audio_driver(driver);
		delete_fluid_synth(synth);
		delete_fluid_settings(settings);
	}
#endif
}

void Audio::setVolume(float volume)
{
#if PIANO_AL_ENABLED
	if (playback != PLAYBACK_MIDI) {
		alListenerf(AL_GAIN, volume);
	}
#endif

#ifdef PIANO_MIDI_ENABLED
	if (playback == PLAYBACK_MIDI) {
		fluid_settings_setnum(settings, "synth.gain", (double)volume);
	}
#endif
}

std::unordered_set<Note> Audio::getActiveNotes() const
{
	std::unordered_set<Note> result;
	std::transform(activeNotes.begin(), activeNotes.end(),
	               std::inserter(result, result.end()),
	               [](const auto &pair) { return pair.first; });
	return result;
}

void Audio::playNote(Note note)
{
#if PIANO_MIDI_ENABLED
	if (playback == Playback::PLAYBACK_MIDI) {
		std::printf("Playing %d %d\n", note.key, note.octave);
		fluid_synth_noteon(synth, 0, note.toMidi(), 80);
		activeNotes[note] = 1;
	}
#endif

#if PIANO_AL_ENABLED
	if (playback != PLAYBACK_MIDI) {
		ALuint source = makeBufferedSource(note_buffer, note.toRelativePitch());

		alSourcePlay(source);

		activeNotes[note] = source;
	}
#endif
}

void Audio::stopNote(Note note)
{
#if PIANO_MIDI_ENABLED
	if (playback == Playback::PLAYBACK_MIDI) {
		std::printf("Stopping %d %d\n", note.key, note.octave);
		fluid_synth_noteoff(synth, 0, note.toMidi());
		activeNotes.erase(note);
	}
#endif

#if PIANO_AL_ENABLED
	if (playback != PLAYBACK_MIDI) {
		const ALuint source = activeNotes[note];

		alSourceStop(source);
		alDeleteSources(1, &source);
		activeNotes.erase(note);
	}
#endif
}

bool Audio::active() const
{
	return !activeNotes.empty();
}

std::ostream &operator<<(std::ostream &os, const Audio::Playback &par)
{
	for (const auto &entry : Audio::PLAYBACK_MAP) {
		if (entry.second == par) {
			os << entry.first;
			break;
		}
	}

	return os;
}
