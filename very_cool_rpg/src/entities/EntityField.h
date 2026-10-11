#pragma once
#include <cstdint>
#include <variant>
#include <string>
#include <unordered_map>

#include <SFML/System/Vector2.hpp>

#include <types/LevelLocation.h>

namespace game::entities {
    using FieldValue = std::variant<int, float, bool, sf::Vector2f, std::string, LevelLocation>;

    struct FieldBag {
		template <typename T>
        const T* TryGet(const std::string& name) const {
            auto it = fields.find(name);
            if (it == fields.end()) {
                return nullptr;
            }
            return std::get_if<T>(&it->second);
        }

		std::unordered_map<std::string, FieldValue> fields;
    };
}
