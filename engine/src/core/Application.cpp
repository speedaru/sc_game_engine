#include <pch.h>
#include <engine/core/Application.h>
#include <engine/renderer/render2d.h>
#include <engine/constants.h>
#include <engine/input/Input.h>
#include <engine/input/InputState.h>
#include <engine/input/InputLayer.h>

#if SC_ENABLE_DEBUG_TOOLS
	#include <imgui-SFML.h>
	#include <imgui.h>

	#include <engine/debug/DebugLayer.h>
#endif

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

		// needs to be last layer so it's the last one to see events
		PushLayer(std::make_shared<input::InputLayer>());

#if SC_ENABLE_DEBUG_TOOLS
		m_debugToolsReady = ImGui::SFML::Init(m_window->GetNativeWindow());
		if (!m_debugToolsReady) {
			LOG_OBJ_E("imgui-sfml failed to initialize, debug tooling is disabled this run");
		}
#endif
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

#if SC_ENABLE_DEBUG_TOOLS
		if (m_debugToolsReady) {
			ImGui::SFML::Shutdown();
			m_debugToolsReady = false;
		}
#endif

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
#if SC_ENABLE_DEBUG_TOOLS
			// imgui has to see every event so that imgui state stays synced
			if (m_debugToolsReady) ImGui::SFML::ProcessEvent(window, *event);
#endif

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

#if SC_ENABLE_DEBUG_TOOLS
		// pushed at the very top above all initial game layers
		if (m_debugToolsReady) {
			PushLayer(std::make_shared<debug::DebugLayer>());
		}
#endif

		// attach whatever was queued before the loop started
		FlushLayerCommands();

		sf::Clock clock;
		constexpr float TIME_STEP = 1.f / PHYSICS_HZ;
		float accumulator = 0.f;

		sf::RenderWindow& window = m_window->GetNativeWindow();
		while (window.isOpen()) {
			float deltaTime = clock.restart().asSeconds();
			accumulator += std::min(0.25f, deltaTime); // cap to 0.25s max

			input::NewFrame();

			// process OS window events
			ProcessEvents();

#if SC_ENABLE_DEBUG_TOOLS
			// start imgui frame
			if (m_debugToolsReady) ImGui::SFML::Update(window, sf::seconds(deltaTime));
#endif

			// fixed update logic
			while (accumulator >= TIME_STEP) {
				for (auto& layer : m_layers) {
					layer->OnFixedUpdate(TIME_STEP);
				}
				accumulator -= TIME_STEP;
			}

			const float alpha = accumulator / TIME_STEP; // 0 = just ticked, approaching 1 = next tick imminent

			// variable update logic
			for (auto& layer : m_layers) {
				layer->OnUpdate(deltaTime);
			}

			// render
			window.clear(sf::Color::Black);
			for (auto& layer : m_layers) {
				layer->OnRender(window, alpha);
			}

#if SC_ENABLE_DEBUG_TOOLS
			// render imgui above all other game layers
			if (m_debugToolsReady) ImGui::SFML::Render(window);
#endif

			window.display();

			// apply layer stack changes here
			FlushLayerCommands();
		}
	}
}
