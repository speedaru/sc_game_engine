Read through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, wiRead through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, with a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translation units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.Read through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, with a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translatRead through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, with a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    Read through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, with a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translation units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.Read through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, with a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translation units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.Read through everything. Before the architecture, three findings that change the shape of this project — none of them are "the debug system" but all of them block it:

1. There is no event plumbing anywhere in the engine. Window::Update() (Window.cpp:13-22) polls events and throws every one away except Closed. ILayer has no OnEvent. ImGui-SFML is completely dead without ImGui::SFML::ProcessEvent(window, event) per event — no clicks, no typing, no window dragging. So "add an imgui overlay" starts with building event dispatch into the layer stack.

2. imgui isn't compiled into the engine or the game. Only sc_editor compiles imgui + imgui-sfml sources. The engine has the imgui include path but no imgui-sfml path and compiles neither. Good news: sc_editor doesn't link the engine (checked AdditionalDependencies), so pulling imgui into sc_game_engine.lib won't collide.

3. Input is polling-only, with a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translation units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translation units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.ion units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.th a stack that only reads the top. InputContext::IsActionPressed calls sf::Keyboard::isKeyPressed directly, and Input::IsActionActive only consults s_contexts.back(). Two consequences: a debug input context would silently kill all gameplay input, and there's no edge detection, so a polled toggle key fires every frame. Once events exist, sf::Event::KeyPressed gives you the edge for free — so the debug toggle should be an event handler, not an input action.

---
The core model: a module is a class, a file, and a checkbox

// engine/include/engine/debug/IDebugModule.h
namespace sc::debug {
    class IDebugModule {
    public:
        virtual ~IDebugModule() = default;

        virtual const char* Name() const = 0;
        virtual const char* Category() const { return "General"; }  // → which tab

        virtual void OnDrawUI(const DebugContext&) {}                    // imgui widgets
        virtual void OnDrawOverlay(const DebugContext&, DebugDraw&) {}   // world-space shapes
    };
}

That's the whole interface. A module implements one or both draw hooks. The registry owns an enabled flag per module and only calls hooks on enabled ones.

House style suggests free functions over a singleton class — render2d:: and input:: are both namespaces of free functions over file-static state, so sc::debug::Register(...), sc::debug::Toggle() will read as native.

Layout:
- engine/include/engine/debug/ + engine/src/debug/ — framework (IDebugModule, DebugDraw, DebugLayer, registry) and engine modules (CollisionOverlay, SpatialGridOverlay, EntityStats, FrameStats)
- very_cool_rpg/src/debug/ — game modules

The test of whether the API is right: engine modules must use the exact same public API the game does. If an engine module needs privileged access the game can't get, the design is wrong.

On "rules and subrules" — don't build the tree

Your instinct about hierarchy is right for the UI, but I'd push back on making it a code-level structure. A generic recursive IDebugNode with children sounds clean until you notice every feature's sub-options are heterogeneous: the collision module wants two bools, the spatial-grid module wants a bool plus a cell-highlight color plus an entity picker, the stats module wants a component filter. A generic tree forces all of that through a lowest-common-denominator interface and you spend your time fighting it.

Sub-options should just be plain members drawn by the module itself:

class SpatialGridOverlay : public IDebugModule {
    bool m_showCellBorders = true;
    bool m_showOccupancyCounts = false;
    bool m_showEntityToCellLinks = false;
    bool m_highlightNonEmptyOnly = true;

    void OnDrawUI(const DebugContext&) override {
        ImGui::Checkbox("Cell borders", &m_showCellBorders);
        ImGui::Checkbox("Occupancy counts", &m_showOccupancyCounts);
        ImGui::Checkbox("Entity → cell links", &m_showEntityToCellLinks);
        if (ImGui::TreeNode("Appearance")) { /* colors, alpha */ ImGui::TreePop(); }
    }
};

You get arbitrary nesting depth from ImGui::TreeNode/BeginTabBar inside the module, with zero framework. The only thing worth making generic is the master window — tabs by Category(), a checkbox per module, and each enabled module optionally getting its own ImGui::Begin window.

The two decisions that actually fork the implementation

A. What draws the world overlay

render2d can't be reused — it only does textured quads and tile layers, has no line/rect primitives, and asserts on a bound target inside BeginBatch/EndBatch. So this is new code either way. Two real options:

Option 1 — SFML vertex batching. Accumulate into sf::VertexArray (Lines + Triangles), flush in a couple of draw calls inside the camera's view. Native, interleaves with world rendering, respects the view automatically. Costs: you write the batching, and text needs an sf::Font asset + sf::Text (unbatchable, one draw call each).

Option 2 — imgui's background draw list. Project world→screen with window.mapCoordsToPixel(worldPos, camera.GetView()) and push into ImGui::GetBackgroundDrawList(). You get antialiased lines, filled shapes, arbitrary polys, and text with no font asset for free, all batched by imgui, and you write essentially no rendering code. Costs: always draws on top of the world (fine for debug), and you own the projection helper.

I'd recommend option 2 — it deletes an entire subsystem, and text matters more than you'd think for this (entity IDs, cell occupancy counts, coordinates).

The important part either way: DebugDraw is a facade taking world-space coordinates, so no module knows which backend is underneath:

void Rect(const sf::FloatRect&, sf::Color, float thickness = 1.f);
void FilledRect(const sf::FloatRect&, sf::Color);
void Line(sf::Vector2f a, sf::Vector2f b, sf::Color, float thickness = 1.f);
void Text(sf::Vector2f worldPos, sf::Color, const char* fmt, ...);

Start with imgui draw lists; if you later need overlays underneath sprites, swap the backend and no module changes.

B. How modules reach their data

The camera lives in GameplayLayer::m_camera and the level in m_currentLevel — a DebugLayer pushed after gameplay has access to neither. My recommendation is a hybrid split by lifetime:

- Stable-for-app-lifetime deps → constructor injection. debug::Register<CollisionOverlay>(m_registry), debug::Register<QuestDebug>(m_questSystem). Typed, no null checks, and game-specific modules take game-specific dependencies the engine never has to know about.
- Volatile deps → a tiny per-frame DebugContext. Just Level* level and float deltaTime for now, refreshed each frame by the game. This is what makes level switching not break every module holding a stale Level&.

The value of splitting is that the context struct stays small because it only holds things that genuinely change — as opposed to a fat god-struct that grows forever, or a type-erased service locator where every lookup can return null.

The workflow for adding a feature

1. New file pair in engine/src/debug/modules/ or very_cool_rpg/src/debug/
2. Inherit IDebugModule, implement Name(), Category(), and whichever draw hook(s) you need
3. Sub-options = plain members + ImGui::Checkbox/SliderFloat in OnDrawUI
4. One line in a single RegisterDebugModules() bootstrap file
5. Add to the .vcxproj

Do not try to eliminate step 4 with static-init self-registration. REGISTER_DEBUG_MODULE(Foo) at file scope is the classic trick, and in a static library the linker strips translation units nothing references — your modules silently don't exist, with no error. Working around it needs /WHOLEARCHIVE or per-module #pragma comment(linker, "/include:..."). The engine is ConfigurationType=StaticLibrary, so you will hit this. One explicit bootstrap file is boring, bulletproof, and gives you one place to see every module and control ordering.

Suggested build order

1. Event plumbing — poll loop moves into Application::Run, add ILayer::OnEvent(const sf::Event&) -> bool dispatched top-down (topmost layer gets first refusal). Prerequisite for everything, and useful well beyond debug.
2. imgui in the engine build — compile imgui + imgui-sfml into the engine lib, ImGui::SFML::Init/Update/Render/Shutdown in Application, add ILayer::OnImGuiRender() called after all OnRenders. Gate game input on ImGui::GetIO().WantCaptureKeyboard/WantCaptureMouse so typing in a debug field doesn't walk the player around.
3. Framework — IDebugModule, registry, DebugDraw, DebugLayer, master window with category tabs, F1 toggle via KeyPressed.
4. First module: CollisionOverlay — literally move the loop out of GameplayLayer::OnRender:94-104. Proves the whole pipeline against code you already trust.
5. SpatialGridOverlay — and note this one needs a small SpatialGrid API addition: m_grid and m_entityCells are private with no iteration accessors, so there's currently no way to ask "which entities are in cell (r,c)" or "which cells does entity X occupy." Adding read-only accessors is the honest fix (and doubles as the verification tool for the RegisterEntityCollisions bug you're deferring).
6. EntityStats / FrameStats — component-combo counts. Fully generic "filter by any component" needs entt::meta reflection, which is a real project on its own; hardcoded checkboxes for the combos you care about will cover 95% of it.

---
Three things I'd want your call on before writing any of it:

- Overlay backend — sold on imgui draw lists, or do you want SFML-native so overlays can interleave with world layers later?
- Scope of step 1 — the event system is genuinely useful engine work but it's not "debugging." Do you want it as its own focused pass first, or folded into this?
- Release builds — should the whole thing compile out behind SC_ENABLE_DEBUG_TOOLS, or do you want the overlay available in Release for playtesting? Affects whether registration call sites need macro wrapping from day one.