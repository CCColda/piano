#include "sounds.h"

std::unordered_set<Note> Sounds::getCopy()
{
	std::unordered_set<Note> result;

	{
		const std::lock_guard guard(soundMutex);
		result = sounds;
	}

	return result;
}

void Sounds::setTo(const std::unordered_set<Note> &other)
{
	const std::lock_guard guard(soundMutex);
	sounds = other;
}
