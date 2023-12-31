#ifndef PIANO_APP_AUDIO_THREAD_H
#define PIANO_APP_AUDIO_THREAD_H

#include "app_data.h"
#include "audio.h"

void openal_thread(AppData *data, float volume, Audio::Playback playback);

#endif // !defined(PIANO_APP_AUDIO_THREAD_H)