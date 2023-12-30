#ifndef PIANO_SOUNDS_H
#define PIANO_SOUNDS_H

#include <array>
#include <bitset>
#include <mutex>
#include <unordered_set>

#include "notes.h"

struct Sounds {
public:
	constexpr static const std::size_t NUM_OCTAVES = 8;
	constexpr static const std::size_t ACTIVE_NOTES_ESTIMATE = 4;

private:
	std::array<std::bitset<12>, NUM_OCTAVES> m_sounds;
	std::array<std::mutex, NUM_OCTAVES> m_sound_mutexes;

public:
	void lockOctave(std::size_t n);
	void unlockOctave(std::size_t n);

	void toggleNote(Note note, bool on);
	void safeToggleNote(Note note, bool on);
	bool checkNote(Note note);

	std::unordered_set<Note> toNotes();

	inline void clearNote(Note note) { toggleNote(note, false); }
	inline void setNote(Note note) { toggleNote(note, true); }
};

#endif // !defined(PIANO_SOUNDS_H)