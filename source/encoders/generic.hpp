// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (C) 2018 - 2019 Michael Fabian Dirks
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

#include <encoder.hpp>
#include "ffmpeg/avframe-queue.hpp"
#include "ffmpeg/swscale.hpp"

namespace encoder {
	class generic_factory {
		struct info {
			std::string      uid;
			std::string      codec;
			std::string      readable_name;
			obs_encoder_info oei;
		} info;
		AVCodec* avcodec_ptr;

		public:
		generic_factory(AVCodec* codec);
		virtual ~generic_factory();

		void register_encoder();

		const char* get_name();

		void get_defaults(obs_data_t* settings);

		void get_properties(obs_properties_t* props);

		AVCodec* get_avcodec();

		public:
		static bool modified_ratecontrol_properties(void* priv, obs_properties_t* props, obs_property_t* prop,
		                                            obs_data_t* settings);
		static bool modified_threading_properties(void* priv, obs_properties_t* props, obs_property_t* prop,
		                                          obs_data_t* settings);
	};

	class generic {
		obs_encoder_t*   self;
		generic_factory* factory;

		AVCodec*        codec;
		AVCodecContext* context;

		ffmpeg::avframe_queue frame_queue;
		ffmpeg::avframe_queue frame_queue_used;
		ffmpeg::swscale       swscale;
		AVPacket*             current_packet = nullptr;

		public:
		generic(obs_data_t* settings, obs_encoder_t* encoder);
		virtual ~generic();

		// Shared

		void get_properties(obs_properties_t* props);

		bool update(obs_data_t* settings);

		bool encode(struct encoder_frame* frame, struct encoder_packet* packet, bool* received_packet);

		// Audio only
		void get_audio_info(struct audio_convert_info* info);

		size_t get_frame_size();

		// Video only
		void get_video_info(struct video_scale_info* info);

		bool get_sei_data(uint8_t** sei_data, size_t* size);

		bool get_extra_data(uint8_t** extra_data, size_t* size);

		// GPU Video only
		bool encode_texture(uint32_t handle, int64_t pts, uint64_t lock_key, uint64_t* next_key,
		                    struct encoder_packet* packet, bool* received_packet);
	};
} // namespace encoder
