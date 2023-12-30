#include "app_graphics.h"

#include <chrono>
#include <sstream>
#include <thread>

#include <neonComponent.h>
#include <neonEngine.h>
#include <neonResourceLoader.h>

Neon::EngineObjectPtr createNoteObject(unsigned midi, bool is_sharp, float x, float w, float base_y, float max_y, Neon::ShaderComponentPtr shader)
{
	using namespace Neon;
	using namespace cppx;

	const float offset_x = x + (is_sharp ? -w * 0.5f : 0.0f);
	const float extent_y = base_y + (max_y - base_y) * (is_sharp ? 0.6f : 1.0f);

	const float mesh[] = {
	    offset_x, base_y,
	    offset_x + w, base_y,
	    offset_x + w, extent_y,
	    offset_x, extent_y};

	static const unsigned idx[] = {0, 1, 2, 2, 3, 0};

	auto obj = EngineObject::create("note_" + std::to_string(midi));

	obj->addLocalNode(RenderComponent::create("note_render"));
	obj->addNode("note_render",
	             MeshComponent::create(
	                 "note_mesh",
	                 Buffer::Static((void *)mesh, sizeof(mesh)),
	                 MeshComponent::LayoutList{{0, 2, GL_FLOAT, GL_FALSE, 0}},
	                 Buffer::Static((void *)idx, sizeof(idx))));

	obj->addLocalNode(shader);

	auto su_mat = Neon::UniformStorageComponent::create("su_mat", shader->getNodeByPath<Neon::UniformComponent>("$u_mat"));
	su_mat->set(Calcda::Matrix4::orthographic(0.0, 1.0, 0.0, 600.0, -1.0, 1.0).transpose());
	obj->addLocalNode(su_mat);

	auto su_color = Neon::UniformStorageComponent::create("su_color", shader->getNodeByPath<Neon::UniformComponent>("$u_color"));
	su_color->set(is_sharp ? Calcda::Vector4(0.0, 0.0, 0.0, 1.0) : Calcda::Vector4::One);
	obj->addLocalNode(su_color);

	auto normalAssoc = obj->createAssociation("assoc_normal");
	normalAssoc->setRender("note_render");
	normalAssoc->setShader(shader);
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

	piano_scene = Node::create("piano_scene");
	neon->scenes->addLocalNode(piano_scene);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void AppGraphics::mainLoop(Platform::Win32::PlatformContext *const context)
{
	if (data->state != AppState::RUNNING) {
		CloseWindow(context->hwnd);
		return;
	}

	const auto localSounds = data->sounds.toNotes();

	for (Note n = STARTING_NOTE; n <= ENDING_NOTE; n = Note::fromMidi(n.toMidi() + 1)) {
		notes[n]->getNodeByPath<Neon::UniformStorageComponent>("su_color")->set(localSounds.count(n) != 0 ? Calcda::Vector4(0.0, 1.0, 0.0, 1.0) : (n.isSharp() ? Calcda::Vector4(0.0, 0.0, 0.0, 1.0) : Calcda::Vector4::One));
	}

	neon->update();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void AppGraphics::initPiano()
{
	using namespace Neon;

	auto shader = ShaderComponent::create("note_shader");
	shader->addLocalNode(ResourceLoader::loadShaderStage("vertex", GL_VERTEX_SHADER, "data/note.vertex.glsl"));
	shader->addLocalNode(ResourceLoader::loadShaderStage("fragment", GL_FRAGMENT_SHADER, "data/note.fragment.glsl"));
	shader->link();
	shader->bindAttribLocation(0, "i_pos");
	shader->addLocalNode(UniformComponent::create("u_color"));
	shader->addLocalNode(UniformComponent::create("u_mat"));

	float x = (1.0f - PIANO_WIDTH_MULTIPLIER) * 0.5f;

	unsigned num_full_keys = 0;
	for (Note n = STARTING_NOTE; n <= ENDING_NOTE; n = Note::fromMidi(n.toMidi() + 1)) {
		if (!n.isSharp())
			num_full_keys++;
	}

	const float maxwidth = (PIANO_WIDTH_MULTIPLIER / float(num_full_keys));

	for (Note n = STARTING_NOTE; n <= ENDING_NOTE; n = Note::fromMidi(n.toMidi() + 1)) {
		const bool is_sharp = n.isSharp();

		auto obj = createNoteObject(n.toMidi(), is_sharp, x, maxwidth * NOTE_WIDTH_MULTIPLIER, 500, 600, shader);
		notes[n] = obj;
		piano_scene->addLocalNode(obj);

		if (!is_sharp)
			x += maxwidth;
	}
}

bool AppGraphics::begin(const char *window_title, unsigned w, unsigned h, AppData *app_data)
{
	data = app_data;

	m_platform_context.userData = this;
	m_platform_context.onClick = [](void *context, unsigned x, unsigned y, Platform::ClickType t, Platform::ClickDirection d) -> void {
		printf("Click: x=%d y=%d t=%d d=%d\n", x, y, (int)t, (int)d);
	};

	if (m_platform_context.createGL3Window(window_title, w, h) != 0)
		return false;

	neon = Neon::Engine::create();

	initGraphics();
	initPiano();

	m_platform_context.registerLoopFunction([](void *contextPtr) -> void {
		auto *const context = reinterpret_cast<Platform::Win32::PlatformContext *const>(contextPtr);

		reinterpret_cast<decltype(this)>(context->userData)->mainLoop(context);
	});

	return true;
}

void AppGraphics::loop()
{
	m_platform_context.mainLoop();
}

void AppGraphics::end()
{
	notes.clear();
	piano_scene = nullptr;
	neon = nullptr;
}
