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

		void PushLayer(std::shared_ptr<ILayer> layer);
		void PopLayer();

		void Run(); // main loop

	private:
		std::unique_ptr<Window> m_window;
		std::vector<std::shared_ptr<ILayer>> m_layers;
	};
}
