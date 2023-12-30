#include "app.h"

#include <Logger.h>

#include "serial_parser.h"
#include "windows_serial.h"

#include <chrono>

#include <neonComponent.h>
#include <neonEngine.h>
#include <neonResourceLoader.h>

#include "app_audio_thread.h"
#include "app_serial_thread.h"

namespace {
struct PortValidator : public CLI::Validator {
	PortValidator()
	{
		name_ = "PORT";
		func_ = [](const std::string &str) {
			const auto com_part = str.substr(0, 3);
			const auto number_part = str.substr(3);

			const auto number_part_valid = std::transform_reduce(
			    number_part.begin(), number_part.end(),
			    true, std::logical_and{},
			    [](const char &c) { return (bool)std::isdigit(c); });

			return CLI::detail::to_lower(com_part) == "com" && number_part_valid
			           ? std::string()
			           : "Invalid COM port.";
		};
	}
};

bool init_graphics(Platform::Win32::PlatformContext *context)
{
	using namespace Neon;

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE0);
	glEnable(GL_TEXTURE1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	auto *const userData = reinterpret_cast<AppData *const>(context->userData);

	static const float mesh[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 1.0f};
	static const unsigned idx[] = {0, 1, 2, 1, 2, 3};

	userData->neon->options.outDim = {800.0f, 600.0f};
	userData->neon->options.postprocDim = {800.0f, 600.0f};
	userData->neon->options.backgroundColor = {0.8f, 0.1f, 0.1f, 1.0f};
	userData->neon->options.clear = true;

	userData->neon->onSwapBuffers = [context]() -> void { SwapBuffers(context->hdc); };

	userData->neon->applyOptions();

	auto scene = Node::create("scene1");

	userData->neon->scenes->addLocalNode(scene);

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

	scene->addLocalNode(obj);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	return true;
}

void main_loop(void *context)
{
	auto *platformContext = reinterpret_cast<Platform::Win32::PlatformContext *>(context);
	auto *userData = reinterpret_cast<AppData *>(platformContext->userData);

	if (userData->state != AppState::RUNNING) {
		CloseWindow(platformContext->hwnd);
		return;
	}

	userData->neon->update();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

} // namespace

//! @todo strings
PianoApp::PianoApp() : commandLine("Piano app"), data()
{
	data.state = AppState::SETUP;

	arguments.port = "COM8";
	arguments.baud = 115200;
	arguments.serialSettings = Serial::ARDUINO_SETTINGS;
	arguments.volume = 0.3f;

	m_platform_context.userData = &data;
	m_platform_context.onClick = [](void *context, unsigned x, unsigned y, Platform::ClickType t, Platform::ClickDirection d) -> void {
		printf("Click: x=%d y=%d t=%d d=%d\n", x, y, (int)t, (int)d);
	};

	Logger::console = Logger::openStaticOutputStream(std::cout);
	Logger::logLevel = Logger::Level::LVL_VERBOSE;
}

bool PianoApp::initCommandLine(int argc, const char *argv[])
{
	commandLine.add_option("-p,--port", arguments.port, "The serial port to connect to")
	    ->check(PortValidator());

	commandLine.add_option("-b,--baud", arguments.baud, "The baud rate of the serial connection");

	commandLine.add_option("-s,--stop_bits,--stop", arguments.serialSettings.stop_bits, "The number of stop bits")
	    ->transform(CLI::CheckedTransformer(Serial::STOPBIT_MAP, CLI::ignore_case));

	commandLine.add_option("--par,--parity", arguments.serialSettings.parity, "The parity bit. N for none, E for even, O for odd")
	    ->transform(CLI::CheckedTransformer(Serial::PARITY_MAP, CLI::ignore_case));

	commandLine.add_option("--bs,--byte_size", arguments.serialSettings.byte_size, "The number of bits");
	commandLine.add_option("--volume,-v", arguments.volume, "The volume in the range [0-1]");

	try {
		commandLine.parse(argc, argv);
	}
	catch (const CLI::ParseError &exc) {
		(void)commandLine.exit(exc);
		return false;
	}

	data.state = AppState::RUNNING;

	return true;
}

bool PianoApp::initAudio()
{
	m_openal_thread_handle = std::thread(openal_thread, &data, arguments.volume);

	std::unique_lock lock(data.condition_variables.al_done_mutex);
	data.condition_variables.al_done.wait_for(lock, std::chrono::seconds(5));

	return data.state == AppState::RUNNING;
}

bool PianoApp::initGraphics()
{
	//! @todo strings
	if (m_platform_context.createGL3Window("Piano", 800, 600) != 0) {
		return false;
	}

	data.neon = Neon::Engine::create();
	(void)init_graphics(&m_platform_context);

	m_platform_context.registerLoopFunction(main_loop);

	return true;
}

bool PianoApp::initSerial()
{
	m_serial_thread_handle = std::thread(serial_thread, arguments, &data);

	std::unique_lock lock(data.condition_variables.serial_done_mutex);
	data.condition_variables.serial_done.wait_for(lock, std::chrono::seconds(5));

	return data.state == AppState::RUNNING;
}

void PianoApp::mainLoop()
{
	m_platform_context.mainLoop();
}

void PianoApp::cleanup()
{
	data.state = AppState::FINISHED;

	m_serial_thread_handle.join();
	m_openal_thread_handle.join();

	data.neon = nullptr;
}