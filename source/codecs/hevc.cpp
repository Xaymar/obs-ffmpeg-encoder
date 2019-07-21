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

#include "hevc.hpp"

using namespace obsffmpeg::codecs::hevc;

std::map<profile, std::string> obsffmpeg::codecs::hevc::profiles{
    {profile::MAIN, "main"},
    {profile::MAIN10, "main10"},
    {profile::RANGE_EXTENDED, "rext"},
};

std::map<tier, std::string> obsffmpeg::codecs::hevc::profile_tiers{
    {tier::MAIN, "main"},
    {tier::HIGH, "high"},
};

std::map<level, std::string> obsffmpeg::codecs::hevc::levels{
    {level::L1_0, "1.0"}, {level::L2_0, "2.0"}, {level::L3_0, "3.0"}, {level::L3_1, "3.1"},
    {level::L4_0, "4.0"}, {level::L4_1, "4.1"}, {level::L5_0, "5.0"}, {level::L5_1, "5.1"},
    {level::L5_2, "5.2"}, {level::L6_0, "6.0"}, {level::L6_1, "6.1"}, {level::L6_2, "6.2"},
};
