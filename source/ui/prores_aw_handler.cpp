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

#include "prores_aw_handler.hpp"
#include "plugin.hpp"
#include "utility.hpp"

extern "C" {
#include <obs-module.h>
}

#define P_PROFILE "AppleProRes.Profile"
#define P_PROFILE_APCO "AppleProRes.Profile.APCO"
#define P_PROFILE_APCS "AppleProRes.Profile.APCS"
#define P_PROFILE_APCN "AppleProRes.Profile.APCN"
#define P_PROFILE_APCH "AppleProRes.Profile.APCH"
#define P_PROFILE_AP4H "AppleProRes.Profile.AP4H"

INITIALIZER(prores_aw_handler_init)
{
	obsffmpeg::initializers.push_back([]() {
		obsffmpeg::register_codec_handler("prores_aw", std::make_shared<obsffmpeg::ui::prores_aw_handler>());
	});
};

void obsffmpeg::ui::prores_aw_handler::get_defaults(obs_data_t* settings, AVCodec*, AVCodecContext*)
{
	obs_data_set_default_int(settings, P_PROFILE, 0);
}

void obsffmpeg::ui::prores_aw_handler::get_properties(obs_properties_t* props, AVCodec* codec, AVCodecContext*)
{
	{
		auto p = obs_properties_add_list(props, P_PROFILE, TRANSLATE(P_PROFILE), OBS_COMBO_TYPE_LIST,
		                                 OBS_COMBO_FORMAT_INT);
		obs_property_set_long_description(p, TRANSLATE(DESC(P_PROFILE)));
		for (auto ptr = codec->profiles; ptr->profile != FF_PROFILE_UNKNOWN; ptr++) {
			if (strcmp("apco", ptr->name) == 0) {
				obs_property_list_add_int(p, TRANSLATE(P_PROFILE_APCO),
				                          static_cast<int64_t>(ptr->profile));
			} else if (strcmp("apcs", ptr->name) == 0) {
				obs_property_list_add_int(p, TRANSLATE(P_PROFILE_APCS),
				                          static_cast<int64_t>(ptr->profile));
			} else if (strcmp("apcn", ptr->name) == 0) {
				obs_property_list_add_int(p, TRANSLATE(P_PROFILE_APCN),
				                          static_cast<int64_t>(ptr->profile));
			} else if (strcmp("apch", ptr->name) == 0) {
				obs_property_list_add_int(p, TRANSLATE(P_PROFILE_APCH),
				                          static_cast<int64_t>(ptr->profile));
			} else if (strcmp("ap4h", ptr->name) == 0) {
				obs_property_list_add_int(p, TRANSLATE(P_PROFILE_AP4H),
				                          static_cast<int64_t>(ptr->profile));
			} else {
				obs_property_list_add_int(p, ptr->name, ptr->profile);
			}
		}
	}
}

void obsffmpeg::ui::prores_aw_handler::update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context)
{
	context->profile = obs_data_get_int(settings, P_PROFILE);
}
