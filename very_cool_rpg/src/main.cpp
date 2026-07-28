#include <pch.h>
#include <engine/core/Application.h>
#include <engine/utils/logging.h>

#include <layers/GameplayLayer.h>
#include <constants.h>

using namespace game;

int main(int argc, char** argv) {
	sc::core::Application app(sc::core::WindowData{ "very cool RPG", WINDOW_WIDTH, WINDOW_HEIGHT });
	app.PushLayer(std::make_shared<GameplayLayer>());
	app.Run();
}
