#pragma once
#include <string>
#include <unordered_set>

namespace game::world_loader {
	// tag format: _TAG1_TAG2_layer_name
	// a tag can't contain _ inside the tag name
	// each tag is surrounded by a _
	// like this: _TAG1_TAG2_layer_name
	// layer is technically a tag but LDTK doesnt allow multiple _ in a row
	std::unordered_set<std::string> ExtractLayerTags(const std::string& layerName);
}
