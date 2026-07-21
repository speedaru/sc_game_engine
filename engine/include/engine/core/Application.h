#pragma once
#include <engine/core/Window.h>
#include <engine/utils/logging.h>
using logging::LogLevel;

namespace sc::core {
	class Application {
		ADD_CLASS_TAG("Unnamed Application");
	public:
		Application(WindowData&& windowData);
		virtual ~Application();

		void Run(); // main loop

	private:
		std::unique_ptr<Window> m_window;
	};
}
