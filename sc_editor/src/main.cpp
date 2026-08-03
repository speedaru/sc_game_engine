#include <optional>

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include "TileEditorLayer.h"

int main(int argc, char** argv) {
	sf::RenderWindow window(sf::VideoMode({ 1280u, 800u }), "LDtk Collision Editor");
	window.setFramerateLimit(60);

	if (!ImGui::SFML::Init(window)) {
		return -1;
	}
	
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = "editor.ini";

	sc_editor::TileEditorLayer editor;
	if (argc > 1) {
		editor.LoadProject(argv[1]);
	}

	sf::Clock deltaClock;
	while (window.isOpen()) {
		while (const std::optional<sf::Event> event = window.pollEvent()) {
			ImGui::SFML::ProcessEvent(window, *event);
			if (event->is<sf::Event::Closed>()) {
				window.close();
			}
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		editor.OnImGuiRender();

		window.clear();
		ImGui::SFML::Render(window);
		window.display();
	}

	ImGui::SFML::Shutdown();
	return 0;
}
