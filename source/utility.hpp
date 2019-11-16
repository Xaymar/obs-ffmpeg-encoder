// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (c) 2019 Michael Fabian Dirks <info@xaymar.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

	struct obs_graphics {
		obs_graphics()
		{
			obs_enter_graphics();
		}
		~obs_graphics()
		{
			obs_leave_graphics();
		}
	};

	obs_property_t* obs_properties_add_tristate(obs_properties_t* props, const char* name, const char* desc);

	inline bool is_tristate_enabled(int64_t tristate) {
		return tristate == 1;
	}

	inline bool is_tristate_disabled(int64_t tristate) {
		return tristate == 0;
	}

	inline bool is_tristate_default(int64_t tristate) {
		return tristate == -1;
	}
} // namespace obsffmpeg
