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

#include "nvenc_hevc_handler.hpp"
#include "codecs/hevc.hpp"
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

INITIALIZER(nvenc_hevc_handler_init)
{
	obsffmpeg::initializers.push_back([]() {
		obsffmpeg::register_codec_handler("hevc_nvenc", std::make_shared<obsffmpeg::ui::nvenc_hevc_handler>());
	});
};

void obsffmpeg::ui::nvenc_hevc_handler::get_defaults(obs_data_t* settings, AVCodec* codec, AVCodecContext* context)
{
	nvenc::get_defaults(settings, codec, context);

	obs_data_set_default_int(settings, P_HEVC_PROFILE, static_cast<int64_t>(codecs::hevc::profile::MAIN));
	obs_data_set_default_int(settings, P_HEVC_TIER, static_cast<int64_t>(codecs::hevc::profile::MAIN));
	obs_data_set_default_int(settings, P_HEVC_LEVEL, static_cast<int64_t>(codecs::hevc::level::UNKNOWN));
}

void obsffmpeg::ui::nvenc_hevc_handler::get_properties(obs_properties_t* props, AVCodec* codec, AVCodecContext* context)
{
	if (!context) {
		this->get_encoder_properties(props, codec);
	} else {
		this->get_runtime_properties(props, codec, context);
	}
}

void obsffmpeg::ui::nvenc_hevc_handler::update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context)
{
	nvenc::update(settings, codec, context);

	{ // HEVC Options
		codecs::hevc::profile profile =
		    static_cast<codecs::hevc::profile>(obs_data_get_int(settings, P_HEVC_PROFILE));
		switch (profile) {
		case codecs::hevc::profile::MAIN:
		case codecs::hevc::profile::MAIN10:
		case codecs::hevc::profile::RANGE_EXTENDED:
			av_opt_set_int(context->priv_data, "profile", static_cast<int64_t>(profile), 0);
			break;
		default:
			av_opt_set_int(context->priv_data, "profile", static_cast<int64_t>(codecs::hevc::profile::MAIN),
			               0);
			break;
		}

		codecs::hevc::tier tier = static_cast<codecs::hevc::tier>(obs_data_get_int(settings, P_HEVC_TIER));
		switch (tier) {
		case codecs::hevc::tier::MAIN:
		case codecs::hevc::tier::HIGH:
			av_opt_set_int(context->priv_data, "tier", static_cast<int64_t>(tier), 0);
			break;
		default:
			av_opt_set_int(context->priv_data, "tier", static_cast<int64_t>(codecs::hevc::tier::MAIN), 0);
			break;
		}

		codecs::hevc::level level = static_cast<codecs::hevc::level>(obs_data_get_int(settings, P_HEVC_LEVEL));
		if (level != codecs::hevc::level::UNKNOWN) {
			av_opt_set_int(context->priv_data, "level", static_cast<int64_t>(level), 0);
		} else {
			av_opt_set_int(context->priv_data, "level", static_cast<int64_t>(0), 0); // Automatic
		}
	}
}

void obsffmpeg::ui::nvenc_hevc_handler::get_encoder_properties(obs_properties_t* props, AVCodec* codec)
{
	nvenc::get_properties_pre(props, codec);

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, P_HEVC, TRANSLATE(P_HEVC), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_list(grp, P_HEVC_PROFILE, TRANSLATE(P_HEVC_PROFILE),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_HEVC_PROFILE)));
			obs_property_list_add_int(p, TRANSLATE(G_STATE_DEFAULT),
			                          static_cast<int64_t>(codecs::hevc::profile::UNKNOWN));
			for (auto kv : codecs::hevc::profiles) {
				std::string trans = std::string(P_HEVC_PROFILE) + "." + kv.second;
				obs_property_list_add_int(p, TRANSLATE(trans.c_str()), static_cast<int64_t>(kv.first));
			}
		}
		{
			auto p = obs_properties_add_list(grp, P_HEVC_TIER, TRANSLATE(P_HEVC_TIER), OBS_COMBO_TYPE_LIST,
			                                 OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_HEVC_TIER)));
			obs_property_list_add_int(p, TRANSLATE(G_STATE_DEFAULT),
			                          static_cast<int64_t>(codecs::hevc::tier::UNKNOWN));
			for (auto kv : codecs::hevc::profile_tiers) {
				std::string trans = std::string(P_HEVC_TIER) + "." + kv.second;
				obs_property_list_add_int(p, TRANSLATE(trans.c_str()), static_cast<int64_t>(kv.first));
			}
		}
		{
			auto p = obs_properties_add_list(grp, P_HEVC_LEVEL, TRANSLATE(P_HEVC_LEVEL),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_HEVC_LEVEL)));
			obs_property_list_add_int(p, TRANSLATE(G_STATE_AUTOMATIC),
			                          static_cast<int64_t>(codecs::hevc::level::UNKNOWN));
			for (auto kv : codecs::hevc::levels) {
				obs_property_list_add_int(p, kv.second.c_str(), static_cast<int64_t>(kv.first));
			}
		}
	}

	nvenc::get_properties_post(props, codec);
}

void obsffmpeg::ui::nvenc_hevc_handler::get_runtime_properties(obs_properties_t* props, AVCodec* codec,
                                                               AVCodecContext* context)
{
	nvenc::get_runtime_properties(props, codec, context);
}
