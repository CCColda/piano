#include "audio.h"

#include <cstdint>

#define _USE_MATH_DEFINES
#include <math.h>

#include <functional>

namespace {
constexpr static const size_t SINE_WAVE_RES = 11025;
constexpr static const double BASE_FREQUENCY = 440.0;
ALuint makeSineBuffer()
{
	ALuint buf = 0;
	alGenBuffers(1, &buf);

	std::int16_t sin_wave[SINE_WAVE_RES];
	for (unsigned int i = 0; i < SINE_WAVE_RES; i++) {
		sin_wave[i] = std::int16_t(sin((M_PI * 2.0 * BASE_FREQUENCY) / (double)SINE_WAVE_RES * (double)i) * 32760.0);
	}

	alBufferData(buf, AL_FORMAT_MONO16, sin_wave, SINE_WAVE_RES * sizeof(std::int16_t), SINE_WAVE_RES);

	return buf;
}

ALuint makeSineSource(ALuint buf, float pitch)
{
	ALuint src = 0;
	alGenSources(1, &src);

	alSourcei(src, AL_BUFFER, buf);
	alSourcei(src, AL_LOOPING, 1);
	alSourcef(src, AL_PITCH, pitch);

	return src;
}
} // namespace

bool Audio::begin()
{
	device = alcOpenDevice(nullptr);
	if (!device)
		return false;

	context = alcCreateContext(device, nullptr);
	if (!context) {
		alcCloseDevice(device);
		return false;
	}

	alcMakeContextCurrent(context);

	sine_buffer = makeSineBuffer();

	return true;
}

void Audio::end()
{
	for (const auto &activeNote : activeNotes) {
		alSourceStop(activeNote.second);
		alDeleteSources(1, &(activeNote.second));
	}
	activeNotes.clear();

	alDeleteBuffers(1, &sine_buffer);

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
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
	const ALuint source = makeSineSource(sine_buffer, note.toRelativePitch());

	alSourcePlay(source);

	activeNotes[note] = source;
}

void Audio::stopNote(Note note)
{
	const ALuint source = activeNotes[note];

	alSourceStop(source);
	alDeleteSources(1, &source);
	activeNotes.erase(note);
}

bool Audio::active() const
{
	return !activeNotes.empty();
}