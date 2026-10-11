#pragma once
#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

namespace sc::input {
    using ActionId = int32_t;

    class InputContext {
        using Key = sf::Keyboard::Key;
        using MouseButton = sf::Mouse::Button;
		using IsKey = bool(*)(Key);
		using IsMouse = bool(*)(MouseButton);

    public:
        // bind a mouse/key to an action id, multiple keys can bind to same action
        void Bind(Key key, ActionId action);
        void Bind(MouseButton button, ActionId action);

        // same as above for a game's own action enum, so call sites don't cast

        template <typename Action> requires std::is_enum_v<Action>
        void Bind(Key key, Action action) { Bind(key, static_cast<ActionId>(action)); }

        template <typename Action> requires std::is_enum_v<Action>
        void Bind(MouseButton button, Action action) { Bind(button, static_cast<ActionId>(action)); }

        // true if any mouse/key is bound to this action
        bool IsBound(ActionId action) const;

        bool IsActionDown(ActionId action) const;
        bool IsActionJustPressed(ActionId action) const;
        bool IsActionJustReleased(ActionId action) const;

    private:
        bool IsAction(ActionId action, IsKey isKeyFn, IsMouse isMouseFn) const;

    private:
        std::unordered_map<Key, ActionId> m_keyBindings;
        std::unordered_map<MouseButton, ActionId> m_mouseBindings;
    };
}
