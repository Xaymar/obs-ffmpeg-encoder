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

#include "tools.hpp"
#include <stdexcept>

extern "C" {
#include <libavutil/error.h>
#include <libavutil/pixdesc.h>
}

const char* ffmpeg::tools::get_pixel_format_name(AVPixelFormat v)
{
	return av_get_pix_fmt_name(v);
}

const char* ffmpeg::tools::get_color_space_name(AVColorSpace v)
{
	switch (v) {
	case AVCOL_SPC_RGB:
		return "RGB";
	case AVCOL_SPC_BT709:
		return "BT.709";
	case AVCOL_SPC_FCC:
		return "FCC Title 47 CoFR 73.682 (a)(20)";
	case AVCOL_SPC_BT470BG:
		return "BT.601 625";
	case AVCOL_SPC_SMPTE170M:
	case AVCOL_SPC_SMPTE240M:
		return "BT.601 525";
	case AVCOL_SPC_YCGCO:
		return "ITU-T SG16";
	case AVCOL_SPC_BT2020_NCL:
		return "BT.2020 NCL";
	case AVCOL_SPC_BT2020_CL:
		return "BT.2020 CL";
	case AVCOL_SPC_SMPTE2085:
		return "SMPTE 2085";
	case AVCOL_SPC_CHROMA_DERIVED_NCL:
		return "Chroma NCL";
	case AVCOL_SPC_CHROMA_DERIVED_CL:
		return "Chroma CL";
	case AVCOL_SPC_ICTCP:
		return "BT.2100";
	case AVCOL_SPC_NB:
		return "Not Part of ABI";
	}
	return "Unknown";
}

const char* ffmpeg::tools::get_error_description(int error)
{
	switch (error) {
	case AVERROR(EPERM):
		return "Permission Denied";
	case AVERROR(ENOMEM):
		return "Out Of Memory";
	case AVERROR(EINVAL):
		return "Invalid Value for Parameter";
	}
	return "Not Translated Yet";
}

AVPixelFormat ffmpeg::tools::obs_videoformat_to_avpixelformat(video_format v)
{
	switch (v) {
		// 32-Bits
	case VIDEO_FORMAT_BGRX:
		return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_BGRA:
		return AV_PIX_FMT_BGRA;
	case VIDEO_FORMAT_RGBA:
		return AV_PIX_FMT_RGBA;
	case VIDEO_FORMAT_I444:
		return AV_PIX_FMT_YUV444P;
	case VIDEO_FORMAT_YUY2:
		return AV_PIX_FMT_YUYV422;
	case VIDEO_FORMAT_YVYU:
		return AV_PIX_FMT_YVYU422;
	case VIDEO_FORMAT_UYVY:
		return AV_PIX_FMT_UYVY422;
	case VIDEO_FORMAT_I420:
		return AV_PIX_FMT_YUV420P;
	case VIDEO_FORMAT_NV12:
		return AV_PIX_FMT_NV12;
	}
	throw std::invalid_argument("unknown format");
}

AVColorSpace ffmpeg::tools::obs_videocolorspace_to_avcolorspace(video_colorspace v)
{
	switch (v) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		return AVCOL_SPC_BT709;
	case VIDEO_CS_601:
		return AVCOL_SPC_SMPTE170M;
	}
	throw std::invalid_argument("unknown color space");
}

AVColorRange ffmpeg::tools::obs_videorangetype_to_avcolorrange(video_range_type v)
{
	switch (v) {
	case VIDEO_RANGE_DEFAULT:
	case VIDEO_RANGE_FULL:
		return AVCOL_RANGE_JPEG;
	case VIDEO_RANGE_PARTIAL:
		return AVCOL_RANGE_MPEG;
	}
	throw std::invalid_argument("unknown range");
}
