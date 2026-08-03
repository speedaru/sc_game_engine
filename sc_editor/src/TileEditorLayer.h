#pragma once
#include <string>
#include <vector>

#include <SFML/Graphics/Texture.hpp>
#include <imgui.h>

#include "LDtkEditorData.h"

// Forward declared instead of pulling in imgui_internal.h here: only needed as a
// pointer type for the .ini settings handler callbacks, defined in the .cpp.
struct ImGuiSettingsHandler;

namespace sc_editor {

	enum class InteractionState {
		Idle,
		Drawing,
		Dragging,
		Resizing
	};

	// Which edge(s) of a hitbox the cursor is close enough to for a resize drag.
	enum class ResizeHandle {
		None,
		N, S, E, W,
		NE, NW, SE, SW
	};

	// Holds all the live state for the collision editor GUI: the loaded
	// project, which tileset/tile is being edited, and the in-progress
	// mouse interaction on the canvas.
	class TileEditorLayer {
	public:
		TileEditorLayer();

		bool LoadProject(const std::string& path);

		// Draws the whole "LDtk Collision Editor" window. Call once per frame.
		void OnImGuiRender();

	private:
		void RenderTilesetPanel(float width);
		void RenderTilesetViewPanel(float width);
		void RenderCanvasPanel();

		void SelectTileset(int index);
		void SelectTile(int tileId);

		void HandleCanvasInteraction(const ImVec2& canvasPos, float scale, int tileSize);
		ResizeHandle HitTestHandle(const ImVec2& mouse, const ImVec2& rectMin, const ImVec2& rectMax) const;

		// Persists panel widths + last loaded project path into imgui.ini, under a custom
		// "[TileEditor][State]" section, via ImGui's settings handler extension point.
		static void* SettingsReadOpen(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name);
		static void SettingsReadLine(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line);
		static void SettingsWriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf);

		LDtkEditorData m_data;
		bool m_projectLoaded = false;
		std::string m_projectPathInput;

		int m_selectedTilesetIndex = -1;
		sf::Texture m_tilesetTexture;
		bool m_textureLoaded = false;

		// widths of the two leftmost panels; the canvas panel always takes the remaining space.
		// Adjustable by dragging the splitters between panels.
		float m_tilesetPanelWidth = 220.0f;
		float m_tilesetViewPanelWidth = 340.0f;

		// set when a project path was restored from imgui.ini; consumed on the first
		// OnImGuiRender() call so an explicit LoadProject() (e.g. from argv) still wins.
		bool m_pendingAutoLoad = false;

		int m_selectedTileId = 0;
		std::vector<Hitbox> m_activeHitboxes;

		InteractionState m_state = InteractionState::Idle;
		int m_activeHitboxIndex = -1;      // hitbox currently being drawn/dragged/resized
		int m_selectedHitboxIndex = -1;    // hitbox highlighted / deletable
		ResizeHandle m_activeHandle = ResizeHandle::None;

		// tile-space (unscaled pixel) anchor points used while dragging/drawing
		int m_dragAnchorX = 0;
		int m_dragAnchorY = 0;
	};

}
