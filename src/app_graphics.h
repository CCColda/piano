#ifndef PIANO_APP_GRAPHICS_H
#define PIANO_APP_GRAPHICS_H

#include "app_data.h"
#include "notes.h"

#include <neonEngine.h>

#include <win32.hpp>

class AppGraphics {
public:
	constexpr static const Note STARTING_NOTE = Note{Note::Key::C, 4};
	constexpr static const Note ENDING_NOTE = Note{Note::Key::C, 7};
	constexpr static const float NOTE_WIDTH_MULTIPLIER = 0.9f;
	constexpr static const float PIANO_WIDTH_MULTIPLIER = 0.96f;

private:
	Platform::Win32::PlatformContext m_platform_context;
	AppData *data;

	std::unordered_map<Note, Neon::EngineObjectPtr> notes;
	Neon::EnginePtr neon;
	Neon::NodePtr piano_scene;

private:
	void initGraphics();
	void mainLoop(Platform::Win32::PlatformContext *const context);
	void initPiano();

public:
	bool begin(const char *window_title, unsigned w, unsigned h, AppData *data);

	void loop();

	void end();
};

#endif // !defined(PIANO_APP_GRAPHICS_H)