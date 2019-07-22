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
#include <map>
#include "utility.hpp"

extern "C" {
#include <obs-properties.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#pragma warning(pop)
}

namespace obsffmpeg {
	namespace nvenc {
		enum class preset : int64_t {
			DEFAULT,
			SLOW,
			MEDIUM,
			FAST,
			HIGH_PERFORMANCE,
			HIGH_QUALITY,
			BLURAYDISC,
			LOW_LATENCY,
			LOW_LATENCY_HIGH_PERFORMANCE,
			LOW_LATENCY_HIGH_QUALITY,
			LOSSLESS,
			LOSSLESS_HIGH_PERFORMANCE,
		};

		enum class ratecontrolmode : int64_t {
			CQP,
			VBR,
			VBR_HQ,
			CBR,
			CBR_HQ,
			CBR_LD_HQ,
		};

		enum class b_ref_mode : int64_t {
			DISABLED,
			EACH,
			MIDDLE,
		};

		extern std::map<preset, std::string> presets;

		extern std::map<preset, std::string> preset_to_opt;

		extern std::map<ratecontrolmode, std::string> ratecontrolmodes;

		extern std::map<ratecontrolmode, std::string> ratecontrolmode_to_opt;

		extern std::map<b_ref_mode, std::string> b_ref_modes;

		extern std::map<b_ref_mode, std::string> b_ref_mode_to_opt;

		void get_defaults(obs_data_t* settings, AVCodec* codec, AVCodecContext* context);

		void get_properties_pre(obs_properties_t* props, AVCodec* codec);

		void get_properties_post(obs_properties_t* props, AVCodec* codec);

		void get_runtime_properties(obs_properties_t* props, AVCodec* codec, AVCodecContext* context);

		void update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context);
	} // namespace nvenc
} // namespace obsffmpeg
