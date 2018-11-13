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

#ifndef OBS_FFMPEG_ENCODER_PRORES_AW
#define OBS_FFMPEG_ENCODER_PRORES_AW
#pragma once

#include <encoder.hpp>
#include "ffmpeg/avframe-queue.hpp"
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace obsffmpeg {
	namespace encoder {
		class prores_aw : base {
			enum class profile {
				Auto             = FF_PROFILE_UNKNOWN,
				Proxy            = 0 /*FF_PROFILE_PRORES_PROXY*/,
				Light            = 1 /*FF_PROFILE_PRORES_LT*/,
				Standard         = 2 /*FF_PROFILE_PRORES_STANDARD*/,
				HighQuality      = 3 /*FF_PROFILE_PRORES_HQ*/,
				FourFourFourFour = 4 /*FF_PROFILE_PRORES_4444*/ // Automatically set if I444 or RGB input.
			};

			private:
			profile video_profile = profile::Auto;
			ffmpeg::avframe_queue frame_queue;
			ffmpeg::avframe_queue frame_queue_used;
			AVPacket* current_packet = nullptr;

			

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

#endif OBS_FFMPEG_ENCODER_PRORES_AW
