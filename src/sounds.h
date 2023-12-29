#ifndef PIANO_SOUNDS_H
#define PIANO_SOUNDS_H

#include <mutex>
#include <unordered_set>

#include "notes.h"

struct Sounds {
private:
	std::unordered_set<Note> sounds;
	std::mutex soundMutex;

public:
	std::unordered_set<Note> getCopy();
	void setTo(const std::unordered_set<Note> &other);
};

#endif // !defined(PIANO_SOUNDS_H)