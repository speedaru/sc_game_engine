#pragma once
#include <cstdint>
#include <string>

namespace sc::world {
	enum class LayerType : uint32_t {
        Tile,
        YSortedTile,
        Entity,
        Collision
    };

    enum class LayerRenderType : uint32_t {
        Vertex,
        QuadQueue
    };

    struct LayerId {
        std::string name;
        int32_t uid;
    };

	class ILevelLayer {
    public:
        virtual ~ILevelLayer() = default;

        const std::string& GetName() const { return m_id.name; }
        int32_t GetUid() const { return m_id.uid; }
        LayerType GetType() const { return m_type; }
        bool IsDepthBoundary() const { return m_isDepthBoundary; }

    protected:
        // ensures that only derived classes can call the constructor
        ILevelLayer(LayerType type, const LayerId& id, bool isDepthBoundary)
            : m_type(type), m_id(id), m_isDepthBoundary(isDepthBoundary) {}

    private:
        LayerId m_id;
        LayerType m_type;
        bool m_isDepthBoundary;
    };
}
