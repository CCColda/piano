#include "app_graphics.h"

#include <chrono>
#include <sstream>
#include <thread>

#include <iostream>

#include <neonBitmapFont.h>
#include <neonComponent.h>
#include <neonEngine.h>
#include <neonResourceLoader.h>

namespace {
Neon::ShaderComponentPtr loadKeyShader()
{
	using namespace Neon;

	auto shader = ShaderComponent::create("key_shader");
	shader->addLocalNode(ResourceLoader::loadShaderStage("vertex", GL_VERTEX_SHADER, "data/key.vertex.glsl"));
	shader->addLocalNode(ResourceLoader::loadShaderStage("fragment", GL_FRAGMENT_SHADER, "data/key.fragment.glsl"));
	shader->link();
	shader->bindAttribLocation(0, "i_pos");
	shader->addLocalNode(UniformComponent::create("u_color"));
	shader->addLocalNode(UniformComponent::create("u_mat"));
	return shader;
}

Neon::ShaderComponentPtr loadMidiShader()
{
	using namespace Neon;

	auto shader = ShaderComponent::create("midi_shader");
	shader->addLocalNode(ResourceLoader::loadShaderStage("vertex", GL_VERTEX_SHADER, "data/midi.vertex.glsl"));
	shader->addLocalNode(ResourceLoader::loadShaderStage("fragment", GL_FRAGMENT_SHADER, "data/midi.fragment.glsl"));
	shader->link();
	shader->bindAttribLocation(0, "i_pos");
	shader->addLocalNode(UniformComponent::create("u_offset"));
	shader->addLocalNode(UniformComponent::create("u_yscale"));
	shader->addLocalNode(UniformComponent::create("u_cutoff"));
	shader->addLocalNode(UniformComponent::create("u_mat"));
	return shader;
}
} // namespace

/* static */ float AppGraphics::calculateNoteXPosition(Note note, float maxwidth)
{
	float x = PIANO_BEGIN_X;
	for (Note n = STARTING_NOTE; n < note; n = Note::fromMidi(n.toMidi() + 1)) {
		const bool is_sharp = n.isSharp();

		if (!is_sharp)
			x += maxwidth;
	}

	return x;
}

/* static */ float AppGraphics::calculateNoteMaxWidth()
{
	unsigned num_full_keys = 0;
	for (Note n = STARTING_NOTE; n <= ENDING_NOTE; n = Note::fromMidi(n.toMidi() + 1)) {
		if (!n.isSharp())
			num_full_keys++;
	}

	return (PIANO_WIDTH_MULTIPLIER / float(num_full_keys));
}

Neon::EngineObjectPtr AppGraphics::createMidiObject(Note note, float timestamp, float duration)
{
	using namespace Neon;
	using namespace cppx;

	const bool is_sharp = note.isSharp();

	const float width = m_maxkeywidth * NOTE_WIDTH_MULTIPLIER;
	const float offset_x = calculateNoteXPosition(note, m_maxkeywidth) + (is_sharp ? -width * 0.5f : 0.0f);
	const float base_y = 0.0;
	const float extent_y = -duration;

	const float mesh[] = {
	    offset_x, base_y,
	    offset_x + width, base_y,
	    offset_x + width, extent_y,
	    offset_x, extent_y};

	static const unsigned idx[] = {0, 1, 2, 2, 3, 0};

	auto obj = EngineObject::create("midi_" + std::to_string(note.toMidi()) + "_" + std::to_string(timestamp));

	obj->addLocalNode(RenderComponent::create("midi_render"));
	obj->addNode("midi_render",
	             MeshComponent::create(
	                 "midi_mesh",
	                 Buffer::Static((void *)mesh, sizeof(mesh)),
	                 MeshComponent::LayoutList{{0, 2, GL_FLOAT, GL_FALSE, 0}},
	                 Buffer::Static((void *)idx, sizeof(idx))));

	obj->addLocalNode(m_midishader);

	auto su_mat = Neon::UniformStorageComponent::create("su_mat", m_midishader->getNodeByPath<Neon::UniformComponent>("$u_mat"));
	su_mat->set(Calcda::Matrix4::orthographic(0.0, 1.0, 0.0, m_resolution.y, -1.0, 1.0).transpose());
	obj->addLocalNode(su_mat);

	auto su_offset = Neon::UniformStorageComponent::create("su_offset", m_midishader->getNodeByPath<Neon::UniformComponent>("$u_offset"));
	su_offset->set(timestamp);
	obj->addLocalNode(su_offset);

	auto su_yscale = Neon::UniformStorageComponent::create("su_yscale", m_midishader->getNodeByPath<Neon::UniformComponent>("$u_yscale"));
	su_yscale->set(m_yscale);
	obj->addLocalNode(su_yscale);

	auto su_cutoff = Neon::UniformStorageComponent::create("su_cutoff", m_midishader->getNodeByPath<Neon::UniformComponent>("$u_cutoff"));
	su_cutoff->set(m_resolution.y - PIANO_HEIGHT_PIXELS);
	obj->addLocalNode(su_cutoff);

	auto normalAssoc = obj->createAssociation("assoc_normal");
	normalAssoc->setRender("midi_render");
	normalAssoc->setShader(m_midishader);
	normalAssoc->addUniformStorages(su_mat, su_offset, su_yscale, su_cutoff);
	normalAssoc->renderPass = -1;

	obj->zIndex = 0;

	obj->visible = true;

	return obj;
}

Neon::EngineObjectPtr AppGraphics::createKeyObject(Note note, float x)
{
	using namespace Neon;
	using namespace cppx;

	const bool is_sharp = note.isSharp();

	const float width = m_maxkeywidth * NOTE_WIDTH_MULTIPLIER;
	const float offset_x = x + (is_sharp ? -width * 0.5f : 0.0f);
	const float base_y = m_resolution.y - PIANO_HEIGHT_PIXELS;
	const float extent_y = base_y + PIANO_HEIGHT_PIXELS * (is_sharp ? HALF_NOTE_HEIGHT_MULTIPLIER : 1.0f);

	const float mesh[] = {
	    offset_x, base_y,
	    offset_x + width, base_y,
	    offset_x + width, extent_y,
	    offset_x, extent_y};

	static const unsigned idx[] = {0, 1, 2, 2, 3, 0};

	auto obj = EngineObject::create("key_" + std::to_string(note.toMidi()));

	obj->addLocalNode(RenderComponent::create("key_render"));
	obj->addNode("key_render",
	             MeshComponent::create(
	                 "key_mesh",
	                 Buffer::Static((void *)mesh, sizeof(mesh)),
	                 MeshComponent::LayoutList{{0, 2, GL_FLOAT, GL_FALSE, 0}},
	                 Buffer::Static((void *)idx, sizeof(idx))));

	obj->addLocalNode(m_keyshader);

	auto su_mat = UniformStorageComponent::create("su_mat", m_keyshader->getNodeByPath<Neon::UniformComponent>("$u_mat"));
	su_mat->set(Calcda::Matrix4::orthographic(0.0, 1.0, 0.0, m_resolution.y, -1.0, 1.0).transpose());
	obj->addLocalNode(su_mat);

	auto su_color = UniformStorageComponent::create("su_color", m_keyshader->getNodeByPath<Neon::UniformComponent>("$u_color"));
	su_color->set(is_sharp ? SHARP_NOTE_COLOR : NOTE_COLOR);
	obj->addLocalNode(su_color);

	auto normalAssoc = obj->createAssociation("assoc_normal");
	normalAssoc->setRender("key_render");
	normalAssoc->setShader(m_keyshader);
	normalAssoc->addUniformStorages(su_mat, su_color);
	normalAssoc->renderPass = 0;

	obj->zIndex = is_sharp ? 2 : 1;

	obj->visible = true;

	return obj;
}

void AppGraphics::initGraphics()
{
	using namespace Neon;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE0);
	glEnable(GL_TEXTURE1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	neon->options.outDim = {800.0f, 600.0f};
	neon->options.postprocDim = {800.0f, 600.0f};
	neon->options.backgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
	neon->options.clear = true;

	neon->onSwapBuffers = [this]() -> void { SwapBuffers(this->m_platform_context.hdc); };
	neon->applyOptions();

	m_piano_scene = Node::create("piano_scene");
	neon->scenes->addLocalNode(m_piano_scene);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void AppGraphics::initShaders()
{
	m_keyshader = loadKeyShader();
	m_midishader = loadMidiShader();
}

void AppGraphics::mainLoop(Platform::Win32::PlatformContext *const context)
{
	if (data->state != AppState::RUNNING) {
		CloseWindow(context->hwnd);
		return;
	}

	updateCountdown();
	updateMidi();
	updateKeys();

	neon->update();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void AppGraphics::initPiano()
{
	float x = PIANO_BEGIN_X;

	for (Note n = STARTING_NOTE; n <= ENDING_NOTE; n = Note::fromMidi(n.toMidi() + 1)) {
		const bool is_sharp = n.isSharp();

		auto obj = createKeyObject(n, x);
		m_piano_keys[n] = obj;
		m_piano_scene->addLocalNode(obj);

		if (!is_sharp)
			x += m_maxkeywidth;
	}
}

void AppGraphics::initCountdown()
{
	using namespace Neon;

	auto font = ResourceLoader::loadBitmapFont("calibri1250", "data/calibri1250.fnt");

	m_countdown_manager = BitmapTextManagerComponent::create("countdown_manager", font);
	m_countdown_manager->addCharacter('-');
	m_countdown_manager->addCharacter('.');
	m_countdown_manager->addCharacter('.');
	m_countdown_manager->addCharacter('.');
	m_countdown_manager->updateMesh();

	m_countdown_render = EngineObject::create("countdown");
	auto textRender = m_countdown_manager->getRenderComponent();
	auto textShader = Neon::ResourceLoader::loadTextShaders();
	auto textSprite = font->getPageSprite(0);

	m_countdown_render->addLocalNode(textRender);
	m_countdown_render->addLocalNode(textShader);
	m_countdown_render->addLocalNode(textSprite);

	textShader->use();
	textShader->getNodeByPath<UniformComponent>("$u_sampler")->upload(textSprite->slot);
	textShader->getNodeByPath<UniformComponent>("$u_color")->upload(Calcda::Vector3::One);
	textShader->getNodeByPath<UniformComponent>("$u_bordercolor")->upload(Calcda::Vector3::Zero);
	textShader->getNodeByPath<UniformComponent>("$u_charwidth")->upload(0.5f);
	textShader->getNodeByPath<UniformComponent>("$u_charedge")->upload(0.2f);
	textShader->getNodeByPath<UniformComponent>("$u_borderwidth")->upload(0.5f);
	textShader->getNodeByPath<UniformComponent>("$u_borderedge")->upload(0.2f);

	textShader->getNodeByPath<UniformComponent>("$u_mat")->upload((Calcda::Matrix4::orthographic(0, 800, 0, 600, -1, 1) * Calcda::Matrix4::translation(Calcda::Vector3(Calcda::Vector2(400, 300) - (m_countdown_manager->getSize() / 2.0), 0.0f))).transpose());

	auto assoc = m_countdown_render->createAssociation("countdownAssoc");
	assoc->addSprite(textSprite);
	assoc->setRender(textRender);
	assoc->setShader(textShader);
	assoc->renderPass = 0;

	m_countdown_render->visible = false;
	m_piano_scene->addLocalNode(m_countdown_render);
}

void AppGraphics::updateCountdown()
{
	if (m_countdown_data.active) {
		const auto now = clock::now();
		const auto value = (m_countdown_begin - std::chrono::duration_cast<std::chrono::seconds>(now - m_countdown_data.begin).count());

		if (value != m_countdown_data.value) {
			if (value <= 0) {
				m_countdown_data.active = false;
				m_countdown_render->visible = false;
			}
			else {
				m_countdown_data.value = value;
				m_countdown_manager->remove(0);
				m_countdown_manager->insertCharacter(0, value + '0');
				m_countdown_manager->updateMesh();
			}
		}
	}
}

void AppGraphics::updateMidi()
{
	if (m_midi_data.active) {
		const auto now = clock::now();
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_midi_data.playing_started).count() / 1000.0f;
		int i = 0;

		while (i < m_midi_note_pairs.size()) {
			auto &note_pair = m_midi_note_pairs[i];

			const auto valid = note_pair.second != nullptr;
			const auto visible = (note_pair.first.begin - elapsed) < 8.0f && (note_pair.first.begin + note_pair.first.duration - elapsed) > 0.0f;

			if (visible) {
				if (!valid) {
					note_pair.second = createMidiObject(note_pair.first.n, note_pair.first.begin, note_pair.first.duration);
					m_piano_scene->addLocalNode(note_pair.second);
				}

				const float offset = note_pair.first.begin - elapsed;

				note_pair.second->getNodeByPath<Neon::UniformStorageComponent>("su_offset")->set(offset);
			}

			if (!visible && valid) {
				m_piano_scene->removeLocalNode(note_pair.second);
				note_pair.second = nullptr;
				m_midi_note_pairs.erase(m_midi_note_pairs.begin() + i);

				if (m_midi_note_pairs.empty())
					m_midi_data.active = false;
			}
			else {
				i++;
			}
		}
	}
}

void AppGraphics::updateKeys()
{
	const auto localSounds = data->sounds.toNotes();

	for (Note n = STARTING_NOTE; n <= ENDING_NOTE; n = Note::fromMidi(n.toMidi() + 1)) {
		const auto color =
		    localSounds.count(n) != 0
		        ? ACTIVE_NOTE_COLOR
		        : (n.isSharp()
		               ? SHARP_NOTE_COLOR
		               : NOTE_COLOR);

		m_piano_keys[n]->getNodeByPath<Neon::UniformStorageComponent>("su_color")->set(color);
	}
}

AppGraphics::AppGraphics()
{
	m_countdown_data.active = false;
	m_midi_data.active = false;
}

bool AppGraphics::begin(const char *window_title, unsigned w, unsigned h, unsigned countdown_begin, float yscale, AppData *app_data)
{
	m_countdown_begin = countdown_begin;
	m_yscale = yscale;

	data = app_data;

	m_platform_context.userData = this;
	m_platform_context.onClick = [](void *contextPtr, unsigned x, unsigned y, Platform::ClickType t, Platform::ClickDirection d) -> void {
		/* printf("Click: x=%d y=%d t=%d d=%d\n", x, y, (int)t, (int)d);
		 */
		auto *const context = reinterpret_cast<Platform::Win32::PlatformContext *const>(contextPtr);

		reinterpret_cast<decltype(this)>(context->userData)->onClick(x, y, t, d);
	};

	if (m_platform_context.createGL3Window(window_title, w, h) != 0)
		return false;

	neon = Neon::Engine::create();

	m_maxkeywidth = calculateNoteMaxWidth();
	m_resolution = {800, 600};

	initShaders();

	initGraphics();
	initPiano();
	initCountdown();

	m_platform_context.registerLoopFunction([](void *contextPtr) -> void {
		auto *const context = reinterpret_cast<Platform::Win32::PlatformContext *const>(contextPtr);

		reinterpret_cast<decltype(this)>(context->userData)->mainLoop(context);
	});

	return true;
}

void AppGraphics::beginCountdown()
{
	if (m_countdown_data.active || m_midi_data.active)
		return;

	const auto now = clock::now();

	if (m_countdown_begin > 0) {
		m_countdown_data.active = true;
		m_countdown_data.begin = now;
		m_countdown_data.value = m_countdown_begin + 1;

		m_countdown_render->visible = true;
	}

	m_midi_data.active = true;
	m_midi_data.playing_started = now + std::chrono::seconds(m_countdown_begin);
}

void AppGraphics::setNotes(const std::vector<MidiNote> &notes)
{
	m_midi_note_pairs.resize(notes.size());

	int n = 0;
	std::transform(
	    notes.begin(), notes.end(), m_midi_note_pairs.begin(),
	    [n, this](const MidiNote &info) {
		    return std::make_pair(info, nullptr);
	    });
}

void AppGraphics::loop()
{
	m_platform_context.mainLoop();
}

void AppGraphics::end()
{
	m_countdown_manager = nullptr;
	m_countdown_render = nullptr;
	m_midishader = nullptr;
	m_keyshader = nullptr;

	m_countdown_data.active = false;

	m_piano_keys.clear();
	m_piano_scene = nullptr;
	neon = nullptr;
}
