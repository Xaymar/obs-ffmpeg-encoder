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
#include "handler.hpp"

extern "C" {
#include <obs-properties.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#pragma warning(pop)
}

namespace obsffmpeg {
	namespace ui {
		class libvpx_vp9_handler : public handler {
			public:
			virtual void get_defaults(obs_data_t* settings, AVCodec* codec,
			                          AVCodecContext* context) override;

			void get_properties_codec(obs_properties_t* props, AVCodec* codec);

			void get_properties_encoder(obs_properties_t* props, AVCodec* codec, AVCodecContext* context);

			virtual void get_properties(obs_properties_t* props, AVCodec* codec,
			                            AVCodecContext* context) override;

			virtual void update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context) override;

			public:
			static bool modified_ratecontrol(obs_properties_t* props, obs_property_t* property,
			                                 obs_data_t* settings);

			static bool modified_keyframes(obs_properties_t* props, obs_property_t* property,
			                               obs_data_t* settings);
		};
	} // namespace ui
} // namespace obsffmpeg
