#include <pch.h>
#include <loaders/world/LayerTags.h>

#include <engine/utils/logging.h>

namespace game::world_loader {
	std::unordered_set<std::string> ExtractLayerTags(const std::string& layerName) {
		std::unordered_set<std::string> tags;

		size_t cursor = 0;
		while (layerName[cursor++] == '_') {
			// extract tag by looking for ending _
			size_t end = layerName.find('_', cursor);
			if (end == std::string::npos) break; // not a tag bcs doesnt end with _

			tags.insert(layerName.substr(cursor, end - cursor));
			cursor = end;
		}

		return tags;
	}
}
