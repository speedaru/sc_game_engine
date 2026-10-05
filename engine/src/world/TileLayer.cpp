#include <pch.h>
#include <engine/world/TileLayer.h>
#include <engine/utils/logging.h>

namespace sc::world {
	void TileLayer::LoadTiles(const std::vector<TileInstance>& tiles) {
        // a quad is made of 2 triangles
        m_vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
        m_vertices.resize(tiles.size() * TILE_VERTICE_COUNT);

        for (size_t i = 0; i < tiles.size(); ++i) {
            const auto& tile = tiles[i];

            // get a pointer to the vertices of the current tile
            sf::Vertex* triangles = &m_vertices[i * TILE_VERTICE_COUNT];

            // calculate base geometry
            float px = tile.pixelPos.x;
            float py = tile.pixelPos.y;
            float tw = static_cast<float>(tile.textureRect.size.x);
            float th = static_cast<float>(tile.textureRect.size.y);

            // define the 4 corners of the quad
            sf::Vector2f posTopLeft(px, py);
            sf::Vector2f posTopRight(px + tw, py);
            sf::Vector2f posBottomRight(px + tw, py + th);
            sf::Vector2f posBottomLeft(px, py + th);

            // calculate texture coordinates
            float uLeft = static_cast<float>(tile.textureRect.position.x);
            float vTop = static_cast<float>(tile.textureRect.position.y);
            float uRight = uLeft + tw;
            float vBottom = vTop + th;

            sf::Vector2f uvTopLeft(uLeft, vTop);
            sf::Vector2f uvTopRight(uRight, vTop);
            sf::Vector2f uvBottomRight(uRight, vBottom);
            sf::Vector2f uvBottomLeft(uLeft, vBottom);

            // map to triangles
            // top left, top right, bottom right
            triangles[0].position = posTopLeft;     triangles[0].texCoords = uvTopLeft;
            triangles[1].position = posTopRight;    triangles[1].texCoords = uvTopRight;
            triangles[2].position = posBottomRight; triangles[2].texCoords = uvBottomRight;

            // top left, bottom right, bottom left
            triangles[3].position = posTopLeft;     triangles[3].texCoords = uvTopLeft;
            triangles[4].position = posBottomRight; triangles[4].texCoords = uvBottomRight;
            triangles[5].position = posBottomLeft;  triangles[5].texCoords = uvBottomLeft;
        }
	}
}