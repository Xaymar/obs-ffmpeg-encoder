// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (C) 2018 - 2018 Michael Fabian Dirks
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#pragma once

#include "version.hpp"

extern "C" {
#include <obs-config.h>
#include <obs.h>
}

// Logging
#define PLOG(level, ...) blog(level, "[obs-ffmpeg-encoder] " __VA_ARGS__);
#define PLOG_ERROR(...) PLOG(LOG_ERROR, __VA_ARGS__)
#define PLOG_WARNING(...) PLOG(LOG_WARNING, __VA_ARGS__)
#define PLOG_INFO(...) PLOG(LOG_INFO, __VA_ARGS__)
#define PLOG_DEBUG(...) PLOG(LOG_DEBUG, __VA_ARGS__)

// Function Name
#ifndef __FUNCTION_NAME__
#if defined(_WIN32) || defined(_WIN64) //WINDOWS
#define __FUNCTION_NAME__ __FUNCTION__
#else //*NIX
#define __FUNCTION_NAME__ __func__
#endif
#endif

// I18n
#define TRANSLATE(x) obs_module_text(x)
#define DESC(x) x ".Description"

// Other
#define D_STR(s) #s
#define D_VSTR(s) D_STR(s)

// Initializer
#ifdef __cplusplus
#define INITIALIZER(f) \
	static void f(void); \
	struct f##_t_ { \
		f##_t_(void) \
		{ \
			f(); \
		} \
	}; \
	static f##_t_ f##_; \
	static void   f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
#define INITIALIZER2_(f, p) \
	static void f(void); \
	__declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
	__pragma(comment(linker, "/include:" p #f "_")) static void f(void)
#ifdef _WIN64
#define INITIALIZER(f) INITIALIZER2_(f, "")
#else
#define INITIALIZER(f) INITIALIZER2_(f, "_")
#endif
#else
#define INITIALIZER(f) \
	static void f(void) __attribute__((constructor)); \
	static void f(void)
#endif

// Helpers
namespace obsffmpeg {
	bool inline are_property_groups_broken()
	{
		return obs_get_version() < MAKE_SEMANTIC_VERSION(24, 0, 0);
	}
} // namespace obsffmpeg
