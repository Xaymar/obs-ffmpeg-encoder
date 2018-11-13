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

#ifndef OBS_FFMPEG_FFMPEG_AVFRAME_QUEUE
#define OBS_FFMPEG_FFMPEG_AVFRAME_QUEUE
#pragma once

#include <mutex>
#include <deque>

extern "C" {
#include <libavutil/frame.h>
}

namespace ffmpeg {
	class avframe_queue {
		std::deque<AVFrame*> frames;
		std::mutex           lock;

		std::pair<uint32_t, uint32_t> resolution;
		AVPixelFormat                 format = AV_PIX_FMT_NONE;

		AVFrame* create_frame();
		void     destroy_frame(AVFrame* frame);

		public:
		avframe_queue();
		~avframe_queue();

		void     set_resolution(uint32_t width, uint32_t height);
		void     get_resolution(uint32_t& width, uint32_t& height);
		uint32_t get_width();
		uint32_t get_height();

		void          set_pixel_format(AVPixelFormat format);
		AVPixelFormat get_pixel_format();

		void precache(size_t count);

		void clear();

		void push(AVFrame* frame);

		AVFrame* pop();

		AVFrame* pop_only();

		bool empty();

		size_t size();


	};
} // namespace ffmpeg

#endif OBS_FFMPEG_FFMPEG_AVFRAME_QUEUE
