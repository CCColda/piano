#ifndef PIANO_APP_GRAPHICS_H
#define PIANO_APP_GRAPHICS_H

#include "app_data.h"
#include "notes.h"

#include <neonBitmapText.h>
#include <neonEngine.h>

#include <functional>

#include <Geometry.hpp>
#include <win32.hpp>

class AppGraphics {
public:
	constexpr static const Note STARTING_NOTE = Note{Note::Key::C, 4};
	constexpr static const Note ENDING_NOTE = Note{Note::Key::C, 7};
	constexpr static const float NOTE_WIDTH_MULTIPLIER = 0.9f;
	constexpr static const float PIANO_WIDTH_MULTIPLIER = 0.96f;
	constexpr static const float PIANO_HEIGHT_PIXELS = 100.0f;
	constexpr static const float HALF_NOTE_HEIGHT_MULTIPLIER = 3.0f / 5.0f;

	constexpr static const Calcda::Vector4 NOTE_COLOR = Calcda::Vector4(1.0, 1.0, 1.0, 1.0);
	constexpr static const Calcda::Vector4 ACTIVE_NOTE_COLOR = Calcda::Vector4(0.0, 1.0, 0.0, 1.0);
	constexpr static const Calcda::Vector4 SHARP_NOTE_COLOR = Calcda::Vector4(0.0, 0.0, 0.0, 1.0);

	constexpr static const float PIANO_BEGIN_X = (1.0f - PIANO_WIDTH_MULTIPLIER) * 0.5f;

	using clock = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock>;

	struct MidiNote {
		Note n;
		float begin;
		float duration;
	};

public:
	std::function<void(unsigned, unsigned, Platform::ClickType, Platform::ClickDirection)> onClick;
	Neon::EnginePtr neon;

private:
	Platform::Win32::PlatformContext m_platform_context;
	AppData *data;

	std::unordered_map<Note, Neon::EngineObjectPtr> m_piano_keys;
	std::vector<std::pair<MidiNote, Neon::EngineObjectPtr>> m_midi_note_pairs;

	Neon::NodePtr m_piano_scene;
	Neon::BitmapTextManagerComponentPtr m_countdown_manager;
	Neon::EngineObjectPtr m_countdown_render;

	Calcda::Vector2 m_resolution;

	unsigned m_countdown_begin;
	float m_maxkeywidth;

	float m_yscale;

	struct Countdown {
		bool active;
		time_point begin;
		int value;
	};

	struct MidiData {
		bool active;
		time_point playing_started;
	};

	Countdown m_countdown_data;
	MidiData m_midi_data;

	Neon::ShaderComponentPtr m_midishader;
	Neon::ShaderComponentPtr m_keyshader;

private:
	static float calculateNoteMaxWidth();
	static float calculateNoteXPosition(Note note, float maxwidth);

	Neon::EngineObjectPtr createMidiObject(Note note, float timestamp, float duration);
	Neon::EngineObjectPtr createKeyObject(Note note, float x);

	void initGraphics();
	void mainLoop(Platform::Win32::PlatformContext *const context);
	void initShaders();
	void initPiano();
	void initCountdown();

	void updateCountdown();
	void updateMidi();
	void updateKeys();

public:
	AppGraphics();

	bool begin(const char *window_title, unsigned w, unsigned h, unsigned countdown_begin, float yscale, AppData *data);

	void beginCountdown();
	void setNotes(const std::vector<MidiNote> &notes);

	void loop();

	void end();

	inline bool isGameActive() const { return m_countdown_data.active || m_midi_data.active; }
};

#endif // !defined(PIANO_APP_GRAPHICS_H)