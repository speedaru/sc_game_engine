#include <pch.h>
#include <engine/core/Application.h>
#include <engine/renderer/render2d.h>
#include <engine/constants.h>

using namespace sc::renderer;

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

		render2d::Initialize();

		m_window = std::make_unique<Window>(std::move(windowData));
	}

	Application::~Application() {
		LOG_OBJ_I("shutting down engine application");

		// detach directly instead of waiting for FlushLayerCommands
		for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
			(*it)->OnDetach();
		}
		m_layers.clear();

		// anything still queued never attached, so no OnDetach needed
		m_pendingLayerCommands.clear();

		render2d::Shutdown();

		logging::LoggerShutdown();
	}

	void Application::PushLayer(std::shared_ptr<ILayer> layer) {
		m_pendingLayerCommands.push_back({ LayerOp::Push, std::move(layer) });
	}

	void Application::PopLayer() {
		m_pendingLayerCommands.push_back({ LayerOp::Pop, nullptr });
	}

	void Application::RemoveLayer(const std::shared_ptr<ILayer>& layer) {
		m_pendingLayerCommands.push_back({ LayerOp::Remove, layer });
	}

	void Application::FlushLayerCommands() {
		if (m_pendingLayerCommands.empty()) return;

		// move commands before iterating bcs new OnAttach/OnDetach may queue further commands
		std::vector<LayerCommand> commands = std::move(m_pendingLayerCommands);
		m_pendingLayerCommands.clear();

		for (auto& cmd : commands) {
			switch (cmd.op) {
			case LayerOp::Push: {
				m_layers.push_back(cmd.layer);
				cmd.layer->OnAttach();
				break;
			}
			case LayerOp::Pop: {
				if (m_layers.empty()) {
					LOG_OBJ_W("PopLayer requested but the layer stack is empty");
					break;
				}

				std::shared_ptr<ILayer> layer = m_layers.back();
				m_layers.pop_back();
				layer->OnDetach();
				break;
			}
			case LayerOp::Remove: {
				auto it = std::find(m_layers.begin(), m_layers.end(), cmd.layer);
				if (it == m_layers.end()) {
					LOG_OBJ_W("RemoveLayer requested for a layer that isn't on the stack");
					break;
				}

				std::shared_ptr<ILayer> layer = *it;
				m_layers.erase(it);
				layer->OnDetach();
				break;
			}
			}
		}
	}

	void Application::ProcessEvents() {
		sf::RenderWindow& window = m_window->GetNativeWindow();

		while (const std::optional<sf::Event> event = window.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				window.close();
				continue;
			}

			// top down
			for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
				if ((*it)->OnEvent(*event)) break;
			}
		}
	}

	void Application::Run() {
		LOG_OBJ_I("starting main game loop");

		// attach whatever was queued before the loop started
		FlushLayerCommands();

		sf::Clock clock;
		constexpr const float timeStep = 1.f / PHYSICS_HZ;
		float accumulator = 0.f;

		sf::RenderWindow& window = m_window->GetNativeWindow();
		while (window.isOpen()) {
			float deltaTime = clock.restart().asSeconds();
			accumulator += deltaTime;

			// process OS window events
			ProcessEvents();

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

			// apply layer stack changes here
			FlushLayerCommands();
		}
	}
}
