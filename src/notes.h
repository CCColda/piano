#ifndef PIANO_NOTES_H
#define PIANO_NOTES_H

#include <cstdint>
#include <ostream>

struct Note {
	// clang-format off
	enum Key : std::uint8_t { C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B };
	// clang-format on

	Key key;
	std::uint8_t octave;

	float toPitch() const;
	float toRelativePitch() const;

	constexpr std::uint8_t toMidi() const { return octave * 12 + std::uint8_t(key) + 21; }

	constexpr int compare(const Note &other) const { return other.toMidi() - toMidi(); }

	constexpr bool operator<(const Note &other) const { return compare(other) < 0; }
	constexpr bool operator<=(const Note &other) const { return compare(other) <= 0; }
	constexpr bool operator>(const Note &other) const { return compare(other) > 0; }
	constexpr bool operator>=(const Note &other) const { return compare(other) >= 0; }
	constexpr bool operator==(const Note &other) const { return compare(other) == 0; }
	constexpr bool operator!=(const Note &other) const { return compare(other) != 0; }
};

std::ostream &operator<<(std::ostream &o, const Note::Key &key);
std::ostream &operator<<(std::ostream &o, const Note &note);

template <>
struct std::hash<Note> {
	constexpr std::size_t operator()(const Note& note) const noexcept {
		return note.toMidi();
	}
};

#endif // !defined(PIANO_NOTES_H)