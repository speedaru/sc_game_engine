#pragma once
#include <cstdint>
#include <variant>
#include <string>
#include <unordered_map>

#include <SFML/System/Vector2.hpp>

namespace game::entities {
    struct EntityRefValue {
        int32_t levelUid;
        sf::Vector2f pos;
    };

    using FieldValue = std::variant<int, float, bool, sf::Vector2f, std::string, EntityRefValue>;

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
