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

#include "debug_handler.hpp"
#include <map>
#include <string>
#include <utility>
#include "handler.hpp"
#include "plugin.hpp"
#include "utility.hpp"

extern "C" {
#include <obs-properties.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavutil/opt.h>
#pragma warning(pop)
}

void obsffmpeg::ui::debug_handler::get_defaults(obs_data_t* settings, AVCodec* codec, AVCodecContext* context) {}

template<typename T>
std::string to_string(T value){};

template<>
std::string to_string(int64_t value)
{
	std::vector<char> buf(32);
	snprintf(buf.data(), buf.size(), "%lld", value);
	return std::string(buf.data(), buf.data() + buf.size());
}

template<>
std::string to_string(uint64_t value)
{
	std::vector<char> buf(32);
	snprintf(buf.data(), buf.size(), "%llu", value);
	return std::string(buf.data(), buf.data() + buf.size());
}

template<>
std::string to_string(double_t value)
{
	std::vector<char> buf(32);
	snprintf(buf.data(), buf.size(), "%f", value);
	return std::string(buf.data(), buf.data() + buf.size());
}

void obsffmpeg::ui::debug_handler::get_properties(obs_properties_t* props, AVCodec* codec, AVCodecContext* context)
{
	if (context)
		return;

	AVCodecContext* ctx = avcodec_alloc_context3(codec);
	if (!ctx->priv_data) {
		avcodec_free_context(&ctx);
		return;
	}

	PLOG_INFO("Options for '%s':", codec->name);

	std::pair<AVOptionType, std::string> opt_type_name[] = {
	    {AV_OPT_TYPE_FLAGS, "Flags"},
	    {AV_OPT_TYPE_INT, "Int"},
	    {AV_OPT_TYPE_INT64, "Int64"},
	    {AV_OPT_TYPE_DOUBLE, "Double"},
	    {AV_OPT_TYPE_FLOAT, "Float"},
	    {AV_OPT_TYPE_STRING, "String"},
	    {AV_OPT_TYPE_RATIONAL, "Rational"},
	    {AV_OPT_TYPE_BINARY, "Binary"},
	    {AV_OPT_TYPE_DICT, "Dictionary"},
	    {AV_OPT_TYPE_UINT64, "Unsigned Int64"},
	    {AV_OPT_TYPE_CONST, "Constant"},
	    {AV_OPT_TYPE_IMAGE_SIZE, "Image Size"},
	    {AV_OPT_TYPE_PIXEL_FMT, "Pixel Format"},
	    {AV_OPT_TYPE_SAMPLE_FMT, "Sample Format"},
	    {AV_OPT_TYPE_VIDEO_RATE, "Video Rate"},
	    {AV_OPT_TYPE_DURATION, "Duration"},
	    {AV_OPT_TYPE_COLOR, "Color"},
	    {AV_OPT_TYPE_CHANNEL_LAYOUT, "Layout"},
	    {AV_OPT_TYPE_BOOL, "Bool"},
	};
	std::map<std::string, AVOptionType> unit_types;

	const AVOption* opt = nullptr;
	while ((opt = av_opt_next(ctx->priv_data, opt)) != nullptr) {
		std::string type_name = "";
		for (auto kv : opt_type_name) {
			if (opt->type == kv.first) {
				type_name = kv.second;
				break;
			}
		}

		if (opt->type == AV_OPT_TYPE_CONST) {
			if (opt->unit == nullptr) {
				PLOG_INFO("  Constant '%s' and help text '%s' with unknown settings.", opt->name,
				          opt->help);
			} else {
				auto unit_type = unit_types.find(opt->unit);
				if (unit_type == unit_types.end()) {
					PLOG_INFO("  [%s] Flag '%s' and help text '%s' with value '%lld'.", opt->unit,
					          opt->name, opt->help, opt->default_val.i64);
				} else {
					std::string out;
					switch (unit_type->second) {
					case AV_OPT_TYPE_BOOL:
						out = opt->default_val.i64 ? "true" : "false";
						break;
					case AV_OPT_TYPE_INT:
						out = to_string(opt->default_val.i64);
						break;
					case AV_OPT_TYPE_UINT64:
						out = to_string(static_cast<uint64_t>(opt->default_val.i64));
						break;
					case AV_OPT_TYPE_FLAGS:
						out = to_string(static_cast<uint64_t>(opt->default_val.i64));
						break;
					case AV_OPT_TYPE_FLOAT:
					case AV_OPT_TYPE_DOUBLE:
						out = to_string(opt->default_val.dbl);
						break;
					case AV_OPT_TYPE_STRING:
						out = opt->default_val.str;
						break;
					case AV_OPT_TYPE_BINARY:
					case AV_OPT_TYPE_IMAGE_SIZE:
					case AV_OPT_TYPE_PIXEL_FMT:
					case AV_OPT_TYPE_SAMPLE_FMT:
					case AV_OPT_TYPE_VIDEO_RATE:
					case AV_OPT_TYPE_DURATION:
					case AV_OPT_TYPE_COLOR:
					case AV_OPT_TYPE_CHANNEL_LAYOUT:
						break;
					}

					PLOG_INFO("  [%s] Constant '%s' and help text '%s' with value '%s'.", opt->unit,
					          opt->name, opt->help, out.c_str());
				}
			}
		} else {
			if (opt->unit != nullptr) {
				unit_types.emplace(opt->name, opt->type);
			}

			std::string minimum = "", maximum = "", out;
			minimum = to_string(opt->min);
			maximum = to_string(opt->max);
			{
				switch (opt->type) {
				case AV_OPT_TYPE_BOOL:
					out = opt->default_val.i64 ? "true" : "false";
					break;
				case AV_OPT_TYPE_INT:
					out = to_string(opt->default_val.i64);
					break;
				case AV_OPT_TYPE_UINT64:
					out = to_string(static_cast<uint64_t>(opt->default_val.i64));
					break;
				case AV_OPT_TYPE_FLAGS:
					out = to_string(static_cast<uint64_t>(opt->default_val.i64));
					break;
				case AV_OPT_TYPE_FLOAT:
				case AV_OPT_TYPE_DOUBLE:
					out = to_string(opt->default_val.dbl);
					break;
				case AV_OPT_TYPE_STRING:
					out = opt->default_val.str ? opt->default_val.str : "<invalid>";
					break;
				case AV_OPT_TYPE_BINARY:
				case AV_OPT_TYPE_IMAGE_SIZE:
				case AV_OPT_TYPE_PIXEL_FMT:
				case AV_OPT_TYPE_SAMPLE_FMT:
				case AV_OPT_TYPE_VIDEO_RATE:
				case AV_OPT_TYPE_DURATION:
				case AV_OPT_TYPE_COLOR:
				case AV_OPT_TYPE_CHANNEL_LAYOUT:
					break;
				}
			}

			PLOG_INFO(
			    "  Option '%s'%s%s%s with help '%s' of type '%s' with default value '%s', minimum '%s' and maximum '%s'.",
			    opt->name, opt->unit ? " with unit (" : "", opt->unit ? opt->unit : "",
			    opt->unit ? ")" : "", opt->help, type_name.c_str(), out.c_str(), minimum.c_str(),
			    maximum.c_str());
		}
	}
}

void obsffmpeg::ui::debug_handler::update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context) {}
