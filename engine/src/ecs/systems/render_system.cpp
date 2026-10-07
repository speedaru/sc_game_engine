#include <pch.h>
#include <engine/ecs/systems/render_system.h>
#include <engine/ecs/Components.h>
#include <engine/renderer/render2d.h>
#include <engine/math/math.h>

using namespace sc::graphics;
using namespace sc::renderer;

namespace sc::ecs::render_system {
	namespace {
		void DrawEntityLayer(const Registry& registry, int32_t levelUid, const world::ILevelLayer* layer, float alpha) {
			auto view = registry.ViewLevel<const TransformComponent, const SpriteComponent>(levelUid);

			for (auto [entity, trans, spr] : view) {
				// layer uid is the layer definition, shared across levels, so the level filter above is what keeps other levels out
				if (spr.layerUid != layer->GetUid()) continue;

				QuadProps quad;
				quad.position = math::Lerp(trans.prevPos, trans.pos, alpha);
				quad.pivot = trans.pivot;

				quad.texture = spr.texture;
				quad.textureRect = spr.rect;

				render2d::SubmitQuad(quad);
			}
		}
	}

	void RenderWorld(const Registry& registry, const world::Level& level, sf::RenderWindow& window, const Camera2D& camera, float alpha) {
		render2d::BeginBatch(window, camera);

		for (const auto& layer : level.GetLayers()) {
			// we are about to draw a flat tile layer so apply Y-sort to all the quads
			// in the quadQueue BEFORE drawing the current flat layer
			if (layer->IsDepthBoundary()) {
				render2d::FlushQuads();
			}

			switch (layer->GetType()) {
			case world::LayerType::Tile:
			{
				auto* tileLayer = static_cast<world::TileLayer*>(layer.get());
				render2d::DrawTileLayer(*tileLayer);
				break;
			}
			case world::LayerType::YSortedTile:
			{
				auto* yTileLayer = static_cast<world::YSortedTileLayer*>(layer.get());
				render2d::SubmitYSortedTileLayer(*yTileLayer);
				break;
			}
			case world::LayerType::Entity:
			{
				DrawEntityLayer(registry, level.GetUid(), layer.get(), alpha);
				break;
			}
			}
		}

		// flush rest of quads
		render2d::FlushQuads();

		render2d::EndBatch();
	}
}
