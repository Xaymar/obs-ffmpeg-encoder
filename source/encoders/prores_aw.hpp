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

#include <encoder.hpp>
#include <vector>
#include "ffmpeg/avframe-queue.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace obsffmpeg {
	namespace encoder {
		class prores_aw : base {
			enum class profile {
				Auto        = FF_PROFILE_UNKNOWN,
				Proxy       = 0 /*FF_PROFILE_PRORES_PROXY*/,
				Light       = 1 /*FF_PROFILE_PRORES_LT*/,
				Standard    = 2 /*FF_PROFILE_PRORES_STANDARD*/,
				HighQuality = 3 /*FF_PROFILE_PRORES_HQ*/,
				FourFourFourFour =
				    4 /*FF_PROFILE_PRORES_4444*/ // Automatically set if I444 or RGB input.
			};

			private:
			profile               video_profile = profile::Auto;
			ffmpeg::avframe_queue frame_queue;
			ffmpeg::avframe_queue frame_queue_used;
			AVPacket*             current_packet = nullptr;

			public:
			prores_aw(obs_data_t* settings, obs_encoder_t* encoder);

			~prores_aw();

			virtual void get_properties(obs_properties_t* props) override;

			virtual bool update(obs_data_t* settings) override;

			virtual bool get_extra_data(uint8_t** extra_data, size_t* size) override;

			virtual bool get_sei_data(uint8_t** sei_data, size_t* size) override;

			virtual void get_video_info(video_scale_info* info) override;

			virtual bool encode(encoder_frame* frame, encoder_packet* packet,
			                    bool* received_packet) override;

			public:
			static void initialize();

			static void finalize();

			static void get_defaults(obs_data_t* settings);

			static obs_properties_t* get_properties();
		};
	} // namespace encoder
} // namespace obsffmpeg
