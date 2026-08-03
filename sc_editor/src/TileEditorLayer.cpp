#include "TileEditorLayer.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

#include <imgui-SFML.h>
#include <imgui_internal.h> // ImGuiSettingsHandler / AddSettingsHandler / MarkIniSettingsDirty
#include <misc/cpp/imgui_stdlib.h>

namespace sc_editor {

	namespace {
		constexpr float kCanvasSize = 512.0f;
		constexpr float kHandleMargin = 6.0f;
		constexpr float kSplitterThickness = 6.0f;
		constexpr float kMinPanelWidth = 80.0f;
		constexpr float kMinCanvasWidth = 150.0f;

		ImTextureID ToImTextureID(unsigned int glHandle) {
			ImTextureID id{};
			std::memcpy(&id, &glHandle, sizeof(glHandle));
			return id;
		}

		int ClampInt(int v, int lo, int hi) {
			return v < lo ? lo : (v > hi ? hi : v);
		}

		bool PointInRect(const ImVec2& p, const ImVec2& rectMin, const ImVec2& rectMax) {
			return p.x >= rectMin.x && p.x <= rectMax.x && p.y >= rectMin.y && p.y <= rectMax.y;
		}

		// Draggable vertical bar placed between two panels via ImGui::SameLine(). Adjusts
		// *width in place and shows a horizontal-resize cursor while hovered/dragged.
		void RenderSplitter(const char* id, float* width, float height, float minWidth, float maxWidth) {
			ImGui::Button(id, ImVec2(kSplitterThickness, height));
			if (ImGui::IsItemActive() && ImGui::GetIO().MouseDelta.x != 0.0f) {
				*width += ImGui::GetIO().MouseDelta.x;
				ImGui::MarkIniSettingsDirty();
			}
			*width = std::clamp(*width, minWidth, std::max(minWidth, maxWidth));

			if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
		}

		// Sets the OS/software cursor to indicate which direction a hitbox edge would
		// resize in, so the user gets the same affordance as resizing a normal window.
		void ApplyResizeCursor(ResizeHandle handle) {
			switch (handle) {
				case ResizeHandle::N:
				case ResizeHandle::S:
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
					break;
				case ResizeHandle::E:
				case ResizeHandle::W:
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
					break;
				case ResizeHandle::NE:
				case ResizeHandle::SW:
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
					break;
				case ResizeHandle::NW:
				case ResizeHandle::SE:
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
					break;
				default:
					break;
			}
		}
	}

	TileEditorLayer::TileEditorLayer() {
		// Must run before ImGui's first NewFrame() (which is when the .ini file gets parsed),
		// so the handler is registered from the constructor rather than lazily in OnImGuiRender().
		ImGuiSettingsHandler handler;
		handler.TypeName = "TileEditor";
		handler.TypeHash = ImHashStr("TileEditor");
		handler.ReadOpenFn = &TileEditorLayer::SettingsReadOpen;
		handler.ReadLineFn = &TileEditorLayer::SettingsReadLine;
		handler.WriteAllFn = &TileEditorLayer::SettingsWriteAll;
		handler.UserData = this;
		ImGui::AddSettingsHandler(&handler);
	}

	bool TileEditorLayer::LoadProject(const std::string& path) {
		m_projectPathInput = path;
		m_projectLoaded = m_data.Load(path);

		m_selectedTilesetIndex = -1;
		m_textureLoaded = false;
		m_selectedTileId = 0;
		m_activeHitboxes.clear();
		m_state = InteractionState::Idle;
		m_activeHitboxIndex = -1;
		m_selectedHitboxIndex = -1;

		ImGui::MarkIniSettingsDirty();

		if (!m_projectLoaded) {
			std::cerr << "[TileEditorLayer] failed to load project: " << path << "\n";
		}
		return m_projectLoaded;
	}

	void* TileEditorLayer::SettingsReadOpen(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) {
		if (strcmp(name, "State") != 0) {
			return nullptr;
		}
		return handler->UserData;
	}

	void TileEditorLayer::SettingsReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
		TileEditorLayer* self = static_cast<TileEditorLayer*>(entry);
		if (!self) {
			return;
		}

		if (strncmp(line, "TilesetPanelWidth=", 18) == 0) {
			self->m_tilesetPanelWidth = std::strtof(line + 18, nullptr);
		}
		else if (strncmp(line, "TilesetViewPanelWidth=", 22) == 0) {
			self->m_tilesetViewPanelWidth = std::strtof(line + 22, nullptr);
		}
		else if (strncmp(line, "ProjectPath=", 12) == 0) {
			self->m_projectPathInput = line + 12;
			self->m_pendingAutoLoad = !self->m_projectPathInput.empty();
		}
	}

	void TileEditorLayer::SettingsWriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf) {
		TileEditorLayer* self = static_cast<TileEditorLayer*>(handler->UserData);
		if (!self) {
			return;
		}

		const std::string& projectPath = self->m_projectLoaded ? self->m_data.GetProjectPath() : self->m_projectPathInput;

		out_buf->appendf("[TileEditor][State]\n");
		out_buf->appendf("TilesetPanelWidth=%f\n", self->m_tilesetPanelWidth);
		out_buf->appendf("TilesetViewPanelWidth=%f\n", self->m_tilesetViewPanelWidth);
		out_buf->appendf("ProjectPath=%s\n", projectPath.c_str());
		out_buf->appendf("\n");
	}

	void TileEditorLayer::SelectTileset(int index) {
		m_selectedTilesetIndex = index;
		m_textureLoaded = false;

		const auto& tilesets = m_data.GetTilesets();
		if (index < 0 || index >= (int)tilesets.size()) {
			return;
		}

		std::filesystem::path imagePath = tilesets[index].relPath;
		if (imagePath.is_relative()) {
			std::filesystem::path projectDir = std::filesystem::path(m_data.GetProjectPath()).parent_path();
			imagePath = projectDir / imagePath;
		}

		if (m_tilesetTexture.loadFromFile(imagePath)) {
			m_textureLoaded = true;
		}
		else {
			std::cerr << "[TileEditorLayer] failed to load tileset image: " << imagePath.string() << "\n";
		}

		SelectTile(0);
	}

	void TileEditorLayer::SelectTile(int tileId) {
		m_selectedTileId = tileId;
		m_activeHitboxes = m_data.GetTileCollisions(m_selectedTilesetIndex, tileId);
		m_state = InteractionState::Idle;
		m_activeHitboxIndex = -1;
		m_selectedHitboxIndex = -1;
		m_activeHandle = ResizeHandle::None;
	}

	void TileEditorLayer::OnImGuiRender() {
		if (m_pendingAutoLoad && !m_projectLoaded) {
			LoadProject(m_projectPathInput);
		}
		m_pendingAutoLoad = false;

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("LDtk Collision Editor", nullptr,
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::InputText("Project Path", &m_projectPathInput);
		ImGui::SameLine();
		if (ImGui::Button("Load")) {
			LoadProject(m_projectPathInput);
		}

		ImGui::BeginDisabled(!m_projectLoaded || m_selectedTilesetIndex < 0);
		if (ImGui::Button("Save Collisions", ImVec2(-FLT_MIN, 0))) {
			m_data.SaveTileCollisions(m_selectedTilesetIndex, m_selectedTileId, m_activeHitboxes);
		}
		ImGui::EndDisabled();

		ImGui::Separator();

		if (m_projectLoaded) {
			float availableWidth = ImGui::GetContentRegionAvail().x;
			float availableHeight = ImGui::GetContentRegionAvail().y;

			float maxTilesetPanelWidth = availableWidth - m_tilesetViewPanelWidth - kMinCanvasWidth - 2.0f * kSplitterThickness;
			float maxTilesetViewPanelWidth = availableWidth - m_tilesetPanelWidth - kMinCanvasWidth - 2.0f * kSplitterThickness;

			RenderTilesetPanel(m_tilesetPanelWidth);
			ImGui::SameLine();
			RenderSplitter("##tilesetSplitter", &m_tilesetPanelWidth, availableHeight, kMinPanelWidth, maxTilesetPanelWidth);
			ImGui::SameLine();

			RenderTilesetViewPanel(m_tilesetViewPanelWidth);
			ImGui::SameLine();
			RenderSplitter("##tilesetViewSplitter", &m_tilesetViewPanelWidth, availableHeight, kMinPanelWidth, maxTilesetViewPanelWidth);
			ImGui::SameLine();

			RenderCanvasPanel();
		}

		ImGui::End();
	}

	void TileEditorLayer::RenderTilesetPanel(float width) {
		ImGui::BeginChild("TilesetsPanel", ImVec2(width, 0), true);
		ImGui::TextUnformatted("Tilesets");
		ImGui::Separator();

		const auto& tilesets = m_data.GetTilesets();
		for (int i = 0; i < (int)tilesets.size(); ++i) {
			bool selected = (i == m_selectedTilesetIndex);
			if (ImGui::Selectable(tilesets[i].identifier.c_str(), selected) && i != m_selectedTilesetIndex) {
				SelectTileset(i);
			}
		}

		ImGui::EndChild();
	}

	void TileEditorLayer::RenderTilesetViewPanel(float width) {
		ImGui::BeginChild("TilesetViewPanel", ImVec2(width, 0), true);

		if (m_textureLoaded && m_selectedTilesetIndex >= 0) {
			const TilesetInfo& ts = m_data.GetTilesets()[m_selectedTilesetIndex];

			ImVec2 imagePos = ImGui::GetCursorScreenPos();
			ImGui::Image(m_tilesetTexture);

			if (ts.tileSize > 0 && ts.columns > 0) {
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					ImVec2 mouse = ImGui::GetIO().MousePos;
					int col = (int)((mouse.x - imagePos.x) / ts.tileSize);
					int row = (int)((mouse.y - imagePos.y) / ts.tileSize);
					SelectTile(row * ts.columns + col);
				}

				int col = m_selectedTileId % ts.columns;
				int row = m_selectedTileId / ts.columns;
				ImVec2 tileMin(imagePos.x + col * ts.tileSize, imagePos.y + row * ts.tileSize);
				ImVec2 tileMax(tileMin.x + ts.tileSize, tileMin.y + ts.tileSize);
				ImGui::GetWindowDrawList()->AddRect(tileMin, tileMax, IM_COL32(255, 255, 0, 200), 0.0f, 0, 2.0f);
			}
		}
		else {
			ImGui::TextUnformatted("Select a tileset to preview it here.");
		}

		ImGui::EndChild();
	}

	void TileEditorLayer::RenderCanvasPanel() {
		ImGui::BeginChild("CanvasPanel", ImVec2(0, 0), true);

		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImGui::Dummy(ImVec2(kCanvasSize, kCanvasSize));
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasMax(canvasPos.x + kCanvasSize, canvasPos.y + kCanvasSize);

		drawList->AddRectFilled(canvasPos, canvasMax, IM_COL32(40, 40, 40, 255));

		const bool haveTile = m_textureLoaded && m_selectedTilesetIndex >= 0;
		const TilesetInfo* ts = haveTile ? &m_data.GetTilesets()[m_selectedTilesetIndex] : nullptr;

		if (ts && ts->tileSize > 0 && ts->columns > 0) {
			int col = m_selectedTileId % ts->columns;
			int row = m_selectedTileId / ts->columns;
			float scale = kCanvasSize / (float)ts->tileSize;

			sf::Vector2u texSize = m_tilesetTexture.getSize();
			ImVec2 uv0((col * ts->tileSize) / (float)texSize.x, (row * ts->tileSize) / (float)texSize.y);
			ImVec2 uv1(((col + 1) * ts->tileSize) / (float)texSize.x, ((row + 1) * ts->tileSize) / (float)texSize.y);

			drawList->AddImage(ToImTextureID(m_tilesetTexture.getNativeHandle()), canvasPos, canvasMax, uv0, uv1);

			for (int i = 0; i <= ts->tileSize; ++i) {
				float x = canvasPos.x + i * scale;
				drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasMax.y), IM_COL32(255, 255, 255, 40));
				float y = canvasPos.y + i * scale;
				drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasMax.x, y), IM_COL32(255, 255, 255, 40));
			}

			for (int i = 0; i < (int)m_activeHitboxes.size(); ++i) {
				const Hitbox& hb = m_activeHitboxes[i];
				ImVec2 rmin(canvasPos.x + hb.x * scale, canvasPos.y + hb.y * scale);
				ImVec2 rmax(canvasPos.x + (hb.x + hb.w) * scale, canvasPos.y + (hb.y + hb.h) * scale);

				bool selected = (i == m_selectedHitboxIndex);
				ImU32 fill = selected ? IM_COL32(255, 80, 80, 90) : IM_COL32(80, 160, 255, 70);
				ImU32 border = selected ? IM_COL32(255, 80, 80, 255) : IM_COL32(80, 160, 255, 255);
				drawList->AddRectFilled(rmin, rmax, fill);
				drawList->AddRect(rmin, rmax, border, 0.0f, 0, 2.0f);
			}

			if (ImGui::IsWindowHovered()) {
				ResizeHandle hoverHandle = ResizeHandle::None;
				if (m_state == InteractionState::Resizing) {
					hoverHandle = m_activeHandle;
				}
				else if (m_state == InteractionState::Idle) {
					ImVec2 mouse = ImGui::GetIO().MousePos;
					for (int i = (int)m_activeHitboxes.size() - 1; i >= 0; --i) {
						const Hitbox& hb = m_activeHitboxes[i];
						ImVec2 rmin(canvasPos.x + hb.x * scale, canvasPos.y + hb.y * scale);
						ImVec2 rmax(canvasPos.x + (hb.x + hb.w) * scale, canvasPos.y + (hb.y + hb.h) * scale);
						hoverHandle = HitTestHandle(mouse, rmin, rmax);
						if (hoverHandle != ResizeHandle::None) {
							break;
						}
					}
				}
				ApplyResizeCursor(hoverHandle);

				HandleCanvasInteraction(canvasPos, scale, ts->tileSize);
			}

			if (m_state == InteractionState::Idle && m_selectedHitboxIndex >= 0 && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
				m_activeHitboxes.erase(m_activeHitboxes.begin() + m_selectedHitboxIndex);
				m_selectedHitboxIndex = -1;
			}
		}
		else {
			ImGui::SetCursorScreenPos(ImVec2(canvasPos.x + 8, canvasPos.y + 8));
			ImGui::TextUnformatted("Select a tile to edit its collisions.");
		}

		ImGui::EndChild();
	}

	ResizeHandle TileEditorLayer::HitTestHandle(const ImVec2& mouse, const ImVec2& rectMin, const ImVec2& rectMax) const {
		bool withinX = mouse.x >= rectMin.x - kHandleMargin && mouse.x <= rectMax.x + kHandleMargin;
		bool withinY = mouse.y >= rectMin.y - kHandleMargin && mouse.y <= rectMax.y + kHandleMargin;
		if (!withinX || !withinY) {
			return ResizeHandle::None;
		}

		bool nearLeft = std::fabs(mouse.x - rectMin.x) <= kHandleMargin;
		bool nearRight = std::fabs(mouse.x - rectMax.x) <= kHandleMargin;
		bool nearTop = std::fabs(mouse.y - rectMin.y) <= kHandleMargin;
		bool nearBottom = std::fabs(mouse.y - rectMax.y) <= kHandleMargin;

		if (nearLeft && nearTop) return ResizeHandle::NW;
		if (nearRight && nearTop) return ResizeHandle::NE;
		if (nearLeft && nearBottom) return ResizeHandle::SW;
		if (nearRight && nearBottom) return ResizeHandle::SE;
		if (nearLeft) return ResizeHandle::W;
		if (nearRight) return ResizeHandle::E;
		if (nearTop) return ResizeHandle::N;
		if (nearBottom) return ResizeHandle::S;
		return ResizeHandle::None;
	}

	void TileEditorLayer::HandleCanvasInteraction(const ImVec2& canvasPos, float scale, int tileSize) {
		ImVec2 mouse = ImGui::GetIO().MousePos;
		int snappedX = ClampInt((int)std::round((mouse.x - canvasPos.x) / scale), 0, tileSize);
		int snappedY = ClampInt((int)std::round((mouse.y - canvasPos.y) / scale), 0, tileSize);

		if (m_state == InteractionState::Idle) {
			if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				return;
			}

			// topmost box first: a resize handle takes priority over a plain drag
			for (int i = (int)m_activeHitboxes.size() - 1; i >= 0; --i) {
				const Hitbox& hb = m_activeHitboxes[i];
				ImVec2 rmin(canvasPos.x + hb.x * scale, canvasPos.y + hb.y * scale);
				ImVec2 rmax(canvasPos.x + (hb.x + hb.w) * scale, canvasPos.y + (hb.y + hb.h) * scale);

				ResizeHandle handle = HitTestHandle(mouse, rmin, rmax);
				if (handle != ResizeHandle::None) {
					m_state = InteractionState::Resizing;
					m_activeHitboxIndex = i;
					m_selectedHitboxIndex = i;
					m_activeHandle = handle;
					return;
				}
			}

			for (int i = (int)m_activeHitboxes.size() - 1; i >= 0; --i) {
				const Hitbox& hb = m_activeHitboxes[i];
				ImVec2 rmin(canvasPos.x + hb.x * scale, canvasPos.y + hb.y * scale);
				ImVec2 rmax(canvasPos.x + (hb.x + hb.w) * scale, canvasPos.y + (hb.y + hb.h) * scale);

				if (PointInRect(mouse, rmin, rmax)) {
					m_state = InteractionState::Dragging;
					m_activeHitboxIndex = i;
					m_selectedHitboxIndex = i;
					m_dragAnchorX = snappedX - hb.x;
					m_dragAnchorY = snappedY - hb.y;
					return;
				}
			}

			// empty space: begin drawing a new box
			m_selectedHitboxIndex = -1;
			Hitbox newBox;
			newBox.x = snappedX;
			newBox.y = snappedY;
			m_activeHitboxes.push_back(newBox);
			m_activeHitboxIndex = (int)m_activeHitboxes.size() - 1;
			m_dragAnchorX = snappedX;
			m_dragAnchorY = snappedY;
			m_state = InteractionState::Drawing;
			return;
		}

		if (m_activeHitboxIndex < 0 || m_activeHitboxIndex >= (int)m_activeHitboxes.size()) {
			m_state = InteractionState::Idle;
			return;
		}
		Hitbox& hb = m_activeHitboxes[m_activeHitboxIndex];
		bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left);

		if (m_state == InteractionState::Drawing) {
			int x0 = std::min(m_dragAnchorX, snappedX);
			int x1 = std::max(m_dragAnchorX, snappedX);
			int y0 = std::min(m_dragAnchorY, snappedY);
			int y1 = std::max(m_dragAnchorY, snappedY);
			hb.x = x0;
			hb.y = y0;
			hb.w = x1 - x0;
			hb.h = y1 - y0;

			if (released) {
				if (hb.w <= 0 || hb.h <= 0) {
					m_activeHitboxes.erase(m_activeHitboxes.begin() + m_activeHitboxIndex);
					m_selectedHitboxIndex = -1;
				}
				else {
					m_selectedHitboxIndex = m_activeHitboxIndex;
				}
				m_activeHitboxIndex = -1;
				m_state = InteractionState::Idle;
			}
		}
		else if (m_state == InteractionState::Dragging) {
			hb.x = ClampInt(snappedX - m_dragAnchorX, 0, tileSize - hb.w);
			hb.y = ClampInt(snappedY - m_dragAnchorY, 0, tileSize - hb.h);

			if (released) {
				m_activeHitboxIndex = -1;
				m_state = InteractionState::Idle;
			}
		}
		else if (m_state == InteractionState::Resizing) {
			int left = hb.x;
			int top = hb.y;
			int right = hb.x + hb.w;
			int bottom = hb.y + hb.h;

			bool touchesW = m_activeHandle == ResizeHandle::W || m_activeHandle == ResizeHandle::NW || m_activeHandle == ResizeHandle::SW;
			bool touchesE = m_activeHandle == ResizeHandle::E || m_activeHandle == ResizeHandle::NE || m_activeHandle == ResizeHandle::SE;
			bool touchesN = m_activeHandle == ResizeHandle::N || m_activeHandle == ResizeHandle::NW || m_activeHandle == ResizeHandle::NE;
			bool touchesS = m_activeHandle == ResizeHandle::S || m_activeHandle == ResizeHandle::SW || m_activeHandle == ResizeHandle::SE;

			if (touchesW) left = ClampInt(snappedX, 0, right - 1);
			if (touchesE) right = ClampInt(snappedX, left + 1, tileSize);
			if (touchesN) top = ClampInt(snappedY, 0, bottom - 1);
			if (touchesS) bottom = ClampInt(snappedY, top + 1, tileSize);

			hb.x = left;
			hb.y = top;
			hb.w = right - left;
			hb.h = bottom - top;

			if (released) {
				m_activeHitboxIndex = -1;
				m_state = InteractionState::Idle;
			}
		}
	}

}
