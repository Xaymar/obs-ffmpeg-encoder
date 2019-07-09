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
#include "utility.hpp"

#define P_RATECONTROL "RateControl"
#define P_RATECONTROL_METHOD "RateControl.Method"
#define P_RATECONTROL_METHOD_(x) "RateControl.Method." D_VSTR(x)
#define P_RATECONTROL_BITRATE_TARGET "RateControl.Bitrate.Target"
#define P_RATECONTROL_BITRATE_MINIMUM "RateControl.Bitrate.Minimum"
#define P_RATECONTROL_BITRATE_MAXIMUM "RateControl.Bitrate.Maximum"
#define P_RATECONTROL_BUFFERSIZE "RateControl.BufferSize"
