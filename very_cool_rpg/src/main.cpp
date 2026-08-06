#include <pch.h>
#include <engine/core/Application.h>
#include <engine/utils/logging.h>
#include <engine/debug/debug_system.h>
#include <engine/debug/modules/FrameStats.h>

#include <layers/GameplayLayer.h>
#include <constants.h>

using Application = sc::core::Application;
using WindowData = sc::core::WindowData;
using namespace game;
namespace dbg = sc::debug;

void RegisterDebugModules() {
	// the body is wrapped, not the function: in release the module types don't exist,
	// so naming FrameStats here at all wouldn't compile
	dbg::RegisterModule(std::make_unique<dbg::modules::FrameStats>());
}

void PushGameLayers(Application& app) {
	app.PushLayer(std::make_shared<GameplayLayer>());
}

int main(int argc, char** argv) {
	Application app(WindowData{ "very cool RPG", WINDOW_WIDTH, WINDOW_HEIGHT });
	SC_DEBUG_ONLY(RegisterDebugModules());
	PushGameLayers(app);
	app.Run();
}
