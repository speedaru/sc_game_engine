#include <pch.h>
#include <engine/constants.h>
#include <engine/core/Application.h>

namespace sc::core {
	Application::Application(WindowData&& windowData) {
#if _DEBUG == 1
		LogLevel logLevel = LogLevel::None
			| LogLevel::Debug
			| LogLevel::Info
			| LogLevel::Error
			| LogLevel::Warn
			| LogLevel::Trace;
#elif _DEBUG == 0
		LogLevel logLevel = LogLevel::None
			| LogLevel::Error;
#endif
		logging::LoggerInit("sc_engine_log.txt", logLevel);

		LOG_OBJ_I("initializing engine application");
		m_window = std::make_unique<Window>(std::move(windowData));
	}

	Application::~Application() {
		LOG_OBJ_I("shutting down engine application");

		logging::LoggerShutdown();
	}

	void Application::Run() {
		LOG_OBJ_I("starting main game loop");

		sf::Clock clock;
		constexpr const float timeStep = 1.f / PHYSICS_HZ;
		float accumulator = 0.f;

		sf::RenderWindow& window = m_window->GetNativeWindow();
		while (window.isOpen()) {
			float deltaTime = clock.restart().asSeconds();
			accumulator += deltaTime;

			// process OS window events
			m_window->Update();

			// fixed update
			while (accumulator >= timeStep) {
				// TODO: update control_module and ECS using timeStep
				accumulator -= timeStep;
			}

			LOG_D("rendering window");

			// render
			window.clear(sf::Color::Black);
			// TODO: ecs render queue
			window.display();
		}
	}
}
