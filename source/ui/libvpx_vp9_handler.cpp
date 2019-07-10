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

#include "libvpx_vp9_handler.hpp"
#include "plugin.hpp"
#include "strings.hpp"
#include "utility.hpp"

extern "C" {
#include <obs-module.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavutil/opt.h>
#pragma warning(pop)
}

/*
[obs-ffmpeg-encoder] Options for 'libvpx-vp9':
[obs-ffmpeg-encoder]   Option 'auto-alt-ref' with help 'Enable use of alternate reference frames (2-pass only)' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2.000000'.
[obs-ffmpeg-encoder]   Option 'lag-in-frames' with help 'Number of frames to look ahead for alternate reference frame selection' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'arnr-maxframes' with help 'altref noise reduction max frame count' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'arnr-strength' with help 'altref noise reduction filter strength' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'arnr-type' with unit (arnr_type) with help 'altref noise reduction filter type' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   [arnr_type] Flag 'backward' and help text '(null)' with value '1'.
[obs-ffmpeg-encoder]   [arnr_type] Flag 'forward' and help text '(null)' with value '2'.
[obs-ffmpeg-encoder]   [arnr_type] Flag 'centered' and help text '(null)' with value '3'.
[obs-ffmpeg-encoder]   Option 'tune' with unit (tune) with help 'Tune the encoding to a specific scenario' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   [tune] Constant 'psnr' and help text '(null)' with value '0'.
[obs-ffmpeg-encoder]   [tune] Constant 'ssim' and help text '(null)' with value '1'.
[obs-ffmpeg-encoder]   Option 'deadline' with unit (quality) with help 'Time to spend encoding, in microseconds.' of type 'Int' with default value '1000000', minimum '-2147483648.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   [quality] Flag 'best' and help text '(null)' with value '0'.
[obs-ffmpeg-encoder]   [quality] Flag 'good' and help text '(null)' with value 'F4240'.
[obs-ffmpeg-encoder]   [quality] Flag 'realtime' and help text '(null)' with value '1'.
[obs-ffmpeg-encoder]   Option 'error-resilient' with unit (er) with help 'Error resilience configuration' of type 'Flags' with default value '0', minimum '-2147483648.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'max-intra-rate' with help 'Maximum I-frame bitrate (pct) 0=unlimited' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   [er] Flag 'default' and help text 'Improve resiliency against losses of whole frames' with value '1'.
[obs-ffmpeg-encoder]   [er] Flag 'partitions' and help text 'The frame partitions are independently decodable by the bool decoder, meaning that partitions can be decoded even though earlier partitions have been lost. Note that intra predicition is still done over the partition boundary.' with value '2'.
[obs-ffmpeg-encoder]   Option 'crf' with help 'Select the quality for constant quality mode' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '63.000000'.
[obs-ffmpeg-encoder]   Option 'static-thresh' with help 'A change threshold on blocks below which they will be skipped by the encoder' of type 'Int' with default value '0', minimum '0.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'drop-threshold' with help 'Frame drop threshold' of type 'Int' with default value '0', minimum '-2147483648.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'noise-sensitivity' with help 'Noise sensitivity' of type 'Int' with default value '0', minimum '0.000000' and maximum '4.000000'.
[obs-ffmpeg-encoder]   Option 'undershoot-pct' with help 'Datarate undershoot (min) target (%)' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '100.000000'.
[obs-ffmpeg-encoder]   Option 'overshoot-pct' with help 'Datarate overshoot (max) target (%)' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '1000.000000'.
[obs-ffmpeg-encoder]   Option 'cpu-used' with help 'Quality/Speed ratio modifier' of type 'Int' with default value '1', minimum '-8.000000' and maximum '8.000000'.
[obs-ffmpeg-encoder]   Option 'lossless' with help 'Lossless mode' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '1.000000'.
[obs-ffmpeg-encoder]   Option 'tile-columns' with help 'Number of tile columns to use, log2' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '6.000000'.
[obs-ffmpeg-encoder]   Option 'tile-rows' with help 'Number of tile rows to use, log2' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '2.000000'.
[obs-ffmpeg-encoder]   Option 'frame-parallel' with help 'Enable frame parallel decodability features' of type 'Bool' with default value 'true', minimum '-1.000000' and maximum '1.000000'.
[obs-ffmpeg-encoder]   Option 'aq-mode' with unit (aq_mode) with help 'adaptive quantization mode' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '4.000000'.
[obs-ffmpeg-encoder]   [aq_mode] Flag 'none' and help text 'Aq not used' with value '0'.
[obs-ffmpeg-encoder]   [aq_mode] Flag 'variance' and help text 'Variance based Aq' with value '1'.
[obs-ffmpeg-encoder]   [aq_mode] Flag 'complexity' and help text 'Complexity based Aq' with value '2'.
[obs-ffmpeg-encoder]   [aq_mode] Flag 'cyclic' and help text 'Cyclic Refresh Aq' with value '3'.
[obs-ffmpeg-encoder]   [aq_mode] Flag 'equator360' and help text '360 video Aq' with value '4'.
[obs-ffmpeg-encoder]   Option 'level' with help 'Specify level' of type 'Float' with default value '-1.000000', minimum '-1.000000' and maximum '6.200000'.
[obs-ffmpeg-encoder]   Option 'row-mt' with help 'Row based multi-threading' of type 'Bool' with default value 'true', minimum '-1.000000' and maximum '1.000000'.
[obs-ffmpeg-encoder]   Option 'speed' with help '' of type 'Int' with default value '1', minimum '-16.000000' and maximum '16.000000'.
[obs-ffmpeg-encoder]   Option 'quality' with unit (quality) with help '' of type 'Int' with default value '1000000', minimum '-2147483648.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   Option 'vp8flags' with unit (flags) with help '' of type 'Flags' with default value '0', minimum '0.000000' and maximum '4294967295.000000'.
[obs-ffmpeg-encoder]   [flags] Flag 'error_resilient' and help text 'enable error resilience' with value '1'.
[obs-ffmpeg-encoder]   [flags] Flag 'altref' and help text 'enable use of alternate reference frames (VP8/2-pass only)' with value '2'.
[obs-ffmpeg-encoder]   Option 'arnr_max_frames' with help 'altref noise reduction max frame count' of type 'Int' with default value '0', minimum '0.000000' and maximum '15.000000'.
[obs-ffmpeg-encoder]   Option 'arnr_strength' with help 'altref noise reduction filter strength' of type 'Int' with default value '3', minimum '0.000000' and maximum '6.000000'.
[obs-ffmpeg-encoder]   Option 'arnr_type' with help 'altref noise reduction filter type' of type 'Int' with default value '3', minimum '1.000000' and maximum '3.000000'.
[obs-ffmpeg-encoder]   Option 'rc_lookahead' with help 'Number of frames to look ahead for alternate reference frame selection' of type 'Int' with default value '25', minimum '0.000000' and maximum '25.000000'.
*/

INITIALIZER(libvpx_vp9_handler_init)
{
	obsffmpeg::initializers.push_back([]() {
		obsffmpeg::register_codec_handler("libvpx-vp9", std::make_shared<obsffmpeg::ui::libvpx_vp9_handler>());
	});
};

#define P_RATECONTROL "VP9.RateControl"
#define P_RATECONTROL_MODE "VP9.RateControl.Mode"
#define P_RATECONTROL_MODE_(x) "VP9.RateControl.Mode." D_VSTR(x)
#define P_RATECONTROL_QUALITY "VP9.RateControl.Quality"
#define P_RATECONTROL_BITRATE_TARGET "VP9.RateControl.Bitrate.Target"
#define P_RATECONTROL_BITRATE_MINIMUM "VP9.RateControl.Bitrate.Minimum"
#define P_RATECONTROL_BITRATE_MAXIMUM "VP9.RateControl.Bitrate.Maximum"
#define P_RATECONTROL_BUFFERSIZE "VP9.RateControl.BufferSize"

#define P_KEYFRAMES "VP9.KeyFrames"
#define P_KEYFRAMES_INTERVALTYPE "VP9.KeyFrames.IntervalType"
#define P_KEYFRAMES_INTERVALTYPE_(x) "VP9.KeyFrames.IntervalType." D_VSTR(x)
#define P_KEYFRAMES_INTERVAL "VP9.KeyFrames.Interval"
#define P_KEYFRAMES_INTERVAL_SECONDS "VP9.KeyFrames.Interval.Seconds"
#define P_KEYFRAMES_INTERVAL_FRAMES "VP9.KeyFrames.Interval.Frames"

#define P_PERFORMANCE "VP9.Performance"
#define P_PERFORMANCE_QUALITYSPEEDRATIO P_PERFORMANCE ".QualitySpeedRatio"
#define P_PERFORMANCE_DEADLINE P_PERFORMANCE ".Deadline"
#define P_PERFORMANCE_TILING P_PERFORMANCE ".Tiling"
#define P_PERFORMANCE_TILING_ROWS P_PERFORMANCE_TILING ".Rows"
#define P_PERFORMANCE_TILING_COLUMNS P_PERFORMANCE_TILING ".Columns"

#define S_RATECONTROL_MODE_CBR "CBR"
#define S_RATECONTROL_MODE_ABR "ABR"
#define S_RATECONTROL_MODE_VBR "VBR"
#define S_RATECONTROL_MODE_CQ "CQ"
#define S_RATECONTROL_MODE_VQ "VQ"
#define S_RATECONTROL_MODE_LL "LL"

std::pair<std::string, std::string> rc_modes[] = {

    // CBR defines bitrate, minrate, maxrate, buffersize. Min and maxrate are bitrate.
    {S_RATECONTROL_MODE_CBR, P_RATECONTROL_MODE_(ConstantBitrate)},
    // ABR defines bitrate
    {S_RATECONTROL_MODE_ABR, P_RATECONTROL_MODE_(AverageBitrate)},
    // VBR defines bitrate, minrate, maxrate (and buffersize?)
    {S_RATECONTROL_MODE_VBR, P_RATECONTROL_MODE_(VariableBitrate)},
    // CQ defines "crf" and sets bitrate to 0.
    {S_RATECONTROL_MODE_CQ, P_RATECONTROL_MODE_(ConstantQuality)},
    // VQ defines "crf" and non-zero bitrate maximum.
    {S_RATECONTROL_MODE_VQ, P_RATECONTROL_MODE_(VariableQuality)},
    // Lossless defines nothing but lossless.
    {S_RATECONTROL_MODE_LL, P_RATECONTROL_MODE_(Lossless)},
};

void obsffmpeg::ui::libvpx_vp9_handler::get_defaults(obs_data_t* settings, AVCodec*, AVCodecContext*)
{
	obs_data_set_default_string(settings, P_RATECONTROL_MODE, S_RATECONTROL_MODE_CBR);
	obs_data_set_default_int(settings, P_RATECONTROL_QUALITY, 23);
	obs_data_set_default_int(settings, P_RATECONTROL_BITRATE_TARGET, 2500);
	obs_data_set_default_int(settings, P_RATECONTROL_BITRATE_MINIMUM, 2500);
	obs_data_set_default_int(settings, P_RATECONTROL_BITRATE_MAXIMUM, 2500);
	obs_data_set_default_int(settings, P_RATECONTROL_BUFFERSIZE, 5000);

	obs_data_set_default_int(settings, P_KEYFRAMES_INTERVALTYPE, 0);
	obs_data_set_default_double(settings, P_KEYFRAMES_INTERVAL_SECONDS, 2.0);
	obs_data_set_default_int(settings, P_KEYFRAMES_INTERVAL_FRAMES, 300);

	obs_data_set_default_int(settings, P_PERFORMANCE_QUALITYSPEEDRATIO, 1);
	obs_data_set_default_double(settings, P_PERFORMANCE_DEADLINE, 100.0);
	obs_data_set_default_bool(settings, P_PERFORMANCE_TILING, false);
	obs_data_set_default_int(settings, P_PERFORMANCE_TILING_COLUMNS, -1);
	obs_data_set_default_int(settings, P_PERFORMANCE_TILING_ROWS, -1);
}

void obsffmpeg::ui::libvpx_vp9_handler::get_properties_codec(obs_properties_t* props, AVCodec* codec)
{
	{
		auto grp = obs_properties_create();
		{
			auto p = obs_properties_add_list(grp, P_RATECONTROL_MODE, TRANSLATE(P_RATECONTROL_MODE),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_MODE)));
			obs_property_set_modified_callback(p, modified_ratecontrol);
			for (auto kv : rc_modes) {
				obs_property_list_add_string(p, TRANSLATE(kv.second.c_str()), kv.first.c_str());
			}
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QUALITY,
			                                       TRANSLATE(P_RATECONTROL_QUALITY), 0, 63, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QUALITY)));
		}
		{
			auto p = obs_properties_add_int(grp, P_RATECONTROL_BITRATE_TARGET,
			                                TRANSLATE(P_RATECONTROL_BITRATE_TARGET), 1,
			                                std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BITRATE_TARGET)));
		}
		{
			auto p = obs_properties_add_int(grp, P_RATECONTROL_BITRATE_MINIMUM,
			                                TRANSLATE(P_RATECONTROL_BITRATE_MINIMUM), 0,
			                                std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BITRATE_MINIMUM)));
		}
		{
			auto p = obs_properties_add_int(grp, P_RATECONTROL_BITRATE_MAXIMUM,
			                                TRANSLATE(P_RATECONTROL_BITRATE_MAXIMUM), 0,
			                                std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BITRATE_MAXIMUM)));
		}
		{
			auto p =
			    obs_properties_add_int(grp, P_RATECONTROL_BUFFERSIZE, TRANSLATE(P_RATECONTROL_BUFFERSIZE),
			                           0, std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BUFFERSIZE)));
		}

		obs_properties_add_group(props, P_RATECONTROL, TRANSLATE(P_RATECONTROL), OBS_GROUP_NORMAL, grp);
	}
	{
		auto grp = obs_properties_create();
		{
			auto p =
			    obs_properties_add_list(grp, P_KEYFRAMES_INTERVALTYPE, TRANSLATE(P_KEYFRAMES_INTERVALTYPE),
			                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_KEYFRAMES_INTERVALTYPE)));
			obs_property_set_modified_callback(p, &modified_keyframes);
			obs_property_list_add_int(p, TRANSLATE(P_KEYFRAMES_INTERVALTYPE_(Seconds)), 0);
			obs_property_list_add_int(p, TRANSLATE(P_KEYFRAMES_INTERVALTYPE_(Frames)), 1);
		}
		{
			auto p =
			    obs_properties_add_float(grp, P_KEYFRAMES_INTERVAL_SECONDS, TRANSLATE(P_KEYFRAMES_INTERVAL),
			                             0.00, std::numeric_limits<int16_t>::max(), 0.01);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_KEYFRAMES_INTERVAL)));
		}
		{
			auto p =
			    obs_properties_add_int(grp, P_KEYFRAMES_INTERVAL_FRAMES, TRANSLATE(P_KEYFRAMES_INTERVAL), 0,
			                           std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_KEYFRAMES_INTERVAL)));
		}

		obs_properties_add_group(props, P_KEYFRAMES, TRANSLATE(P_KEYFRAMES), OBS_GROUP_NORMAL, grp);
	}
	{
		auto grp = obs_properties_create();
		{
			auto p = obs_properties_add_int_slider(grp, P_PERFORMANCE_QUALITYSPEEDRATIO,
			                                       TRANSLATE(P_PERFORMANCE_QUALITYSPEEDRATIO), -16, 16, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_PERFORMANCE_QUALITYSPEEDRATIO)));
		}
		{
			auto p = obs_properties_add_float_slider(grp, P_PERFORMANCE_DEADLINE,
			                                         TRANSLATE(P_PERFORMANCE_DEADLINE), 0.0, 1000.0, 0.01);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_PERFORMANCE_DEADLINE)));
		}
		{
			auto grp2 = obs_properties_create();
			{
				auto p =
				    obs_properties_add_int_slider(grp2, P_PERFORMANCE_TILING_COLUMNS,
				                                  TRANSLATE(P_PERFORMANCE_TILING_COLUMNS), 0, 6, 1);
				obs_property_set_long_description(p, TRANSLATE(DESC(P_PERFORMANCE_TILING_COLUMNS)));
			}
			{
				auto p = obs_properties_add_int_slider(grp2, P_PERFORMANCE_TILING_ROWS,
				                                       TRANSLATE(P_PERFORMANCE_TILING_ROWS), 0, 2, 1);
				obs_property_set_long_description(p, TRANSLATE(DESC(P_PERFORMANCE_TILING_ROWS)));
			}

			obs_properties_add_group(grp, P_PERFORMANCE_TILING, TRANSLATE(P_PERFORMANCE_TILING),
			                         OBS_GROUP_CHECKABLE, grp2);
		}
		obs_properties_add_group(props, P_PERFORMANCE, TRANSLATE(P_PERFORMANCE), OBS_GROUP_NORMAL, grp);
	}
}

void obsffmpeg::ui::libvpx_vp9_handler::get_properties_encoder(obs_properties_t* props, AVCodec* codec,
                                                               AVCodecContext* context)
{
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL), false);
	obs_property_set_enabled(obs_properties_get(props, P_KEYFRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, P_PERFORMANCE), false);
}

void obsffmpeg::ui::libvpx_vp9_handler::get_properties(obs_properties_t* props, AVCodec* codec, AVCodecContext* context)
{
	if (!context) {
		get_properties_codec(props, codec);
	} else {
		get_properties_encoder(props, codec, context);
	}
}

void obsffmpeg::ui::libvpx_vp9_handler::update(obs_data_t* settings, AVCodec*, AVCodecContext* context)
{
	obs_video_info ovi;
	if (!obs_get_video_info(&ovi)) {
		throw std::runtime_error("no video info");
	}

	{ // Rate Control
		bool        has_quality     = false;
		bool        has_bitrate     = false;
		bool        has_bitrate_min = false;
		bool        has_bitrate_max = false;
		bool        has_buffer      = false;
		const char* rc_mode         = obs_data_get_string(settings, P_RATECONTROL_MODE);
		if (strcmp(rc_mode, S_RATECONTROL_MODE_LL) == 0) {
		} else if (strcmp(rc_mode, S_RATECONTROL_MODE_CBR) == 0) {
			has_bitrate = true;
			has_buffer  = true;
		} else if (strcmp(rc_mode, S_RATECONTROL_MODE_ABR) == 0) {
			has_bitrate = true;
		} else if (strcmp(rc_mode, S_RATECONTROL_MODE_VBR) == 0) {
			has_bitrate     = true;
			has_bitrate_max = true;
			has_bitrate_min = true;
			has_buffer      = true;
		} else if (strcmp(rc_mode, S_RATECONTROL_MODE_CQ) == 0) {
			has_quality = true;
		} else if (strcmp(rc_mode, S_RATECONTROL_MODE_VQ) == 0) {
			has_quality     = true;
			has_bitrate_max = true;
		}

		if (has_quality) {
			av_opt_set_int(context->priv_data, "crf", obs_data_get_int(settings, P_RATECONTROL_QUALITY), 0);
		}
		if (has_bitrate) {
			context->bit_rate = static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BITRATE_TARGET));
		}
		if (has_bitrate_min) {
			context->rc_min_rate =
			    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BITRATE_MINIMUM));
		}
		if (has_bitrate_max) {
			context->rc_max_rate =
			    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BITRATE_MAXIMUM));
		}
		if (has_buffer) {
			context->rc_buffer_size =
			    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BUFFERSIZE));
		}
	}

	{ // Key Frames
		int64_t kf_type    = obs_data_get_int(settings, P_KEYFRAMES_INTERVALTYPE);
		bool    is_seconds = (kf_type == 0);

		if (is_seconds) {
			context->gop_size = static_cast<int>(obs_data_get_double(settings, P_KEYFRAMES_INTERVAL_SECONDS)
			                                     * (ovi.fps_num / ovi.fps_den));
		} else {
			context->gop_size = static_cast<int>(obs_data_get_int(settings, P_KEYFRAMES_INTERVAL_FRAMES));
		}
		context->keyint_min = context->gop_size;
	}

	{ // Performance
		// Row Multithreading is always on.
		av_opt_set_int(context->priv_data, "row-mt", 1, 0);

		// Quality/Speed Ratio Modifier
		{
			int v = static_cast<int>(obs_data_get_int(settings, P_PERFORMANCE_QUALITYSPEEDRATIO));
			av_opt_set_int(context->priv_data, "cpu-used", v, 0);
			av_opt_set_int(context->priv_data, "speed", v, 0);
		}

		{ // Deadline
			auto deadline_mul = obs_data_get_double(settings, P_PERFORMANCE_DEADLINE) / 100.0;
			auto frame_ms     = 1000.0 / (ovi.fps_num * ovi.fps_den);
			int  final_ms     = static_cast<int>(deadline_mul * frame_ms);
			av_opt_set_int(context->priv_data, "deadline", final_ms, 0);
		}

		{ // Tiling
			int columns, rows;

			columns = context->width / 64;
			if (columns == 0) {
				columns = -1;
			} else if (columns > 6) {
				columns = 6;
			}

			rows = context->height / 64;
			if (rows == 0) {
				rows = -1;
			} else if (rows > 2) {
				rows = 2;
			}

			av_opt_set_int(context->priv_data, "tile-columns", columns, 0);
			av_opt_set_int(context->priv_data, "tile-rows", rows, 0);
		}
	}

	av_opt_set_int(context->priv_data, "lag-in-frames", 0, 0);
	av_opt_set_int(context->priv_data, "arnr-maxframes", 0, 0);
	av_opt_set_int(context->priv_data, "aq-mode", 0, 0);
	av_opt_set_double(context->priv_data, "level", 6.2, 0);
	av_opt_set_int(context->priv_data, "rc_lookahead", 0, 0);
}

bool obsffmpeg::ui::libvpx_vp9_handler::modified_ratecontrol(obs_properties_t* props, obs_property_t* property,
                                                             obs_data_t* settings)
{
	bool has_quality     = false;
	bool has_bitrate     = false;
	bool has_bitrate_min = false;
	bool has_bitrate_max = false;
	bool has_buffer      = false;

	const char* rc_mode = obs_data_get_string(settings, P_RATECONTROL_MODE);
	if (strcmp(rc_mode, S_RATECONTROL_MODE_LL) == 0) {
	} else if (strcmp(rc_mode, S_RATECONTROL_MODE_CBR) == 0) {
		has_bitrate = true;
		has_buffer  = true;
	} else if (strcmp(rc_mode, S_RATECONTROL_MODE_ABR) == 0) {
		has_bitrate = true;
	} else if (strcmp(rc_mode, S_RATECONTROL_MODE_VBR) == 0) {
		has_bitrate     = true;
		has_bitrate_max = true;
		has_bitrate_min = true;
		has_buffer      = true;
	} else if (strcmp(rc_mode, S_RATECONTROL_MODE_CQ) == 0) {
		has_quality = true;
	} else if (strcmp(rc_mode, S_RATECONTROL_MODE_VQ) == 0) {
		has_quality     = true;
		has_bitrate_max = true;
	}

	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QUALITY), has_quality);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BITRATE_TARGET), has_bitrate);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BITRATE_MINIMUM), has_bitrate_min);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BITRATE_MAXIMUM), has_bitrate_max);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BUFFERSIZE), has_buffer);

	return true;
}

bool obsffmpeg::ui::libvpx_vp9_handler::modified_keyframes(obs_properties_t* props, obs_property_t* property,
                                                           obs_data_t* settings)
{
	bool is_seconds = obs_data_get_int(settings, P_KEYFRAMES_INTERVALTYPE) == 0;

	obs_property_set_visible(obs_properties_get(props, P_KEYFRAMES_INTERVAL_FRAMES), !is_seconds);
	obs_property_set_visible(obs_properties_get(props, P_KEYFRAMES_INTERVAL_SECONDS), is_seconds);

	return true;
}
