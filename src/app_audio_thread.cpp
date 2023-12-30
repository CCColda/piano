#include "app_audio_thread.h"

#include "audio.h"

#include <thread>

std::unordered_set<Note> setDifference(const std::unordered_set<Note> &base, const std::unordered_set<Note> &other)
{
	std::unordered_set<Note> result;
	for (const auto &item : base) {
		if (other.count(item) == 0)
			result.insert(item);
	}

	return result;
}

void openal_thread(AppData *data, float volume)
{
	if (!data->audio.begin()) {
		data->state = AppState::FINISHED;
	}
	else {
		data->audio.setVolume(volume);
	}

	data->condition_variables.al_done.notify_one();

	while (data->state == AppState::RUNNING) {
		const auto playedNotes = data->audio.getActiveNotes();
		const auto localSounds = data->sounds.toNotes();

		std::unordered_set<Note> locallyAvailableButNotPlayed = setDifference(localSounds, playedNotes),
		                         playedButNotLocallyAvailable = setDifference(playedNotes, localSounds);

		for (const auto &noteToStop : playedButNotLocallyAvailable)
			data->audio.stopNote(noteToStop);

		for (const auto &noteToPlay : locallyAvailableButNotPlayed)
			data->audio.playNote(noteToPlay);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	data->audio.end();
}
