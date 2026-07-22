#include <pch.h>
#include <engine/constants.h>
#include <engine/core/Application.h>
#include <engine/graphics/render2d.h>

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
		logging::LoggerInit(LOG_FILE_NAME, logLevel);
		LOG_OBJ_I("initializing engine application");

		graphics::render2d::Initialize();

		m_window = std::make_unique<Window>(std::move(windowData));
	}

	Application::~Application() {
		LOG_OBJ_I("shutting down engine application");

		const size_t layerCount = m_layers.size();
		for (size_t i = 0; i < layerCount; i++) {
			PopLayer();
		}

		graphics::render2d::Shutdown();

		logging::LoggerShutdown();
	}

	void Application::PushLayer(std::shared_ptr<ILayer> layer) {
		m_layers.push_back(layer);
		layer->OnAttach();
	}

	void Application::PopLayer() {
		auto layer = m_layers.back();
		layer->OnDetach();
		m_layers.pop_back();
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

			// fixed update logic
			while (accumulator >= timeStep) {
				for (auto& layer : m_layers) {
					layer->OnFixedUpdate(timeStep);
				}
				accumulator -= timeStep;
			}

			// variable update logic
			for (auto& layer : m_layers) {
				layer->OnUpdate(deltaTime);
			}

			// render
			window.clear(sf::Color::Black);
			for (auto& layer : m_layers) {
				layer->OnRender(window);
			}
			window.display();
		}
	}
}
