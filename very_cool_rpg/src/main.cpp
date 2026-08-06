#include <pch.h>
#include <engine/core/Application.h>
#include <engine/utils/logging.h>
#include <engine/debug/debug_system.h>
#include <engine/debug/modules/FrameStats.h>
#include <engine/debug/modules/CollisionOverlay.h>

#include <layers/GameplayLayer.h>
#include <constants.h>

using Application = sc::core::Application;
using WindowData = sc::core::WindowData;
using namespace game;
namespace dbg = sc::debug;

// body only compiles in debug builds: the module types don't exist in release
void RegisterDebugModules() {
	SC_DEBUG_ONLY(dbg::RegisterModule(std::make_unique<dbg::modules::FrameStats>()));
	SC_DEBUG_ONLY(dbg::RegisterModule(std::make_unique<dbg::modules::CollisionOverlay>()));
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
