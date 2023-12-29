#include "notes.h"

#include <cmath>

constexpr static const char *KEY_STRING_MAP[] = {
    "C",
    "C#",
    "D",
    "D#",
    "E",
    "F",
    "F#",
    "G",
    "G#",
    "A",
    "A#",
    "B"};

//! @see https://newt.phys.unsw.edu.au/jw/notes.html

float Note::toRelativePitch() const
{
	const std::uint8_t midi = toMidi();
	return static_cast<float>(std::pow(2.0, double(midi - 69) / 12.0));
}

float Note::toPitch() const
{
	return toRelativePitch() * 440.0f;
}

std::ostream &operator<<(std::ostream &o, const Note::Key &key)
{
	o << KEY_STRING_MAP[std::uint8_t(key)];
	return o;
}

std::ostream &operator<<(std::ostream &o, const Note &note)
{
	o << note.key << int(note.octave);
	return o;
}