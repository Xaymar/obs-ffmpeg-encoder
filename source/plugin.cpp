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

#include "plugin.hpp"
#include <memory>
#include <obs-module.h>
#include <obs.h>
#include "utility.hpp"

#include "encoders/generic.hpp"
#include "encoders/prores_aw.hpp"

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#pragma warning(pop)
}

// Initializers and finalizers.
std::list<std::function<void()>> obsffmpeg::initializers;

std::list<std::function<void()>> obsffmpeg::finalizers;

// Codec to Handler mapping.
static std::map<std::string, std::shared_ptr<obsffmpeg::ui::handler>> codec_to_handler_map;

void obsffmpeg::register_codec_handler(std::string codec, std::shared_ptr<obsffmpeg::ui::handler> handler)
{
	codec_to_handler_map.emplace(codec, handler);
}

std::shared_ptr<obsffmpeg::ui::handler> obsffmpeg::find_codec_handler(std::string codec)
{
	auto found = codec_to_handler_map.find(codec);
	if (found == codec_to_handler_map.end())
		return nullptr;
	return found->second;
}

static std::map<AVCodec*, std::shared_ptr<encoder::generic_factory>> generic_factories;

MODULE_EXPORT bool obs_module_load(void)
try {
	// Initialize avcodec.
	avcodec_register_all();

	// Run all initializers.
	for (auto func : obsffmpeg::initializers) {
		func();
	}

	// Register all codecs.
	AVCodec* cdc = nullptr;
	while ((cdc = av_codec_next(cdc)) != nullptr) {
		if (!av_codec_is_encoder(cdc))
			continue;

		if ((cdc->type == AVMediaType::AVMEDIA_TYPE_AUDIO) || (cdc->type == AVMediaType::AVMEDIA_TYPE_VIDEO)) {
			auto ptr = std::make_shared<encoder::generic_factory>(cdc);
			ptr->register_encoder();
			generic_factories.emplace(cdc, ptr);
		}
	}

	obsffmpeg::encoder::prores_aw::initialize();
	return true;
} catch (std::exception ex) {
	PLOG_ERROR("Exception during initalization: %s.", ex.what());
	return false;
} catch (...) {
	PLOG_ERROR("Unrecognized exception during initalization.");
	return false;
}

MODULE_EXPORT void obs_module_unload(void)
try {
	obsffmpeg::encoder::prores_aw::finalize();

	// Run all finalizers.
	for (auto func : obsffmpeg::finalizers) {
		func();
	}
} catch (std::exception ex) {
	PLOG_ERROR("Exception during finalizing: %s.", ex.what());
} catch (...) {
	PLOG_ERROR("Unrecognized exception during finalizing.");
}
