#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <engine/debug/DebugContext.h>

namespace sc::debug {
	class DebugDraw;

	// one self contained debug feature: one file, one checkbox in the master window.
	//
	// sub options are plain members the module draws itself in OnDrawUI. there is
	// deliberately no generic sub module tree here - every feature's options are a
	// different shape (a bool, a color, an entity picker, a filter), and forcing them
	// through a common node interface buys nothing that ImGui::TreeNode doesn't
	// already give you for free inside OnDrawUI
	class IDebugModule {
	public:
		virtual ~IDebugModule() = default;

		// checkbox label in the master window, and the title of this module's own
		// window. must be unique - imgui keys windows by title
		virtual const char* Name() const = 0;

		// modules are grouped into tabs by category
		virtual const char* Category() const { return "General"; }

		// true  -> enabling this module opens a dedicated imgui window for it
		// false -> its widgets render inline under its checkbox in the master window
		virtual bool WantsOwnWindow() const { return true; }

		// imgui widgets. only called while the module is enabled
		virtual void OnDrawUI(const DebugContext& ctx) {}

		// world space overlay drawing. only called while the module is enabled, but
		// unlike OnDrawUI it keeps running while the master window is hidden, so
		// hiding the gui doesn't wipe out the visualizations you turned on
		virtual void OnDrawOverlay(const DebugContext& ctx, DebugDraw& draw) {}
	};
}

#endif
