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
#include <list>
#include <map>
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

	    // Features
	    {AV_CODEC_CAP_PARAM_CHANGE, "Dynamic Parameter Change"},
	    {AV_CODEC_CAP_SUBFRAMES, "Sub-Frames"},
	    {AV_CODEC_CAP_VARIABLE_FRAME_SIZE, "Variable Frame Size"},
	    {AV_CODEC_CAP_SMALL_LAST_FRAME, "Small Final Frame"},

	    // Other
	    {AV_CODEC_CAP_TRUNCATED, "Truncated"},
	    {AV_CODEC_CAP_CHANNEL_CONF, "AV_CODEC_CAP_CHANNEL_CONF"},
	    {AV_CODEC_CAP_DRAW_HORIZ_BAND, "AV_CODEC_CAP_DRAW_HORIZ_BAND"},
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

static std::map<video_format, AVPixelFormat> obs_to_av_format_map = {
    {VIDEO_FORMAT_I420, AV_PIX_FMT_YUV420P},  // YUV 4:2:0
    {VIDEO_FORMAT_NV12, AV_PIX_FMT_NV12},     // NV12 Packed YUV
    {VIDEO_FORMAT_YVYU, AV_PIX_FMT_YVYU422},  // YVYU Packed YUV
    {VIDEO_FORMAT_YUY2, AV_PIX_FMT_YUYV422},  // YUYV Packed YUV
    {VIDEO_FORMAT_UYVY, AV_PIX_FMT_UYVY422},  // UYVY Packed YUV
    {VIDEO_FORMAT_RGBA, AV_PIX_FMT_RGBA},     //
    {VIDEO_FORMAT_BGRA, AV_PIX_FMT_BGRA},     //
    {VIDEO_FORMAT_BGRX, AV_PIX_FMT_BGR0},     //
    {VIDEO_FORMAT_Y800, AV_PIX_FMT_GRAY8},    //
    {VIDEO_FORMAT_I444, AV_PIX_FMT_YUV444P},  //
    {VIDEO_FORMAT_BGR3, AV_PIX_FMT_BGR24},    //
    {VIDEO_FORMAT_I422, AV_PIX_FMT_YUV422P},  //
    {VIDEO_FORMAT_I40A, AV_PIX_FMT_YUVA420P}, //
    {VIDEO_FORMAT_I42A, AV_PIX_FMT_YUVA422P}, //
    {VIDEO_FORMAT_YUVA, AV_PIX_FMT_YUVA444P}, //
                                              //{VIDEO_FORMAT_AYUV, AV_PIX_FMT_AYUV444P}, //
};

AVPixelFormat ffmpeg::tools::obs_videoformat_to_avpixelformat(video_format v)
{
	auto found = obs_to_av_format_map.find(v);
	if (found != obs_to_av_format_map.end()) {
		return found->second;
	}
	return AV_PIX_FMT_NONE;
}

video_format ffmpeg::tools::avpixelformat_to_obs_videoformat(AVPixelFormat v)
{
	for (const auto& kv : obs_to_av_format_map) {
		if (kv.second == v)
			return kv.first;
	}
	return VIDEO_FORMAT_NONE;
}

AVPixelFormat ffmpeg::tools::get_least_lossy_format(const AVPixelFormat* haystack, AVPixelFormat needle)
{
	int data_loss = 0;
	return avcodec_find_best_pix_fmt_of_list(haystack, needle, 0, &data_loss);
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

bool ffmpeg::tools::can_hardware_encode(const AVCodec* codec)
{
	AVPixelFormat hardware_formats[] = {AV_PIX_FMT_D3D11};

	for (const AVPixelFormat* fmt = codec->pix_fmts; (fmt != nullptr) && (*fmt != AV_PIX_FMT_NONE); fmt++) {
		for (auto cmp : hardware_formats) {
			if (*fmt == cmp) {
				return true;
			}
		}
	}
	return false;
}

std::vector<AVPixelFormat> ffmpeg::tools::get_software_formats(const AVPixelFormat* list)
{
	AVPixelFormat hardware_formats[] = {
#if FF_API_VAAPI
		AV_PIX_FMT_VAAPI_MOCO,
		AV_PIX_FMT_VAAPI_IDCT,
#endif
		AV_PIX_FMT_VAAPI,
		AV_PIX_FMT_DXVA2_VLD,
		AV_PIX_FMT_VDPAU,
		AV_PIX_FMT_QSV,
		AV_PIX_FMT_MMAL,
		AV_PIX_FMT_D3D11VA_VLD,
		AV_PIX_FMT_CUDA,
		AV_PIX_FMT_XVMC,
		AV_PIX_FMT_VIDEOTOOLBOX,
		AV_PIX_FMT_MEDIACODEC,
		AV_PIX_FMT_D3D11,
		AV_PIX_FMT_OPENCL,
	};

	std::vector<AVPixelFormat> fmts;
	for (auto fmt = list; fmt && (*fmt != AV_PIX_FMT_NONE); fmt++) {
		bool is_blacklisted = false;
		for (auto blacklisted : hardware_formats) {
			if (*fmt == blacklisted)
				is_blacklisted = true;
		}
		if (!is_blacklisted)
			fmts.push_back(*fmt);
	}

	fmts.push_back(AV_PIX_FMT_NONE);

	return std::move(fmts);
}

static std::map<std::pair<AVPixelFormat, AVPixelFormat>, double_t> format_compatibility = {
    {{AV_PIX_FMT_NV12, AV_PIX_FMT_NV12}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_NV12, AV_PIX_FMT_NV21}, 65535.0},

    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUVA420P}, 65535.0},
    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P9}, 58981.5},
    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P10}, 53083.35},
    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P12}, 47775.015},
    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P14}, 42997.5135},
    {{AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P16}, 38697.76215},

    {{AV_PIX_FMT_YUVA420P, AV_PIX_FMT_YUVA420P}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YUVA420P, AV_PIX_FMT_YUVA420P9}, 65535.0},
    {{AV_PIX_FMT_YUVA420P, AV_PIX_FMT_YUVA420P10}, 58981.5},
    {{AV_PIX_FMT_YUVA420P, AV_PIX_FMT_YUVA420P16}, 53083.35},
    {{AV_PIX_FMT_YUVA420P, AV_PIX_FMT_YUV420P}, 32767.0},

    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUVA422P}, 65535.0},
    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P9}, 58981.5},
    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P10}, 53083.35},
    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P12}, 47775.015},
    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P14}, 42997.5135},
    {{AV_PIX_FMT_YUV422P, AV_PIX_FMT_YUV422P16}, 38697.76215},

    {{AV_PIX_FMT_YUVA422P, AV_PIX_FMT_YUVA422P}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA422P9}, 65535.0},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA422P10}, 58981.5},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA422P16}, 53083.35},
    {{AV_PIX_FMT_YUVA422P, AV_PIX_FMT_YUV422P}, 32767.0},

    {{AV_PIX_FMT_YVYU422, AV_PIX_FMT_YVYU422}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YVYU422, AV_PIX_FMT_YUYV422}, 65535.0},

    {{AV_PIX_FMT_UYVY422, AV_PIX_FMT_UYVY422}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_UYVY422, AV_PIX_FMT_YVYU422}, 65535.0},

    {{AV_PIX_FMT_YUYV422, AV_PIX_FMT_YUYV422}, std::numeric_limits<double_t>::max()},

    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUVA444P}, 65535.0},
    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P9}, 58981.5},
    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P10}, 53083.35},
    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P12}, 47775.015},
    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P14}, 42997.5135},
    {{AV_PIX_FMT_YUV444P, AV_PIX_FMT_YUV444P16}, 38697.76215},

    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA444P}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA444P9}, 65535.0},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA444P10}, 58981.5},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUVA444P16}, 53083.35},
    {{AV_PIX_FMT_YUVA444P, AV_PIX_FMT_YUV444P}, 32767.0},

    {{AV_PIX_FMT_RGBA, AV_PIX_FMT_RGBA}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_RGBA, AV_PIX_FMT_RGB0}, 65535.0},
    {{AV_PIX_FMT_RGBA, AV_PIX_FMT_0RGB}, 32767.0},
    {{AV_PIX_FMT_RGBA, AV_PIX_FMT_RGB24}, 16384.0},

    {{AV_PIX_FMT_BGRA, AV_PIX_FMT_BGRA}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_BGRA, AV_PIX_FMT_BGR0}, 65535.0},
    {{AV_PIX_FMT_BGRA, AV_PIX_FMT_0BGR}, 32767.0},
    {{AV_PIX_FMT_BGRA, AV_PIX_FMT_BGR24}, 16384.0},

    {{AV_PIX_FMT_BGR0, AV_PIX_FMT_BGR0}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_BGR0, AV_PIX_FMT_BGRA}, 65535.0},
    {{AV_PIX_FMT_BGR0, AV_PIX_FMT_BGR24}, 32767.0},

    {{AV_PIX_FMT_GRAY8, AV_PIX_FMT_GRAY8}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_GRAY8, AV_PIX_FMT_GRAY9}, 65535.0},
    {{AV_PIX_FMT_GRAY8, AV_PIX_FMT_GRAY10}, 58981.5},
    {{AV_PIX_FMT_GRAY8, AV_PIX_FMT_GRAY12}, 53083.35},
    {{AV_PIX_FMT_GRAY8, AV_PIX_FMT_GRAY14}, 47775.015},
    {{AV_PIX_FMT_GRAY8, AV_PIX_FMT_GRAY16}, 42997.5135},

    {{AV_PIX_FMT_BGR24, AV_PIX_FMT_BGR24}, std::numeric_limits<double_t>::max()},
    {{AV_PIX_FMT_BGR24, AV_PIX_FMT_RGB24}, 32767.0},
};

AVPixelFormat ffmpeg::tools::get_best_compatible_format(const AVPixelFormat* list, AVPixelFormat source)
{
	double_t      score = std::numeric_limits<double_t>::min();
	AVPixelFormat best  = source;

	for (auto fmt = list; fmt && (*fmt != AV_PIX_FMT_NONE); fmt++) {
		auto found = format_compatibility.find(std::pair{source, *fmt});
		if (found != format_compatibility.end()) {
			score = found->second;
			best  = *fmt;
		}
	}

	if (score <= 0) {
		int data_loss = 0;
		return avcodec_find_best_pix_fmt_of_list(list, source, 0, &data_loss);
	}

	return best;
}
