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

#include <condition_variable>
#include <encoder.hpp>
#include <mutex>
#include <thread>
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
	};

	class generic {
		obs_encoder_t*   self;
		generic_factory* factory;

		AVCodec*        codec;
		AVCodecContext* context;

		ffmpeg::avframe_queue frame_queue;
		ffmpeg::avframe_queue frame_queue_used;
		ffmpeg::swscale       swscale;
		AVPacket              current_packet;

		int64_t lag_in_frames;
		int64_t frame_count;

		public:
		generic(obs_data_t* settings, obs_encoder_t* encoder);
		virtual ~generic();

		public: // OBS API
		// Shared
		void get_properties(obs_properties_t* props);

		bool update(obs_data_t* settings);

		// Audio only
		void get_audio_info(struct audio_convert_info* info);

		size_t get_frame_size();

		bool audio_encode(struct encoder_frame* frame, struct encoder_packet* packet, bool* received_packet);

		// Video only
		void get_video_info(struct video_scale_info* info);

		bool get_sei_data(uint8_t** sei_data, size_t* size);

		bool get_extra_data(uint8_t** extra_data, size_t* size);

		bool video_encode(struct encoder_frame* frame, struct encoder_packet* packet, bool* received_packet);

		bool video_encode_texture(uint32_t handle, int64_t pts, uint64_t lock_key, uint64_t* next_key,
		                          struct encoder_packet* packet, bool* received_packet);

		int receive_packet(bool* received_packet, struct encoder_packet* packet);

		int send_frame(std::shared_ptr<AVFrame> frame);
	};
} // namespace encoder
