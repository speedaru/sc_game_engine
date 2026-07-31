#pragma once
#include <cstdint>
#include <unordered_map>

#include <SFML/Window/Keyboard.hpp>

namespace sc::input {
    using ActionId = int32_t;

    class InputContext {
    public:
        // bind a key to an action ID
        void Bind(sf::Keyboard::Key key, ActionId action);

        bool IsActionPressed(ActionId action) const;

    private:
        std::unordered_map<sf::Keyboard::Key, ActionId> m_keyBindings;
    };
}
