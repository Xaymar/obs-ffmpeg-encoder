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
