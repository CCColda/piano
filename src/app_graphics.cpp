#include "app_graphics.h"

#include <chrono>
#include <thread>

#include <neonComponent.h>
#include <neonEngine.h>
#include <neonResourceLoader.h>

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
	neon->options.backgroundColor = {0.8f, 0.1f, 0.1f, 1.0f};
	neon->options.clear = true;

	neon->onSwapBuffers = [this]() -> void { SwapBuffers(this->m_platform_context.hdc); };
	neon->applyOptions();

	piano_scene = Node::create("piano_scene");
	neon->scenes->addLocalNode(piano_scene);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void AppGraphics::mainLoop(Platform::Win32::PlatformContext *const context)
{
	if (data->state != AppState::RUNNING) {
		CloseWindow(context->hwnd);
		return;
	}

	neon->update();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void AppGraphics::initPiano()
{
	using namespace Neon;

	static const float mesh[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 1.0f};
	static const unsigned idx[] = {0, 1, 2, 1, 2, 3};

	//! @todo
	auto obj = EngineObject::create("engineobj_sprite");

	obj->addLocalNode(RenderComponent::create("sprite_render"));
	obj->addNode("sprite_render",
	             MeshComponent::create(
	                 "mesh1",
	                 Buffer::Static((void *)mesh, sizeof(mesh)),
	                 MeshComponent::LayoutList{{0, 2, GL_FLOAT, GL_FALSE, 0}, {1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2}},
	                 Buffer::Static((void *)idx, sizeof(idx))));

	obj->addLocalNode(ResourceLoader::loadSprite("texture", 0, "data/grid.png"));

	obj->addLocalNode(ShaderComponent::create("sprite_shader"));
	obj->addNode("sprite_shader", ResourceLoader::loadShaderStage("vertex", GL_VERTEX_SHADER, "data/texture.gles20.vertex.glsl"));
	obj->addNode("sprite_shader", ResourceLoader::loadShaderStage("fragment", GL_FRAGMENT_SHADER, "data/texture.gles20.fragment.glsl"));
	obj->getNodeByPath<ShaderComponent>("sprite_shader")->link();
	obj->getNodeByPath<ShaderComponent>("sprite_shader")->bindAttribLocation(0, "i_pos");
	obj->getNodeByPath<ShaderComponent>("sprite_shader")->bindAttribLocation(1, "i_txc");

	obj->addLocalNode(ShaderComponent::create("sprite_bloom_shader"));
	obj->addNode("sprite_bloom_shader", ResourceLoader::loadShaderStage("vertex", GL_VERTEX_SHADER, "data/texture.gles20.vertex.glsl"));
	obj->addNode("sprite_bloom_shader", ResourceLoader::loadShaderStage("fragment", GL_FRAGMENT_SHADER, "data/texture.bloom.gles20.fragment.glsl"));
	obj->getNodeByPath<ShaderComponent>("sprite_bloom_shader")->link();
	obj->getNodeByPath<ShaderComponent>("sprite_bloom_shader")->bindAttribLocation(0, "i_pos");
	obj->getNodeByPath<ShaderComponent>("sprite_bloom_shader")->bindAttribLocation(1, "i_txc");

	obj->getNodeByPath<ShaderComponent>("sprite_shader")->use();
	obj->addNode("sprite_shader", UniformComponent::create("u_mat"));
	obj->getNodeByPath<UniformComponent>("sprite_shader/$u_mat")->upload(Calcda::Matrix4::Identity);

	obj->getNodeByPath<ShaderComponent>("sprite_bloom_shader")->use();
	obj->addNode("sprite_bloom_shader", UniformComponent::create("u_mat"));
	obj->addNode("sprite_bloom_shader", UniformComponent::create("u_bloom"));
	obj->getNodeByPath<UniformComponent>("sprite_bloom_shader/$u_mat")->upload(Calcda::Matrix4::Identity);
	obj->getNodeByPath<UniformComponent>("sprite_bloom_shader/$u_bloom")->upload(0.9f);

	auto normalAssoc = obj->createAssociation("assoc_normal");
	normalAssoc->setRender("sprite_render");
	normalAssoc->setShader("sprite_shader");
	normalAssoc->addSprite("texture");
	normalAssoc->renderPass = 0;

	auto bloomAssoc = obj->createAssociation("assoc_bloom");
	bloomAssoc->setRender("sprite_render");
	bloomAssoc->setShader("sprite_bloom_shader");
	bloomAssoc->addSprite("texture");
	bloomAssoc->renderPass = 1;

	obj->visible = true;

	piano_scene->addLocalNode(obj);
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
