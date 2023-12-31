#include "audio.h"

#include <cstdint>

#define _USE_MATH_DEFINES
#include <math.h>

#include <functional>

namespace {
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

#if PIANO_MIDI_ENABLED
ALuint makeMidiSource(ALuint *buffers, std::size_t buffer_count)
{
	ALuint alsrc = 0;

	alGenSources(1, &alsrc);
	alSourceQueueBuffers(alsrc, buffer_count, buffers);
	alSourcePlay(alsrc);

	return alsrc;
}

void updateMidiBuffer(ALuint albuf, fluid_synth_t *synth)
{
	std::int16_t buffer[WAVE_RES * 2];
	fluid_synth_write_s16(synth, WAVE_RES, buffer, 0, 2, buffer, 1, 2);

	alBufferData(albuf, AL_FORMAT_STEREO16, buffer, sizeof(buffer), WAVE_RES);
}

#endif
} // namespace

/* static */ std::map<std::string, Audio::Playback> Audio::PLAYBACK_MAP = {
#if PIANO_MIDI_ENABLED
    {"midi", Audio::Playback::PLAYBACK_MIDI},
#endif
    {"square", Audio::Playback::PLAYBACK_SQUARE},
    {"sine", Audio::Playback::PLAYBACK_SINE},
    {"triangle", Audio::Playback::PLAYBACK_TRIANGLE}};

bool Audio::begin(Playback iplayback)
{
	if (context == nullptr) {
		device = alcOpenDevice(nullptr);
		if (!device)
			return false;

		context = alcCreateContext(device, nullptr);
		if (!context) {
			alcCloseDevice(device);
			return false;
		}

		alcMakeContextCurrent(context);

		playback = iplayback;

		switch (playback) {
			case PLAYBACK_SINE:
				note_buffer = makeSineBuffer();
				break;
			case PLAYBACK_SQUARE:
				note_buffer = makeSquareBuffer();
				break;
			case PLAYBACK_TRIANGLE:
				note_buffer = makeTriangleBuffer();
				break;
			case PLAYBACK_MIDI:
				note_buffer = 0;
#if PIANO_MIDI_ENABLED
				auto *settings = new_fluid_settings();
				fluid_settings_setstr(settings, "player.timing-source", "sample");
				fluid_settings_setstr(settings, "synth.lock-memory", "0");

				synth = new_fluid_synth(settings);
				delete_fluid_settings(settings);

				midi_source = makeMidiSource(midi_buffers, MIDI_BUFFER_COUNT);
				alGenBuffers(MIDI_BUFFER_COUNT, midi_buffers);
				for (int i = 0; i < MIDI_BUFFER_COUNT; i++) {
					updateMidiBuffer(midi_buffers[i], synth);
				}
#endif
				break;
		}

		return true;
	}
	return false;
}

void Audio::update()
{
#if PIANO_MIDI_ENABLED
	if (playback == PLAYBACK_MIDI) {
		ALint processed_buf_count;
		alGetSourcei(midi_source, AL_BUFFERS_PROCESSED, &processed_buf_count);
		if (processed_buf_count == 0) {
			return;
		}

		auto unqueued = std::vector<ALuint>(processed_buf_count);

		alSourceUnqueueBuffers(midi_source, processed_buf_count, unqueued.data());
		for (int i = 0; i < processed_buf_count; ++i)
			updateMidiBuffer(unqueued[i], synth);

		alSourceQueueBuffers(midi_source, processed_buf_count, unqueued.data());
	}
#endif
}

void Audio::end()
{
	if (context != nullptr) {
		for (const auto &activeNote : activeNotes) {
			alSourceStop(activeNote.second);
			alDeleteSources(1, &(activeNote.second));
		}
		activeNotes.clear();

		if (playback != PLAYBACK_MIDI) {
			alDeleteBuffers(1, &note_buffer);
			note_buffer = 0;
		}
#if PIANO_MIDI_ENABLED
		else {
			fluid_synth_all_notes_off(synth, 0);

			delete_fluid_synth(synth);

			alSourceStop(midi_source);
			alDeleteSources(1, &(midi_source));
			midi_source = 0;

			alDeleteBuffers(MIDI_BUFFER_COUNT, midi_buffers);
		}
#endif

		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);

		context = nullptr;
	}
}

void Audio::setVolume(float volume)
{
	alListenerf(AL_GAIN, volume);
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
		fluid_synth_noteon(synth, 0, note.toMidi(), 64);
	}
	else
#endif
	{
		ALuint source = makeBufferedSource(note_buffer, note.toRelativePitch());

		alSourcePlay(source);

		activeNotes[note] = source;
	}
}

void Audio::stopNote(Note note)
{
#if PIANO_MIDI_ENABLED
	if (playback == Playback::PLAYBACK_MIDI) {
		fluid_synth_noteoff(synth, 0, note.toMidi());
	}
	else
#endif
	{
		const ALuint source = activeNotes[note];

		alSourceStop(source);
		alDeleteSources(1, &source);
		activeNotes.erase(note);
	}
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
