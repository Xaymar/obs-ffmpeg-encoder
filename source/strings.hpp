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

#define P_STATE_DEFAULT "State.Default"
#define P_STATE_DISABLED "State.Disabled"
#define P_STATE_ENABLED "State.Enabled"
#define P_STATE_AUTOMATIC "State.Automatic"

#define P_RATECONTROL "RateControl"
#define P_RATECONTROL_METHOD "RateControl.Method"
#define P_RATECONTROL_METHOD_(x) "RateControl.Method." D_VSTR(x)
#define P_RATECONTROL_BITRATE_TARGET "RateControl.Bitrate.Target"
#define P_RATECONTROL_BITRATE_MINIMUM "RateControl.Bitrate.Minimum"
#define P_RATECONTROL_BITRATE_MAXIMUM "RateControl.Bitrate.Maximum"
#define P_RATECONTROL_BUFFERSIZE "RateControl.BufferSize"
#define P_RATECONTROL_QUALITY_TARGET "RateControl.Quality.Target"
#define P_RATECONTROL_QUALITY_MINIMUM "RateControl.Quality.Minimum"
#define P_RATECONTROL_QUALITY_MAXIMUM "RateControl.Quality.Maximum"
#define P_RATECONTROL_QP_I "RateControl.QP.I"
#define P_RATECONTROL_QP_P "RateControl.QP.P"
#define P_RATECONTROL_QP_B "RateControl.QP.B"
#define P_RATECONTROL_QP_I_INITIAL "RateControl.QP.I.Initial"
#define P_RATECONTROL_QP_P_INITIAL "RateControl.QP.P.Initial"
#define P_RATECONTROL_QP_B_INITIAL "RateControl.QP.B.Initial"

#define P_KEYFRAMES "KeyFrames"
#define P_KEYFRAMES_INTERVALTYPE "KeyFrames.IntervalType"
#define P_KEYFRAMES_INTERVALTYPE_(x) "KeyFrames.IntervalType." D_VSTR(x)
#define P_KEYFRAMES_INTERVAL "KeyFrames.Interval"
#define P_KEYFRAMES_INTERVAL_SECONDS "KeyFrames.Interval.Seconds"
#define P_KEYFRAMES_INTERVAL_FRAMES "KeyFrames.Interval.Frames"
