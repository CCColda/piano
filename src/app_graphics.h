#ifndef PIANO_APP_GRAPHICS_H
#define PIANO_APP_GRAPHICS_H

#include "app_data.h"
#include "notes.h"

#include <neonBitmapText.h>
#include <neonEngine.h>

#include <win32.hpp>

class AppGraphics {
public:
	constexpr static const Note STARTING_NOTE = Note{Note::Key::C, 4};
	constexpr static const Note ENDING_NOTE = Note{Note::Key::C, 7};
	constexpr static const float NOTE_WIDTH_MULTIPLIER = 0.9f;
	constexpr static const float PIANO_WIDTH_MULTIPLIER = 0.96f;
	constexpr static const float PIANO_HEIGHT_PIXELS = 100.0f;
	constexpr static const float HALF_NOTE_HEIGHT_MULTIPLIER = 3.0f / 5.0f;
	constexpr static const float SCREEN_PER_SECOND = 1.0f / 8.0f;
	constexpr static const int COUNTDOWN_BEGIN = 9;

	static_assert(COUNTDOWN_BEGIN < 10 && COUNTDOWN_BEGIN > 0, "COUNTDOWN_BEGIN must be a positive integer less than 10.");

	using clock = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock>;

private:
	Platform::Win32::PlatformContext m_platform_context;
	AppData *data;

	std::unordered_map<Note, Neon::EngineObjectPtr> keys;
	Neon::EnginePtr neon;
	Neon::NodePtr piano_scene;
	Neon::BitmapTextManagerComponentPtr countdown;
	Neon::EngineObjectPtr countdown_render;

	struct Countdown {
		bool active;
		time_point begin;
		int value;
	};

	Countdown countdown_data;

public:
	struct NoteInfo {
		Note n;
		time_point begin;
		time_point end;
	};

	std::vector<NoteInfo> notes;

private:
	void initGraphics();
	void mainLoop(Platform::Win32::PlatformContext *const context);
	void initPiano();
	void initCountdown();

public:
	bool begin(const char *window_title, unsigned w, unsigned h, AppData *data);

	void beginCountdown();

	void loop();

	void end();
};

#endif // !defined(PIANO_APP_GRAPHICS_H)