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

#ifndef OBS_FFMPEG_UTILITY_HPP
#define OBS_FFMPEG_UTILITY_HPP
#pragma once

#include "version.hpp"

// Logging
#define PLOG(level, ...) blog(level, "["##PROJECT_NAME##"] " __VA_ARGS__);
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
#define vstr(s) dstr(s)
#define dstr(s) #s

#endif OBS_FFMPEG_UTILITY_HPP
