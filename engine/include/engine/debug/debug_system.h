#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <memory>
#include <utility>

#include <engine/debug/DebugContext.h>
#include <engine/debug/IDebugModule.h>

namespace sc::debug {
	class DebugDraw;

	// ---- registration ------------------------------------------------------------

	// takes ownership. call these from one bootstrap function at startup.
	//
	// do NOT replace this with static initializer self registration. the engine is a
	// static library, and the linker drops object files that nothing references - a
	// self registering module would silently not exist, with no error to chase. one
	// explicit bootstrap is duller but it also gives you a single place to see every
	// module that exists and to control their order
	void RegisterModule(std::unique_ptr<IDebugModule> module);

	template <typename T, typename... Args>
	void Register(Args&&... args) {
		RegisterModule(std::make_unique<T>(std::forward<Args>(args)...));
	}

	// drops every registered module. called by DebugLayer::OnDetach
	void Shutdown();

	// ---- per frame state ---------------------------------------------------------

	// published once a frame by whoever owns the registry/level/camera. modules are
	// skipped entirely on any frame where this hasn't been set to something valid
	void SetContext(const DebugContext& ctx);
	const DebugContext& GetContext();

	// ---- master window -----------------------------------------------------------

	// toggles the imgui windows only. module overlays keep drawing, so you can hide the
	// gui to look at the game without losing the visualizations you enabled
	bool IsOverlayVisible();
	void SetOverlayVisible(bool visible);
	void ToggleOverlay();

	// ---- driven by DebugLayer, not meant for game code ----------------------------

	void DrawUI();
	void DrawOverlay(DebugDraw& draw);
}

#endif
