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
#include <obs-module.h>
#include <obs.h>
#include "utility.hpp"

#include "encoders/prores_aw.hpp"

extern "C" {
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#pragma warning(pop)
}

MODULE_EXPORT bool obs_module_load(void)
{
	try {
		avcodec_register_all();
		obsffmpeg::encoder::prores_aw::initialize();
		return true;
	} catch (std::exception ex) {
		PLOG_ERROR("Exception during initalization: %s.", ex.what());
	} catch (...) {
		PLOG_ERROR("Unrecognized exception during initalization.");
	}
	return false;
}

MODULE_EXPORT void obs_module_unload(void)
{
	try {
		obsffmpeg::encoder::prores_aw::finalize();
	} catch (std::exception ex) {
		PLOG_ERROR("Exception during finalizing: %s.", ex.what());
	} catch (...) {
		PLOG_ERROR("Unrecognized exception during finalizing.");
	}
}
