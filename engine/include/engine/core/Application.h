#pragma once
#include <vector>
#include <memory>

#include <engine/core/Window.h>
#include <engine/core/ILayer.h>
#include <engine/utils/logging.h>
using logging::LogLevel;

namespace sc::core {
	class Application {
		ADD_CLASS_TAG("Unnamed Application");
	public:
		Application(WindowData&& windowData);
		virtual ~Application();

		// stack changes are deferred to the end of the current frame, so all 3 are
		// safe to call from inside any layer hook without invalidating
		// the loop currently walking the stack

		void PushLayer(std::shared_ptr<ILayer> layer);
		void PopLayer(); // removes the topmost layer
		void RemoveLayer(const std::shared_ptr<ILayer>& layer); // removes a specific layer

		void Run(); // main loop

	private:
		enum class LayerOp { Push, Pop, Remove };

		struct LayerCommand {
			LayerOp op;
			std::shared_ptr<ILayer> layer; // unused for Pop
		};

		// polls OS events and dispatches them down the layer stack
		void ProcessEvents();

		// applies queued layer changes, only safe to call outside every dispatch loop
		void FlushLayerCommands();

	private:
		std::unique_ptr<Window> m_window;
		std::vector<std::shared_ptr<ILayer>> m_layers; // bottom to top
		std::vector<LayerCommand> m_pendingLayerCommands;
	};
}
