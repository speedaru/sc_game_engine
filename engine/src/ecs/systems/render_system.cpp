#include <pch.h>
#include <engine/ecs/systems/render_system.h>
#include <engine/ecs/Components.h>
#include <engine/renderer/render2d.h>

using namespace sc::graphics;
using namespace sc::renderer;

namespace sc::ecs::render_system {
	namespace {
		void DrawEntityLayer(const Registry& registry, const world::ILevelLayer* layer) {
			auto view = registry.GetRegistry().view<const TransformComponent, const SpriteComponent>();

			for (auto [entity, trans, spr] : view.each()) {
				if (spr.layerUid != layer->GetId().uid) continue;

				QuadProps quad;
				quad.position = trans.pos;
				quad.pivot = trans.pivot;

				quad.texture = spr.texture;
				quad.textureRect = spr.rect;

				render2d::SubmitQuad(quad);
			}
		}
	}

	void RenderWorld(const Registry& registry, const world::Level& level, sf::RenderWindow& window, const Camera2D& camera) {
		render2d::BeginBatch(window, camera);

		for (const auto& layer : level.GetLayers()) {
			// we are about to draw a flat tile layer so apply Y-sort to all the quads
			// in the quadQueue BEFORE drawing the current flat layer
			if (layer->IsDepthBoundary()) {
				render2d::FlushQuads();
			}

			if (layer->GetType() == world::LayerType::Tile) {
				auto* tileLayer = static_cast<world::TileLayer*>(layer.get());
				render2d::DrawTileLayer(*tileLayer);
			}
			else if (layer->GetType() == world::LayerType::YSortedTile) {
				auto* yTileLayer = static_cast<world::YSortedTileLayer*>(layer.get());
				render2d::SubmitYSortedTileLayer(*yTileLayer);
			}
			else if (layer->GetType() == world::LayerType::Entity) {
				DrawEntityLayer(registry, layer.get());
			}
			else if (layer->GetType() == world::LayerType::Collision) {
				// TODO: implement debug collisions viewing
			}
		}

		// flush rest of quads
		render2d::FlushQuads();

		render2d::EndBatch();
	}
}
