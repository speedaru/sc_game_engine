#pragma once
#include <string>
#include <vector>
#include <memory>

#include <SFML/System/Vector2.hpp>

#include <engine/world/ILevelLayer.h>

namespace sc::world {
	struct LevelId {
		const std::string name;
		int32_t uid;
	};

	// stores layers in bottom to top order
	class Level {
	public:
		Level(const LevelId& id, sf::Vector2i size) : m_id(id), m_size(size) {}

		// adds layers in bottom to top
		void AddLayer(std::unique_ptr<ILevelLayer> layer);

		const LevelId& GetId() const { return m_id; }
		sf::Vector2i GetSize() const { return m_size; }

		// expose layer stack for renderer
		const std::vector<std::unique_ptr<ILevelLayer>>& GetLayers() const { return m_layers; }

	private:
		LevelId m_id;
		sf::Vector2i m_size;
		std::vector<std::unique_ptr<ILevelLayer>> m_layers;
	};
}
