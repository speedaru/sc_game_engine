#include <pch.h>
#include <engine/debug/modules/FrameStats.h>

#if SC_ENABLE_DEBUG_TOOLS
#include <cfloat>

#include <imgui.h>

namespace sc::debug::modules {
	void FrameStats::OnDrawUI(const DebugContext& ctx) {
		const float frameMs = ctx.deltaTime * 1000.f;

		m_frameMs[m_head] = frameMs;
		m_head = (m_head + 1) % HISTORY;

		float total = 0.f;
		float worst = 0.f;
		for (float sample : m_frameMs) {
			total += sample;
			worst = std::max(worst, sample);
		}
		const float average = total / static_cast<float>(HISTORY);

		ImGui::Text("frame   %6.2f ms  (%.0f fps)", frameMs, frameMs > 0.f ? 1000.f / frameMs : 0.f);
		ImGui::Text("average %6.2f ms  over %zu frames", average, HISTORY);
		ImGui::Text("worst   %6.2f ms", worst);

		ImGui::Checkbox("Graph", &m_showGraph);
		if (m_showGraph) {
			// FLT_MAX for the bounds tells imgui to autoscale to the data
			ImGui::PlotLines(
				"##frametime",
				m_frameMs.data(),
				static_cast<int>(HISTORY),
				static_cast<int>(m_head),
				nullptr,
				0.f,
				FLT_MAX,
				ImVec2(0.f, 60.f)
			);
		}
	}
}

#endif
