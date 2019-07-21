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

// Codec: HEVC
#define P_HEVC "Codec.HEVC"
#define P_HEVC_PROFILE "Codec.HEVC.Profile"
#define P_HEVC_TIER "Codec.HEVC.Tier"
#define P_HEVC_LEVEL "Codec.HEVC.Level"

namespace obsffmpeg {
	namespace codecs {
		namespace hevc {
			enum class profile {
				MAIN,
				MAIN10,
				RANGE_EXTENDED,
				UNKNOWN = -1,
			};

			enum class tier {
				MAIN,
				HIGH,
				UNKNOWN = -1,
			};

			enum class level {
				L1_0    = 30,
				L2_0    = 60,
				L3_0    = 90,
				L3_1    = 93,
				L4_0    = 120,
				L4_1    = 123,
				L5_0    = 150,
				L5_1    = 153,
				L5_2    = 156,
				L6_0    = 180,
				L6_1    = 183,
				L6_2    = 186,
				UNKNOWN = -1,
			};

			extern std::map<profile, std::string> profiles;

			extern std::map<tier, std::string> profile_tiers;

			extern std::map<level, std::string> levels;

		} // namespace hevc
	}         // namespace codecs
} // namespace obsffmpeg
