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

#ifndef OBS_FFMPEG_FFMPEG_UTILITY
#define OBS_FFMPEG_FFMPEG_UTILITY
#pragma once

#include <obs.h>
#include <string>
#include <vector>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
}

namespace ffmpeg {
	namespace tools {
		std::string translate_encoder_capabilities(int capabilities);

		const char* get_pixel_format_name(AVPixelFormat v);

		const char* get_color_space_name(AVColorSpace v);

		const char* get_error_description(int error);

		AVPixelFormat obs_videoformat_to_avpixelformat(video_format v);

		video_format avpixelformat_to_obs_videoformat(AVPixelFormat v);

		AVPixelFormat get_least_lossy_format(const AVPixelFormat* haystack, AVPixelFormat needle);

		AVColorSpace obs_videocolorspace_to_avcolorspace(video_colorspace v);

		AVColorRange obs_videorangetype_to_avcolorrange(video_range_type v);

		bool can_hardware_encode(const AVCodec* codec);

		std::vector<AVPixelFormat> get_software_formats(const AVPixelFormat* list);

		AVPixelFormat get_best_compatible_format(const AVPixelFormat* list, AVPixelFormat source);
	} // namespace tools
} // namespace ffmpeg

#endif OBS_FFMPEG_FFMPEG_UTILITY
