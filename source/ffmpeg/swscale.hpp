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

#ifndef OBS_FFMPEG_FFMPEG_SWSCALE
#define OBS_FFMPEG_FFMPEG_SWSCALE
#pragma once

#include <cinttypes>
#include <utility>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

namespace ffmpeg {
	class swscale {
		std::pair<uint32_t, uint32_t> source_size;
		AVPixelFormat                 source_format     = AV_PIX_FMT_NONE;
		bool                          source_full_range = false;
		AVColorSpace                  source_colorspace = AVCOL_SPC_UNSPECIFIED;

		std::pair<uint32_t, uint32_t> target_size;
		AVPixelFormat                 target_format     = AV_PIX_FMT_NONE;
		bool                          target_full_range = false;
		AVColorSpace                  target_colorspace = AVCOL_SPC_UNSPECIFIED;

		SwsContext* context = nullptr;

		public:
		swscale();
		~swscale();

		void                          set_source_size(uint32_t width, uint32_t height);
		void                          get_source_size(uint32_t& width, uint32_t& height);
		std::pair<uint32_t, uint32_t> get_source_size();
		uint32_t                      get_source_width();
		uint32_t                      get_source_height();
		void                          set_source_format(AVPixelFormat format);
		AVPixelFormat                 get_source_format();
		void                          set_source_color(bool full_range, AVColorSpace space);
		void                          set_source_colorspace(AVColorSpace space);
		AVColorSpace                  get_source_colorspace();
		void                          set_source_full_range(bool full_range);
		bool                          is_source_full_range();

		void                          set_target_size(uint32_t width, uint32_t height);
		void                          get_target_size(uint32_t& width, uint32_t& height);
		std::pair<uint32_t, uint32_t> get_target_size();
		uint32_t                      get_target_width();
		uint32_t                      get_target_height();
		void                          set_target_format(AVPixelFormat format);
		AVPixelFormat                 get_target_format();
		void                          set_target_color(bool full_range, AVColorSpace space);
		void                          set_target_colorspace(AVColorSpace space);
		AVColorSpace                  get_target_colorspace();
		void                          set_target_full_range(bool full_range);
		bool                          is_target_full_range();

		bool initialize(int flags);
		bool finalize();

		int32_t convert(const uint8_t* const source_data[], const int source_stride[], int32_t source_row,
		                int32_t source_rows, uint8_t* const target_data[], const int target_stride[]);
	};
} // namespace ffmpeg

#endif OBS_FFMPEG_FFMPEG_SWSCALE
