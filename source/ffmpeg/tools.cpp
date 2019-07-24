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

#include "tools.hpp"
#include <sstream>
#include <stdexcept>

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/pixdesc.h>
#pragma warning(pop)
}

std::string ffmpeg::tools::translate_encoder_capabilities(int capabilities)
{
	// Sorted by relative importance.
	std::pair<int, std::string> caps[] = {
	    {AV_CODEC_CAP_EXPERIMENTAL, "Experimental"},

	    // Quality
	    {AV_CODEC_CAP_LOSSLESS, "Lossless"},
	    //{AV_CODEC_CAP_INTRA_ONLY, "I-Frames only"},

	    // Threading
	    //{AV_CODEC_CAP_FRAME_THREADS, "Frame-Threading"},
	    //{AV_CODEC_CAP_SLICE_THREADS, "Slice-Threading"},
	    //{AV_CODEC_CAP_AUTO_THREADS, "Automatic-Threading"},

	    // Features
	    {AV_CODEC_CAP_PARAM_CHANGE, "Dynamic Parameter Change"},
	    {AV_CODEC_CAP_SUBFRAMES, "Sub-Frames"},
	    {AV_CODEC_CAP_VARIABLE_FRAME_SIZE, "Variable Frame Size"},
	    {AV_CODEC_CAP_SMALL_LAST_FRAME, "Small Final Frame"},
	    //{AV_CODEC_CAP_DR1, "Uses get_buffer"},
	    //{AV_CODEC_CAP_DELAY, "Requires Flush"},

	    // Other
	    {AV_CODEC_CAP_TRUNCATED, "Truncated"},
	    {AV_CODEC_CAP_CHANNEL_CONF, "AV_CODEC_CAP_CHANNEL_CONF"},
	    {AV_CODEC_CAP_DRAW_HORIZ_BAND, "AV_CODEC_CAP_DRAW_HORIZ_BAND"},
	    {AV_CODEC_CAP_HWACCEL_VDPAU, "AV_CODEC_CAP_HWACCEL_VDPAU"},
	    {AV_CODEC_CAP_AVOID_PROBING, "AV_CODEC_CAP_AVOID_PROBING"},
	};

	std::stringstream sstr;
	for (auto const kv : caps) {
		if (capabilities & kv.first) {
			capabilities &= ~kv.first;
			sstr << kv.second;
			if (capabilities != 0) {
				sstr << ", ";
			} else {
				break;
			}
		}
	}

	return sstr.str();
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
		//	case AVERROR(ENOENT):
		//	case AVERROR(ESRCH):
		//	case AVERROR(EINTR):
		//	case AVERROR(EIO):
		//	case AVERROR(ENXIO):
		//	case AVERROR(E2BIG):
		//	case AVERROR(ENOEXEC):
		//	case AVERROR(EBADF):
		//	case AVERROR(ECHILD):
		//	case AVERROR(EAGAIN):
	case AVERROR(ENOMEM):
		return "Out Of Memory";
		//	case AVERROR(EACCES):
		//	case AVERROR(EFAULT):
		//	case AVERROR(EBUSY):
		//	case AVERROR(EEXIST):
		//	case AVERROR(EXDEV):
		//	case AVERROR(ENODEV):
		//	case AVERROR(ENOTDIR):
		//	case AVERROR(EISDIR):
		//	case AVERROR(ENFILE):
		//	case AVERROR(EMFILE):
		//	case AVERROR(ENOTTY):
		//	case AVERROR(EFBIG):
		//	case AVERROR(ENOSPC):
		//	case AVERROR(ESPIPE):
		//	case AVERROR(EROFS):
		//	case AVERROR(EMLINK):
		//	case AVERROR(EPIPE):
		//	case AVERROR(EDOM):
		//	case AVERROR(EDEADLK):
		//	case AVERROR(ENAMETOOLONG):
		//	case AVERROR(ENOLCK):
		//	case AVERROR(ENOSYS):
		//	case AVERROR(ENOTEMPTY):
	case AVERROR(EINVAL):
		return "Invalid Value(s)";
	case AVERROR(ERANGE):
		return "Out of Range";
		//	case AVERROR(EILSEQ):
		//	case AVERROR(STRUNCATE):
	}
	return "Not Translated Yet";
}

AVPixelFormat ffmpeg::tools::obs_videoformat_to_avpixelformat(video_format v)
{
	switch (v) {
		// 32-Bits
	case VIDEO_FORMAT_BGRX:
		return AV_PIX_FMT_BGR0;
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

video_format ffmpeg::tools::avpixelformat_to_obs_videoformat(AVPixelFormat v)
{
	switch (v) {
	case AV_PIX_FMT_YUV420P:
		return VIDEO_FORMAT_I420;
	case AV_PIX_FMT_NV12:
		return VIDEO_FORMAT_NV12;
	case AV_PIX_FMT_YVYU422:
		return VIDEO_FORMAT_YVYU;
	case AV_PIX_FMT_YUYV422:
		return VIDEO_FORMAT_YUY2;
	case AV_PIX_FMT_UYVY422:
		return VIDEO_FORMAT_UYVY;
	case AV_PIX_FMT_RGBA:
		return VIDEO_FORMAT_RGBA;
	case AV_PIX_FMT_BGRA:
		return VIDEO_FORMAT_BGRA;
	case AV_PIX_FMT_BGR0:
		return VIDEO_FORMAT_BGRX;
	case AV_PIX_FMT_GRAY8:
		return VIDEO_FORMAT_Y800;
	case AV_PIX_FMT_YUV444P:
		return VIDEO_FORMAT_I444;
	}
	return VIDEO_FORMAT_NONE;
}

AVColorSpace ffmpeg::tools::obs_videocolorspace_to_avcolorspace(video_colorspace v)
{
	switch (v) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		return AVCOL_SPC_BT709;
	case VIDEO_CS_601:
		return AVCOL_SPC_BT470BG;
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
