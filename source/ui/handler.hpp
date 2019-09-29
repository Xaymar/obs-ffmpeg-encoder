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

#include <string>
#include "hwapi/base.hpp"

extern "C" {
#include <obs.h>

#include <obs-data.h>
#include <obs-encoder.h>
#include <obs-properties.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#pragma warning(pop)
}

namespace obsffmpeg {
	namespace ui {
		class handler {
			public:
			virtual void override_visible_name(const AVCodec* codec, std::string& name);

			virtual void override_info(obs_encoder_info* main, obs_encoder_info* fallback);

			virtual void override_colorformat(AVPixelFormat& target_format, obs_data_t* settings,
			                                  const AVCodec* codec, AVCodecContext* context);

			virtual void get_defaults(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context);

			virtual void get_properties(obs_properties_t* props, const AVCodec* codec,
			                            AVCodecContext* context);

			virtual obsffmpeg::hwapi::device find_hw_device(std::shared_ptr<obsffmpeg::hwapi::base> api,
			                                                const AVCodec* codec, AVCodecContext* context);

			virtual void update(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context);

			virtual void log_options(obs_data_t* settings, const AVCodec* codec, AVCodecContext* context);

			virtual void import_from_ffmpeg(const std::string ffmpeg, obs_data_t* settings,
			                                const AVCodec* codec, AVCodecContext* context);

			virtual std::string export_for_ffmpeg(obs_data_t* settings, const AVCodec* codec,
			                                      AVCodecContext* context);

			virtual void process_avpacket(AVPacket& packet, const AVCodec* codec, AVCodecContext* context);
		};
	} // namespace ui
} // namespace obsffmpeg
