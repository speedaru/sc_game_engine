#pragma once
#include <memory> // for va_list

#define __RELATIVE_FILE__ (__FILE__ + sizeof(PROJECT_DIR "src\\") - 1)

namespace logging {
	enum class LogLevel {
		None	= 0,
		Trace	= 1 << 0,
		Debug	= 1 << 1,
		Info	= 1 << 2,
		Warn	= 1 << 3,
		Error	= 1 << 4,
	};

	inline LogLevel operator|(LogLevel a, LogLevel b) { return static_cast<LogLevel>((unsigned int)a | (unsigned int)b); }
	inline bool operator&(LogLevel a, LogLevel b) { return (unsigned int)a & (unsigned int)b; }

	const char* GetLogLevelName(LogLevel level);

	// initialize logger (opens the file)
	void LoggerInit(const char* filename, LogLevel allowedLevels);
	// close logger
	void LoggerShutdown();
	// core logging function (don't call directly, use macros)
	void LogOutput(LogLevel level, const char* file, int line, const char* func, const char* fmt, ...);
	void LogOutputRawV(const char* fmt, va_list args);
	void LogOutputRaw(const char* fmt, ...);

	constexpr bool LOG_TO_FILE = true;
	constexpr bool LOG_TO_CONSOLE = true;
	constexpr bool LOG_TIME = false; // date time
	constexpr bool LOG_FILENAME = false; // filename and line number
	constexpr bool LOG_FUNCTION = false; // function

	constexpr int SPACES_PER_INDENT = 2; // spaces per indentation level
	inline int g_IndentLevel = 0; // Use thread-local if in multi-threaded user mode
	inline bool g_pauseLog = false;

	#define LOG_T(...) LogOutput(logging::LogLevel::Trace, __RELATIVE_FILE__, __LINE__, __func__, __VA_ARGS__)
	#define LOG_D(...) LogOutput(logging::LogLevel::Debug, __RELATIVE_FILE__, __LINE__, __func__, __VA_ARGS__)
	#define LOG_I(...) LogOutput(logging::LogLevel::Info,  __RELATIVE_FILE__, __LINE__, __func__, __VA_ARGS__)
	#define LOG_W(...) LogOutput(logging::LogLevel::Warn,  __RELATIVE_FILE__, __LINE__, __func__, __VA_ARGS__)
	#define LOG_E(...) LogOutput(logging::LogLevel::Error, __RELATIVE_FILE__, __LINE__, __func__, __VA_ARGS__)

	#define LOG_OBJ(level, fmt, ...) LogOutput(level, __RELATIVE_FILE__, __LINE__, __func__, "[%s:0x%p] " fmt, this->m_tag, this, ##__VA_ARGS__)
	#define LOG_OBJ_T(fmt, ...) LOG_OBJ(logging::LogLevel::Trace, fmt, ##__VA_ARGS__)
	#define LOG_OBJ_D(fmt, ...) LOG_OBJ(logging::LogLevel::Debug, fmt, ##__VA_ARGS__)
	#define LOG_OBJ_I(fmt, ...) LOG_OBJ(logging::LogLevel::Info, fmt, ##__VA_ARGS__)
	#define LOG_OBJ_W(fmt, ...) LOG_OBJ(logging::LogLevel::Warn, fmt, ##__VA_ARGS__)
	#define LOG_OBJ_E(fmt, ...) LOG_OBJ(logging::LogLevel::Error, fmt, ##__VA_ARGS__)

	class ScopedLog {
	public:
		ScopedLog(const char* func) : m_func(func) {
			LOG_I(">> %s\n", m_func);
			g_IndentLevel++;
		}
		~ScopedLog() {
			g_IndentLevel--;
			LOG_I("<< %s\n", m_func);
		}

	private:
		const char* m_func;
	};

	class ScopedPauseLog {
	public:
		ScopedPauseLog(bool* pauseLog) : m_pPauseLog(pauseLog), m_prevVal(false) {
			if (m_pPauseLog) {
				m_prevVal = *m_pPauseLog;
				*m_pPauseLog = true;
			}
		}

		~ScopedPauseLog() {
			if (m_pPauseLog) {
				*m_pPauseLog = m_prevVal;
			}
		}

	private:
		bool m_prevVal;
		bool* m_pPauseLog;
	};

	#define LOG_SCOPE() logging::ScopedLog _scope(__func__)
	#define LOG_PAUSE_SCOPED() logging::ScopedPauseLog _pauseLog(&logging::g_pauseLog)
	#define LOG_PAUSE() logging::g_pauseLog = true
	#define LOG_UNPAUSE() logging::g_pauseLog = false

	// should always add at beginning or end to not mess up access modifiers
	#define ADD_CLASS_TAG(defaultTag)							\
		public: static constexpr const char* DEFAULT_TAG = defaultTag; \
		protected: const char* m_tag = DEFAULT_TAG;				\
		public:	void SetTag(const char* tag) { m_tag = tag; }
}
