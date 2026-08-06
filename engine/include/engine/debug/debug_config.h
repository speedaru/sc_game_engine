#pragma once

// master switch for all debug tooling. release builds compile the entire debug/
// subsystem - modules, overlay rendering, imgui - down to nothing.
// define it yourself before including anything from debug/ to override
#ifndef SC_ENABLE_DEBUG_TOOLS
	#ifdef _DEBUG
		#define SC_ENABLE_DEBUG_TOOLS 1
	#else
		#define SC_ENABLE_DEBUG_TOOLS 0
	#endif
#endif

// wraps call sites that should only exist in debug builds:
//     SC_DEBUG_ONLY(sc::debug::Register<MyModule>(m_registry));
//
// every debug/ header guards its own declarations, so calling into the subsystem
// without this wrapper is a compile error in release rather than a silent no-op.
// that's deliberate: a debug feature that quietly stops existing is worse than one
// that tells you where it was used
#if SC_ENABLE_DEBUG_TOOLS
	#define SC_DEBUG_ONLY(...) __VA_ARGS__
#else
	#define SC_DEBUG_ONLY(...)
#endif

// definition of debug namespace to not crash release build
namespace sc::debug {}
