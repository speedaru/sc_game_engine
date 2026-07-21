#include <engine/core/Application.h>
#include <engine/utils/logging.h>

int main(int argc, char** argv) {
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
	logging::LoggerInit("rpg_game_log.txt", logLevel);

	sc::core::Application app(sc::core::WindowData{ "very cool RPG", 1280, 720 });
	app.Run();

	logging::LoggerShutdown();
}
