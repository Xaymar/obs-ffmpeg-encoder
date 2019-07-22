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

// Codec: H264
#define P_H264 "Codec.H264"
#define P_H264_PROFILE "Codec.H264.Profile"
#define P_H264_LEVEL "Codec.H264.Level"

namespace obsffmpeg {
	namespace codecs {
		namespace h264 {
			enum class profile {
				CONSTRAINED_BASELINE,
				BASELINE,
				MAIN,
				HIGH,
				HIGH444_PREDICTIVE,
				UNKNOWN = -1,
			};

			enum class level {
				L1_0 = 10,
				L1_0b,
				L1_1,
				L1_2,
				L1_3,
				L2_0 = 20,
				L2_1,
				L2_2,
				L3_0 = 30,
				L3_1,
				L3_2,
				L4_0 = 40,
				L4_1,
				L4_2,
				L5_0 = 50,
				L5_1,
				L5_2,
				L6_0 = 60,
				L6_1,
				L6_2,
				UNKNOWN = -1,
			};
		} // namespace h264
	}         // namespace codecs
} // namespace obsffmpeg
