#include <pch.h>
#include <engine/core/Application.h>
#include <engine/utils/logging.h>

#include <layers/GameplayLayer.h>

int main(int argc, char** argv) {
	sc::core::Application app(sc::core::WindowData{ "very cool RPG", 1280, 768 });
	app.PushLayer(std::make_shared<game::GameplayLayer>());
	app.Run();
}
