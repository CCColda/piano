#include "sounds.h"

#include <assert.h>

void Sounds::lockOctave(std::size_t n)
{
	assert(("n must be less than the number of supported octaves.", n < NUM_OCTAVES));
	m_sound_mutexes[n].lock();
}

void Sounds::unlockOctave(std::size_t n)
{
	assert(("n must be less than the number of supported octaves.", n < NUM_OCTAVES));
	m_sound_mutexes[n].unlock();
}

void Sounds::toggleNote(Note note, bool on)
{
	m_sounds[note.octave][std::uint8_t(note.key)] = on;
}

void Sounds::safeToggleNote(Note note, bool on)
{
	lockOctave(note.octave);
	m_sounds[note.octave][std::uint8_t(note.key)] = on;
	unlockOctave(note.octave);
}

bool Sounds::checkNote(Note note)
{
	return m_sounds[note.octave][std::uint8_t(note.key)];
}

std::unordered_set<Note> Sounds::toNotes()
{
	std::unordered_set<Note> result;
	result.reserve(ACTIVE_NOTES_ESTIMATE);

	for (std::size_t i = 0; i < NUM_OCTAVES; i++) {
		lockOctave(i);
		const auto &octave = m_sounds[i];

		for (std::size_t j = 0; j < 12; j++)
			if (octave[j])
				result.insert(Note{Note::Key(j), std::uint8_t(i)});

		unlockOctave(i);
	}

	return result;
}