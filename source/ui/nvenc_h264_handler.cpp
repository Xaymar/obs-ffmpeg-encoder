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

#include "nvenc_h264_handler.hpp"
#include "codecs/h264.hpp"
#include "nvenc_shared.hpp"
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

/* Missing Options:

Seem to be covered by initQP_* instead.
- [obs-ffmpeg-encoder]   Option 'cq' with help 'Set target quality level (0 to 51, 0 means automatic) for constant quality mode in VBR rate control' of type 'Float' with default value '0.000000', minimum '0.000000' and maximum '51.000000'.
- [obs-ffmpeg-encoder]   Option 'qp' with help 'Constant quantization parameter rate control method' of type 'Int' with default value '-1', minimum '-1.000000' and maximum '51.000000'.

Not sure what there are useful for.
[obs-ffmpeg-encoder]   Option 'aud' with help 'Use access unit delimiters' of type 'Bool' with default value 'false', minimum '0.000000' and maximum '1.000000'.
[obs-ffmpeg-encoder]   Option 'surfaces' with help 'Number of concurrent surfaces' of type 'Int' with default value '0', minimum '0.000000' and maximum '64.000000'.
[obs-ffmpeg-encoder]   Option 'delay' with help 'Delay frame output by the given amount of frames' of type 'Int' with default value '2147483647', minimum '0.000000' and maximum '2147483647.000000'.

Should probably add this.
[obs-ffmpeg-encoder]   Option 'gpu' with unit (gpu) with help 'Selects which NVENC capable GPU to use. First GPU is 0, second is 1, and so on.' of type 'Int' with default value '-1', minimum '-2.000000' and maximum '2147483647.000000'.
[obs-ffmpeg-encoder]   [gpu] Constant 'any' and help text 'Pick the first device available' with value '-1'.
[obs-ffmpeg-encoder]   [gpu] Constant 'list' and help text 'List the available devices' with value '-2'.

Useless except for strict_gop maybe?
[obs-ffmpeg-encoder]   Option 'forced-idr' with help 'If forcing keyframes, force them as IDR frames.' of type 'Bool' with default value 'false', minimum '-1.000000' and maximum '1.000000'.
[obs-ffmpeg-encoder]   Option 'strict_gop' with help 'Set 1 to minimize GOP-to-GOP rate fluctuations' of type 'Bool' with default value 'false', minimum '0.000000' and maximum '1.000000'.
[obs-ffmpeg-encoder]   Option 'bluray-compat' with help 'Bluray compatibility workarounds' of type 'Bool' with default value 'false', minimum '0.000000' and maximum '1.000000'.
*/

using namespace obsffmpeg::codecs::h264;

std::map<profile, std::string> profiles{
    {profile::BASELINE, "baseline"},
    {profile::MAIN, "main"},
    {profile::HIGH, "high"},
    {profile::HIGH444_PREDICTIVE, "high444p"},
};

std::map<level, std::string> levels{
    {level::L1_0, "1.0"}, {level::L1_0b, "1.0b"}, {level::L1_1, "1.1"}, {level::L1_2, "1.2"}, {level::L1_3, "1.3"},
    {level::L2_0, "2.0"}, {level::L2_1, "2.1"},   {level::L2_2, "2.2"}, {level::L3_0, "3.0"}, {level::L3_1, "3.1"},
    {level::L3_2, "3.2"}, {level::L4_0, "4.0"},   {level::L4_1, "4.1"}, {level::L4_2, "4.2"}, {level::L5_0, "5.0"},
    {level::L5_1, "5.1"}, {level::L5_2, "5.2"},
};

INITIALIZER(nvenc_h264_handler_init)
{
	obsffmpeg::initializers.push_back([]() {
		obsffmpeg::register_codec_handler("h264_nvenc", std::make_shared<obsffmpeg::ui::nvenc_h264_handler>());
	});
};

void obsffmpeg::ui::nvenc_h264_handler::get_defaults(obs_data_t* settings, AVCodec* codec, AVCodecContext* context)
{
	nvenc::get_defaults(settings, codec, context);

	obs_data_set_default_int(settings, P_H264_PROFILE, static_cast<int64_t>(codecs::h264::profile::HIGH));
	obs_data_set_default_int(settings, P_H264_LEVEL, static_cast<int64_t>(codecs::h264::level::UNKNOWN));
}

void obsffmpeg::ui::nvenc_h264_handler::get_properties(obs_properties_t* props, AVCodec* codec, AVCodecContext* context)
{
	if (!context) {
		this->get_encoder_properties(props, codec);
	} else {
		this->get_runtime_properties(props, codec, context);
	}
}

void obsffmpeg::ui::nvenc_h264_handler::update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context)
{
	nvenc::update(settings, codec, context);

	{
		auto found =
		    profiles.find(static_cast<codecs::h264::profile>(obs_data_get_int(settings, P_H264_PROFILE)));
		if (found != profiles.end()) {
			av_opt_set(context->priv_data, "profile", found->second.c_str(), 0);
		}
	}

	{
		auto found = levels.find(static_cast<codecs::h264::level>(obs_data_get_int(settings, P_H264_LEVEL)));
		if (found != levels.end()) {
			av_opt_set(context->priv_data, "level", found->second.c_str(), 0);
		} else {
			av_opt_set(context->priv_data, "level", "auto", 0);
		}
	}
}

void obsffmpeg::ui::nvenc_h264_handler::get_encoder_properties(obs_properties_t* props, AVCodec* codec)
{
	nvenc::get_properties_pre(props, codec);

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, P_H264, TRANSLATE(P_H264), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_list(grp, P_H264_PROFILE, TRANSLATE(P_H264_PROFILE),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_H264_PROFILE)));
			obs_property_list_add_int(p, TRANSLATE(G_STATE_DEFAULT),
			                          static_cast<int64_t>(codecs::h264::profile::UNKNOWN));
			for (auto kv : profiles) {
				std::string trans = std::string(P_H264_PROFILE) + "." + kv.second;
				obs_property_list_add_int(p, TRANSLATE(trans.c_str()), static_cast<int64_t>(kv.first));
			}
		}
		{
			auto p = obs_properties_add_list(grp, P_H264_LEVEL, TRANSLATE(P_H264_LEVEL),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_H264_LEVEL)));
			obs_property_list_add_int(p, TRANSLATE(G_STATE_AUTOMATIC),
			                          static_cast<int64_t>(codecs::h264::level::UNKNOWN));
			for (auto kv : levels) {
				obs_property_list_add_int(p, kv.second.c_str(), static_cast<int64_t>(kv.first));
			}
		}
	}

	nvenc::get_properties_post(props, codec);
}

void obsffmpeg::ui::nvenc_h264_handler::get_runtime_properties(obs_properties_t* props, AVCodec* codec,
                                                               AVCodecContext* context)
{
	nvenc::get_runtime_properties(props, codec, context);
}
