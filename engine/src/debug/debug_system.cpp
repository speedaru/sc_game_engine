#include <pch.h>
#include <engine/debug/debug_system.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <cstring>

#include <imgui.h>

#include <engine/debug/DebugDraw.h>
#include <engine/utils/logging.h>

namespace sc::debug {
	namespace {
		struct ModuleEntry {
			std::unique_ptr<IDebugModule> module;
			bool enabled = false;
		};

		std::vector<ModuleEntry> s_modules;
		DebugContext s_context;
		bool s_overlayVisible = false;

		// unique categories in first registered order, so the tab bar doesn't reshuffle
		// itself between frames
		std::vector<const char*> CollectCategories() {
			std::vector<const char*> categories;

			for (const auto& entry : s_modules) {
				const char* category = entry.module->Category();

				bool seen = false;
				for (const char* known : categories) {
					if (std::strcmp(known, category) == 0) {
						seen = true;
						break;
					}
				}

				if (!seen) categories.push_back(category);
			}

			return categories;
		}
	}

	void RegisterModule(std::unique_ptr<IDebugModule> module) {
		if (!module) {
			LOG_W("tried to register a null debug module");
			return;
		}

		// imgui keys windows by title, so two modules sharing a name would collapse
		// into one window and fight over it
		for (const auto& entry : s_modules) {
			if (std::strcmp(entry.module->Name(), module->Name()) == 0) {
				LOG_W("debug module '%s' is already registered, ignoring the duplicate", module->Name());
				return;
			}
		}

		LOG_D("registered debug module '%s' in category '%s'", module->Name(), module->Category());
		s_modules.push_back(ModuleEntry{ std::move(module), false });
	}

	void Shutdown() {
		s_modules.clear();
		s_context = DebugContext{};
		s_overlayVisible = false;
	}

	void SetContext(const DebugContext& ctx) { s_context = ctx; }
	const DebugContext& GetContext() { return s_context; }

	bool IsOverlayVisible() { return s_overlayVisible; }
	void SetOverlayVisible(bool visible) { s_overlayVisible = visible; }
	void ToggleOverlay() { s_overlayVisible = !s_overlayVisible; }

	void DrawUI() {
		if (!s_overlayVisible) return;

		const bool contextReady = s_context.IsValid();

		if (ImGui::Begin("Debug", &s_overlayVisible)) {
			if (!contextReady) {
				ImGui::TextColored(ImVec4(1.f, 0.6f, 0.2f, 1.f), "no debug context this frame");
				ImGui::TextUnformatted("the game has to call debug::SetContext() every frame");
				ImGui::Separator();
			}

			if (s_modules.empty()) {
				ImGui::TextUnformatted("no debug modules registered");
			}
			else if (ImGui::BeginTabBar("##categories")) {
				for (const char* category : CollectCategories()) {
					if (!ImGui::BeginTabItem(category)) continue;

					for (auto& entry : s_modules) {
						if (std::strcmp(entry.module->Category(), category) != 0) continue;

						ImGui::Checkbox(entry.module->Name(), &entry.enabled);

						// inline modules render their options under their own checkbox
						if (contextReady && entry.enabled && !entry.module->WantsOwnWindow()) {
							ImGui::Indent();
							ImGui::PushID(entry.module->Name());
							entry.module->OnDrawUI(s_context);
							ImGui::PopID();
							ImGui::Unindent();
						}
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();

		if (!contextReady) return;

		// modules that asked for their own window get one, closable from its title bar
		for (auto& entry : s_modules) {
			if (!entry.enabled || !entry.module->WantsOwnWindow()) continue;

			if (ImGui::Begin(entry.module->Name(), &entry.enabled)) {
				entry.module->OnDrawUI(s_context);
			}
			ImGui::End();
		}
	}

	void DrawOverlay(DebugDraw& draw) {
		if (!s_context.IsValid()) return;

		// deliberately not gated on s_overlayVisible: hiding the gui shouldn't wipe out
		// the visualizations you turned on
		for (auto& entry : s_modules) {
			if (!entry.enabled) continue;

			entry.module->OnDrawOverlay(s_context, draw);
		}
	}
}

#endif
