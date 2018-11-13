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

#include "prores_aw.hpp"
#include <obs-module.h>
#include <thread>
#include "ffmpeg/tools.hpp"
#include "utility.hpp"

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#pragma warning(pop)
}

#include <util/profiler.hpp>

#define T_PROFILE "ProRes.Profile"
#define T_PROFILE_(x) "ProRes.Profile." vstr(x)
#define T_CUSTOM "Custom"

#define LOG_PREFIX "[prores_aw] "

make_encoder_base(prores_aw, obsffmpeg::encoder::prores_aw, "ProRes (Anatoliy Wasserman) (FFMPEG)", "prores");

void obsffmpeg::encoder::prores_aw::initialize()
{
	auto avd = avcodec_find_encoder_by_name("prores_aw");
	if (!avd) {
		PLOG_INFO("ProRes (Anatoliy Wasserman) not supported.");
		return;
	}
	prores_aw_initialize();
	PLOG_INFO("ProRes (Anatoliy Wasserman) supported.");
}

void obsffmpeg::encoder::prores_aw::finalize()
{
	//prores_aw_finalize();
}

void obsffmpeg::encoder::prores_aw::get_defaults(obs_data_t* settings)
{
	obs_data_set_default_int(settings, T_PROFILE, static_cast<long long>(profile::HighQuality));
}

obs_properties_t* obsffmpeg::encoder::prores_aw::get_properties()
{
	obs_properties_t* prs = obs_properties_create();
	obs_property_t*   p   = nullptr;

	p = obs_properties_add_list(prs, T_PROFILE, TRANSLATE(T_PROFILE), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(p, TRANSLATE(DESC(T_PROFILE)));
	obs_property_list_add_int(p, TRANSLATE(T_PROFILE_(Proxy)), static_cast<long long>(profile::Proxy));
	obs_property_list_add_int(p, TRANSLATE(T_PROFILE_(Light)), static_cast<long long>(profile::Light));
	obs_property_list_add_int(p, TRANSLATE(T_PROFILE_(Standard)), static_cast<long long>(profile::Standard));
	obs_property_list_add_int(p, TRANSLATE(T_PROFILE_(HighQuality)), static_cast<long long>(profile::HighQuality));

	p = obs_properties_add_text(prs, T_CUSTOM, TRANSLATE(T_CUSTOM), OBS_TEXT_DEFAULT);
	obs_property_set_long_description(p, TRANSLATE(DESC(T_CUSTOM)));

	return prs;
}

obsffmpeg::encoder::prores_aw::prores_aw(obs_data_t* settings, obs_encoder_t* encoder)
{
	this->self    = encoder;
	auto encvideo = obs_encoder_video(this->self);
	auto voi      = video_output_get_info(encvideo);

	// Options, Parameters, etc.
	/// Resolution, Frame Rate
	this->resolution.first  = voi->width;
	this->resolution.second = voi->height;
	this->framerate.first   = voi->fps_num;
	this->framerate.second  = voi->fps_den;
	/// Source/Input
	this->source_colorspace = ffmpeg::tools::obs_videocolorspace_to_avcolorspace(voi->colorspace);
	this->source_range      = ffmpeg::tools::obs_videorangetype_to_avcolorrange(voi->range);
	this->source_format     = ffmpeg::tools::obs_videoformat_to_avpixelformat(voi->format);
	/// Target/Output
	this->target_colorspace = this->source_colorspace;
	this->target_range      = this->source_range;
	switch (voi->format) {
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		this->target_format = AV_PIX_FMT_YUV444P10;
		this->video_profile = profile::FourFourFourFour;
		break;
	case VIDEO_FORMAT_I444:
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
		this->video_profile = static_cast<profile>(obs_data_get_int(settings, T_PROFILE));
		if (this->video_profile == profile::FourFourFourFour) {
			this->target_format = AV_PIX_FMT_YUV444P10;
		} else {
			this->target_format = AV_PIX_FMT_YUV422P10;
		}
		break;
	}

	// Log Settings
	PLOG_INFO(LOG_PREFIX
	          "Initializing encoder...\n\tResolution: %lux%lu\n\tFramerate: %lu/%lu (%.2lf fps)\n\tInput: %s %s "
	          "%s\n\tOutput: %s %s %s\n\tProfile: %ld",
	          this->resolution.first, this->resolution.second, this->framerate.first, this->framerate.second,
	          (double_t(this->framerate.first) / double_t(this->framerate.second)),
	          ffmpeg::tools::get_pixel_format_name(this->source_format),
	          ffmpeg::tools::get_color_space_name(this->source_colorspace),
	          swscale.is_source_full_range() ? "Full" : "Partial",
	          ffmpeg::tools::get_pixel_format_name(this->target_format),
	          ffmpeg::tools::get_color_space_name(this->target_colorspace),
	          swscale.is_target_full_range() ? "Full" : "Partial", static_cast<int>(this->video_profile));

	// prores_aw restriction
	if (this->resolution.first % 2 == 1) {
		PLOG_ERROR(LOG_PREFIX "Width must be a multiple of 2.");
		throw std::exception();
	}

	// Quit if we for some reason can't find prores_aw anymore.
	this->avcodec = avcodec_find_encoder_by_name("prores_aw");
	if (!this->avcodec) {
		PLOG_ERROR(LOG_PREFIX "Failed to find encoder.");
		throw std::exception();
	}

	this->avcontext = avcodec_alloc_context3(this->avcodec);
	if (!this->avcontext) {
		PLOG_ERROR(LOG_PREFIX "Failed to create context.");
		throw std::exception();
	}

	// Apply Settings
	/// Resolution
	this->avcontext->width  = this->resolution.first;
	this->avcontext->height = this->resolution.second;
	/// Framerate
	this->avcontext->time_base.num   = this->framerate.first;
	this->avcontext->time_base.den   = this->framerate.second;
	this->avcontext->ticks_per_frame = 1;
	/// GOP/Keyframe (ProRes is Intra only)
	this->avcontext->gop_size = 0;
	/// Color, Profile
	this->avcontext->colorspace  = this->target_colorspace;
	this->avcontext->color_range = this->target_range;
	this->avcontext->pix_fmt     = this->target_format;
	this->avcontext->profile     = static_cast<int>(this->video_profile);
	/// Other
	this->avcontext->field_order           = AV_FIELD_PROGRESSIVE;
	this->avcontext->strict_std_compliance = FF_COMPLIANCE_NORMAL;
	this->avcontext->debug                 = 0;
	/// Threading
	this->avcontext->thread_type = FF_THREAD_FRAME;
#if defined(__cplusplus)
	this->avcontext->thread_count = std::thread::hardware_concurrency();
#elif defined(_WIN32)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		this->avcontext->thread_count = sysinfo.dwNumberOfProcessors;
	}
#elif defined(_GNU)
	this->avcontext->thread_count = 16;
#else
	this->avcontext->thread_count = 16;
#endif
	/// Dynamic Stuff
	this->update(settings);

	// Open Encoder
	int res = avcodec_open2(this->avcontext, this->avcodec, NULL);
	if (res < 0) {
		avcodec_free_context(&this->avcontext);
		PLOG_ERROR(LOG_PREFIX "Failed to open codec: %s (%ld).", ffmpeg::tools::get_error_description(res),
		           res);
		throw std::exception();
	}

	// Configure and initialize SWScale
	swscale.set_source_size(voi->width, voi->height);
	swscale.set_source_format(this->source_format);
	swscale.set_source_color(this->source_range == AVCOL_RANGE_JPEG, this->source_colorspace);
	swscale.set_target_size(voi->width, voi->height);
	swscale.set_target_format(this->target_format);
	swscale.set_target_color(this->target_range == AVCOL_RANGE_JPEG, this->target_colorspace);

	if (!swscale.initialize(SWS_FAST_BILINEAR)) {
		PLOG_ERROR(LOG_PREFIX "Incompatible conversion parameters:\n\tInput: %s %s %s\n\tOutput: %s %s %s",
		           ffmpeg::tools::get_pixel_format_name(this->source_format),
		           ffmpeg::tools::get_color_space_name(this->source_colorspace),
		           swscale.is_source_full_range() ? "Full" : "Partial",
		           ffmpeg::tools::get_pixel_format_name(this->target_format),
		           ffmpeg::tools::get_color_space_name(this->target_colorspace),
		           swscale.is_target_full_range() ? "Full" : "Partial");
		throw std::exception();
	}

	frame_queue.set_pixel_format(this->avcontext->pix_fmt);
	frame_queue.set_resolution(this->resolution.first, this->resolution.second);
	frame_queue.precache(this->avcontext->thread_count / 2);

	this->current_packet = av_packet_alloc();
	if (!this->current_packet) {
		PLOG_ERROR(LOG_PREFIX "Failed to allocated packet.");
		throw std::exception();
	}

	PLOG_INFO(LOG_PREFIX "Encoder initialized.");
}

obsffmpeg::encoder::prores_aw::~prores_aw()
{
	frame_queue.clear();

	swscale.finalize();
	if (this->avcontext) {
		avcodec_close(this->avcontext);
		avcodec_free_context(&this->avcontext);
	}
}

void obsffmpeg::encoder::prores_aw::get_properties(obs_properties_t* props) {}

bool obsffmpeg::encoder::prores_aw::update(obs_data_t* settings)
{
	return false;
}

bool obsffmpeg::encoder::prores_aw::get_extra_data(uint8_t** extra_data, size_t* size)
{
	if (!this->avcontext->extradata) {
		return false;
	}
	*extra_data = this->avcontext->extradata;
	*size       = this->avcontext->extradata_size;
	return true;
}

bool obsffmpeg::encoder::prores_aw::get_sei_data(uint8_t** sei_data, size_t* size)
{
	return false;
}

void obsffmpeg::encoder::prores_aw::get_video_info(video_scale_info* info)
{
	return;
}

bool obsffmpeg::encoder::prores_aw::encode(encoder_frame* frame, encoder_packet* packet, bool* received_packet)
{
	int res = 0;

	{
		ScopeProfiler sp_frame("frame");
		AVFrame*      vframe = frame_queue.pop();
		vframe->pts          = frame->pts;

		vframe->color_range = this->avcontext->color_range;
		vframe->colorspace  = this->avcontext->colorspace;

		{
			ScopeProfiler profile("convert");
			res = swscale.convert(reinterpret_cast<uint8_t**>(frame->data),
			                      reinterpret_cast<int*>(frame->linesize), 0, this->resolution.second,
			                      vframe->data, vframe->linesize);
			if (res <= 0) {
				PLOG_ERROR(LOG_PREFIX "Failed to convert frame: %s (%ld).",
				           ffmpeg::tools::get_error_description(res), res);
				return false;
			}
		}

		{
			ScopeProfiler profile("send");
			res = avcodec_send_frame(this->avcontext, vframe);
			if (res < 0) {
				PLOG_ERROR(LOG_PREFIX "Failed to encode frame: %s (%ld).",
				           ffmpeg::tools::get_error_description(res), res);
				return false;
			}
		}

		frame_queue_used.push(vframe);
	}

	{
		ScopeProfiler profile("receive");
		res = avcodec_receive_packet(this->avcontext, this->current_packet);
		if (res < 0) {
			if (res == AVERROR(EAGAIN)) {
				*received_packet = false;
				return true;
			} else if (res == AVERROR(EOF)) {
				return true;
			} else {
				PLOG_ERROR(LOG_PREFIX "Failed to receive packet: %s (%ld).",
				           ffmpeg::tools::get_error_description(res), res);
				return false;
			}
		} else {
			AVFrame* uframe = frame_queue_used.pop_only();
			if (uframe) {
				if (frame_queue.empty()) {
					frame_queue.push(uframe);
				} else {
					av_frame_free(&uframe);
				}
			}
			packet->type          = OBS_ENCODER_VIDEO;
			packet->pts           = this->current_packet->pts;
			packet->dts           = this->current_packet->pts;
			packet->data          = this->current_packet->data;
			packet->size          = this->current_packet->size;
			packet->keyframe      = true; // There are only keyframes in ProRes (Intra Only)
			packet->drop_priority = 0;
			*received_packet      = true;
		}
	}

	return true;
}
