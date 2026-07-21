#include <engine/core/Application.h>
#include <engine/utils/logging.h>

#include <game/layers/GameplayLayer.h>

int main(int argc, char** argv) {
	sc::core::Application app(sc::core::WindowData{ "very cool RPG", 1280, 720 });
	app.PushLayer(std::make_shared<GameplayLayer>());
	app.Run();
}
