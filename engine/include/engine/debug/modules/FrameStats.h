#pragma once
#include <engine/debug/debug_config.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <array>
#include <cstddef>

#include <engine/debug/IDebugModule.h>

namespace sc::debug::modules {
	// frame timing. reads nothing from the context but deltaTime, which makes it the
	// smallest complete worked example of a module - copy this file's shape when adding
	// a new one.
	//
	// note the history only advances while OnDrawUI runs, i.e. while this module is
	// enabled and the master window is open. that's fine for eyeballing frame cost;
	// it is not a profiler
	class FrameStats : public IDebugModule {
	public:
		const char* Name() const override { return "Frame Stats"; }
		const char* Category() const override { return "Engine"; }
		bool WantsOwnWindow() const override { return false; }

		void OnDrawUI(const DebugContext& ctx) override;

	private:
		static constexpr std::size_t HISTORY = 120;

		// ring buffer of frame times in milliseconds
		std::array<float, HISTORY> m_frameMs{};
		std::size_t m_head = 0;

		bool m_showGraph = true;
	};
}

#endif
