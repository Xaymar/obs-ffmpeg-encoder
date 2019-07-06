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

#ifndef OBS_FFMPEG_FFMPEG_UTILITY
#define OBS_FFMPEG_FFMPEG_UTILITY
#pragma once

#include <obs.h>
#include <string>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace ffmpeg {
	namespace tools {
		std::string translate_encoder_capabilities(int capabilities);

		const char* get_pixel_format_name(AVPixelFormat v);

		const char* get_color_space_name(AVColorSpace v);

		const char* get_error_description(int error);

		AVPixelFormat obs_videoformat_to_avpixelformat(video_format v);

		AVColorSpace obs_videocolorspace_to_avcolorspace(video_colorspace v);

		AVColorRange obs_videorangetype_to_avcolorrange(video_range_type v);
	} // namespace tools
} // namespace ffmpeg

#endif OBS_FFMPEG_FFMPEG_UTILITY
