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

#include "generic.hpp"
#include <iomanip>
#include <sstream>
#include <thread>
#include <util/profiler.hpp>
#include "ffmpeg/tools.hpp"
#include "plugin.hpp"
#include "utility.hpp"

extern "C" {
#include <obs-module.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#pragma warning(pop)
}

// Rate Control
#define P_RATECONTROL "RateControl"
#define P_RATECONTROL_PROFILE "RateControl.Profile"
#define P_RATECONTROL_BITRATE "RateControl.Bitrate"
#define P_RATECONTROL_MAXBITRATE "RateControl.MaxBitrate"
#define P_RATECONTROL_VBVBUFFER "RateControl.VBVBuffer"
#define P_RATECONTROL_KEYFRAME "RateControl.KeyFrame"
#define P_RATECONTROL_KEYFRAME_TYPE "RateControl.KeyFrame.Type"
#define P_RATECONTROL_KEYFRAME_INTERVAL "RateControl.KeyFrame.Interval"

// FFmpeg
#define P_FFMPEG "FFmpeg"
#define P_FFMPEG_CUSTOMSETTINGS "FFmpeg.CustomSettings"
#define P_FFMPEG_STANDARDCOMPLIANCE "FFmpeg.StandardCompliance"

enum class keyframe_type { Seconds, Frames };

encoder::generic_factory::generic_factory(AVCodec* codec) : avcodec_ptr(codec), info() {}

encoder::generic_factory::~generic_factory() {}

void encoder::generic_factory::register_encoder()
{
	// Generate unique name from given Id
	{
		std::stringstream sstr;
		sstr << "ffmpeg-" << avcodec_ptr->name << "-0x" << std::uppercase << std::setfill('0') << std::setw(8)
		     << std::hex << avcodec_ptr->capabilities;
		this->info.uid = sstr.str();
	}

	// Also generate a human readable name while we're at it.
	// TODO: Figure out a way to translate from names to other names.
	{
		std::stringstream sstr;
		sstr << "[FFmpeg] " << (avcodec_ptr->long_name ? avcodec_ptr->long_name : avcodec_ptr->name) << " ("
		     << avcodec_ptr->name << ")";
		std::string caps = ffmpeg::tools::translate_encoder_capabilities(avcodec_ptr->capabilities);
		if (caps.length() != 0) {
			sstr << " (" << caps << ")";
		}
		this->info.readable_name = sstr.str();
	}

	// Assign Ids.
	{
		const AVCodecDescriptor* desc = avcodec_descriptor_get(this->avcodec_ptr->id);
		if (desc) {
			this->info.codec = desc->name;
		} else {
			this->info.codec = avcodec_ptr->name;
		}
	}
	this->info.oei.id    = this->info.uid.c_str();
	this->info.oei.codec = this->info.codec.c_str();

	// Detect encoder type (only Video and Audio supported)
	if (avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
		this->info.oei.type = obs_encoder_type::OBS_ENCODER_VIDEO;
	} else if (avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
		this->info.oei.type = obs_encoder_type::OBS_ENCODER_AUDIO;
	} else {
		throw std::invalid_argument("unsupported codec type");
	}

	// Register functions.
	this->info.oei.create = [](obs_data_t* settings, obs_encoder_t* encoder) {
		try {
			return reinterpret_cast<void*>(new generic(settings, encoder));
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			return reinterpret_cast<void*>(0);
		} catch (...) {
			PLOG_ERROR("unknown exception");
			return reinterpret_cast<void*>(0);
		}
	};
	this->info.oei.destroy = [](void* ptr) {
		try {
			delete reinterpret_cast<generic*>(ptr);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	this->info.oei.get_name = [](void* type_data) {
		try {
			return reinterpret_cast<generic_factory*>(type_data)->get_name();
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	this->info.oei.get_defaults2 = [](obs_data_t* settings, void* type_data) {
		try {
			reinterpret_cast<generic_factory*>(type_data)->get_defaults(settings);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	this->info.oei.get_properties2 = [](void* ptr, void* type_data) {
		try {
			obs_properties_t* props = obs_properties_create();
			if (type_data != nullptr) {
				reinterpret_cast<generic_factory*>(type_data)->get_properties(props);
			}
			if (ptr != nullptr) {
				reinterpret_cast<generic*>(ptr)->get_properties(props);
			}
			return props;
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	this->info.oei.update = [](void* ptr, obs_data_t* settings) {
		try {
			return reinterpret_cast<generic*>(ptr)->update(settings);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	this->info.oei.get_sei_data = [](void* ptr, uint8_t** sei_data, size_t* size) {
		try {
			return reinterpret_cast<generic*>(ptr)->get_sei_data(sei_data, size);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	this->info.oei.get_extra_data = [](void* ptr, uint8_t** extra_data, size_t* size) {
		try {
			return reinterpret_cast<generic*>(ptr)->get_extra_data(extra_data, size);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};

	if (this->avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
		this->info.oei.get_video_info = [](void* ptr, struct video_scale_info* info) {
			try {
				reinterpret_cast<generic*>(ptr)->get_video_info(info);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		this->info.oei.encode = [](void* ptr, struct encoder_frame* frame, struct encoder_packet* packet,
		                           bool* received_packet) {
			try {
				return reinterpret_cast<generic*>(ptr)->video_encode(frame, packet, received_packet);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		this->info.oei.encode_texture = [](void* ptr, uint32_t handle, int64_t pts, uint64_t lock_key,
		                                   uint64_t* next_key, struct encoder_packet* packet,
		                                   bool* received_packet) {
			try {
				return reinterpret_cast<generic*>(ptr)->video_encode_texture(
				    handle, pts, lock_key, next_key, packet, received_packet);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};

	} else if (this->avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
		this->info.oei.get_audio_info = [](void* ptr, struct audio_convert_info* info) {
			try {
				reinterpret_cast<generic*>(ptr)->get_audio_info(info);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		this->info.oei.get_frame_size = [](void* ptr) {
			try {
				return reinterpret_cast<generic*>(ptr)->get_frame_size();
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		this->info.oei.encode = [](void* ptr, struct encoder_frame* frame, struct encoder_packet* packet,
		                           bool* received_packet) {
			try {
				return reinterpret_cast<generic*>(ptr)->audio_encode(frame, packet, received_packet);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
	}

	// Finally store ourself as type data.
	this->info.oei.type_data = this;

	obs_register_encoder(&this->info.oei);
	PLOG_INFO("Registered encoder #%llX with name '%s' and long name '%s' and caps %llX", avcodec_ptr,
	          avcodec_ptr->name, avcodec_ptr->long_name, avcodec_ptr->capabilities);
}

const char* encoder::generic_factory::get_name()
{
	return this->info.readable_name.c_str();
}

void encoder::generic_factory::get_defaults(obs_data_t* settings)
{
#ifdef GENERATE_OPTIONS
	AVCodecContext* ctx = avcodec_alloc_context3(this->avcodec_ptr);
	if (ctx->priv_data) {
		const AVOption* opt = nullptr;
		while ((opt = av_opt_next(ctx->priv_data, opt)) != nullptr) {
			if (opt->type == AV_OPT_TYPE_CONST && opt->unit != nullptr) {
				continue;
			}

			switch (opt->type) {
			case AV_OPT_TYPE_BOOL:
				obs_data_set_default_bool(settings, opt->name, !!opt->default_val.i64);
				break;
			case AV_OPT_TYPE_INT:
			case AV_OPT_TYPE_INT64:
			case AV_OPT_TYPE_UINT64:
				obs_data_set_default_int(settings, opt->name, opt->default_val.i64);
				break;
			case AV_OPT_TYPE_FLOAT:
			case AV_OPT_TYPE_DOUBLE:
				obs_data_set_default_double(settings, opt->name, opt->default_val.dbl);
				break;
			case AV_OPT_TYPE_STRING:
				obs_data_set_default_string(settings, opt->name, opt->default_val.str);
				break;
			}
		}
	}
#endif

	{ // Integrated Options
		// Rate Control
		obs_data_set_default_int(settings, P_RATECONTROL_PROFILE, 0);
		obs_data_set_default_int(settings, P_RATECONTROL_BITRATE, 2500);
		obs_data_set_default_int(settings, P_RATECONTROL_KEYFRAME_TYPE, 0);
		obs_data_set_default_double(settings, P_RATECONTROL_KEYFRAME_INTERVAL ".Seconds", 2.0);
		obs_data_set_default_int(settings, P_RATECONTROL_KEYFRAME_INTERVAL ".Frames", 300);

		// FFmpeg
		obs_data_set_default_string(settings, P_FFMPEG_CUSTOMSETTINGS, "");
		obs_data_set_default_int(settings, P_FFMPEG_STANDARDCOMPLIANCE, FF_COMPLIANCE_STRICT);
	}
}

void encoder::generic_factory::get_properties(obs_properties_t* props)
{
#ifdef GENERATE_OPTIONS
	// Encoder Options
	AVCodecContext* ctx = avcodec_alloc_context3(this->avcodec_ptr);
	if (ctx->priv_data) {
		std::map<std::string, std::pair<obs_property_t*, const AVOption*>> unit_property_map;

		const AVOption* opt = nullptr;
		while ((opt = av_opt_next(ctx->priv_data, opt)) != nullptr) {
			obs_property_t* p = nullptr;

			// Constants are parts of a unit, not actual options.
			if (opt->type == AV_OPT_TYPE_CONST) {
				auto unit = unit_property_map.find(opt->unit);
				if (unit == unit_property_map.end()) {
					continue;
				}

				switch (unit->second.second->type) {
				case AV_OPT_TYPE_INT:
				case AV_OPT_TYPE_INT64:
					obs_property_list_add_int(unit->second.first, opt->name, opt->default_val.i64);
					break;
				case AV_OPT_TYPE_FLOAT:
				case AV_OPT_TYPE_DOUBLE:
					obs_property_list_add_float(unit->second.first, opt->name,
					                            opt->default_val.dbl);
					break;
				case AV_OPT_TYPE_STRING:
					obs_property_list_add_string(unit->second.first, opt->name,
					                             opt->default_val.str);
					break;
				default:
					throw std::runtime_error("unhandled const type");
				}

				continue;
			}

			switch (opt->type) {
			case AV_OPT_TYPE_BOOL:
				p = obs_properties_add_bool(props, opt->name, opt->name);
				break;
			case AV_OPT_TYPE_INT:
			case AV_OPT_TYPE_INT64:
			case AV_OPT_TYPE_UINT64:
				if (opt->unit != nullptr) {
					p = obs_properties_add_list(props, opt->name, opt->name, OBS_COMBO_TYPE_LIST,
					                            OBS_COMBO_FORMAT_INT);
					unit_property_map.emplace(opt->unit,
					                          std::pair<obs_property_t*, const AVOption*>{p, opt});
				} else {
					p = obs_properties_add_int(props, opt->name, opt->name,
					                           static_cast<int>(opt->min),
					                           static_cast<int>(opt->max), 1);
				}
				break;
			case AV_OPT_TYPE_FLOAT:
			case AV_OPT_TYPE_DOUBLE:
				if (opt->unit != nullptr) {
					p = obs_properties_add_list(props, opt->name, opt->name, OBS_COMBO_TYPE_LIST,
					                            OBS_COMBO_FORMAT_FLOAT);
					unit_property_map.emplace(opt->unit,
					                          std::pair<obs_property_t*, const AVOption*>{p, opt});
				} else {
					p = obs_properties_add_float(props, opt->name, opt->name, opt->min, opt->max,
					                             0.01);
				}
				break;
			case AV_OPT_TYPE_STRING:
				if (opt->unit != nullptr) {
					p = obs_properties_add_list(props, opt->name, opt->name, OBS_COMBO_TYPE_LIST,
					                            OBS_COMBO_FORMAT_STRING);
					unit_property_map.emplace(opt->unit,
					                          std::pair<obs_property_t*, const AVOption*>{p, opt});
				} else {
					p = obs_properties_add_text(props, opt->name, opt->name,
					                            obs_text_type::OBS_TEXT_DEFAULT);
				}
				break;
			case AV_OPT_TYPE_FLAGS:
			case AV_OPT_TYPE_RATIONAL:
			case AV_OPT_TYPE_VIDEO_RATE:
			case AV_OPT_TYPE_BINARY:
			case AV_OPT_TYPE_DICT:
			case AV_OPT_TYPE_IMAGE_SIZE:
			case AV_OPT_TYPE_PIXEL_FMT:
			case AV_OPT_TYPE_SAMPLE_FMT:
			case AV_OPT_TYPE_DURATION:
			case AV_OPT_TYPE_COLOR:
			case AV_OPT_TYPE_CHANNEL_LAYOUT:
				PLOG_WARNING("Skipped option '%s' for codec '%s' as option type is not supported.",
				             opt->name, this->info.uid.c_str());
				break;
			}

			if ((opt->flags & AV_OPT_FLAG_READONLY) && (p != nullptr)) {
				obs_property_set_enabled(p, false);
			}
		}
	}
	avcodec_free_context(&ctx);
#endif

	// FFmpeg Options
	{ /// Rate Control
		auto prs = obs_properties_create();
		{
			auto grp = obs_properties_create();
			auto p1  = obs_properties_add_list(grp, P_RATECONTROL_KEYFRAME_TYPE,
                                                          TRANSLATE(P_RATECONTROL_KEYFRAME_TYPE), OBS_COMBO_TYPE_LIST,
                                                          OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p1, TRANSLATE(DESC(P_RATECONTROL_KEYFRAME_TYPE)));
			obs_property_set_modified_callback2(p1, modified_ratecontrol_properties, this);
			obs_property_list_add_int(p1, TRANSLATE(P_RATECONTROL_KEYFRAME_TYPE ".Seconds"),
			                          static_cast<int64_t>(keyframe_type::Seconds));
			obs_property_list_add_int(p1, TRANSLATE(P_RATECONTROL_KEYFRAME_TYPE ".Frames"),
			                          static_cast<int64_t>(keyframe_type::Frames));

			auto p2 = obs_properties_add_float(grp, P_RATECONTROL_KEYFRAME_INTERVAL ".Seconds",
			                                   TRANSLATE(P_RATECONTROL_KEYFRAME_INTERVAL), 0.0,
			                                   std::numeric_limits<double_t>::max(), 0.01);
			obs_property_set_long_description(p2, TRANSLATE(DESC(P_RATECONTROL_KEYFRAME_INTERVAL)));

			auto p3 = obs_properties_add_int(grp, P_RATECONTROL_KEYFRAME_INTERVAL ".Frames",
			                                 TRANSLATE(P_RATECONTROL_KEYFRAME_INTERVAL), 0,
			                                 std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p3, TRANSLATE(DESC(P_RATECONTROL_KEYFRAME_INTERVAL)));
			auto gp = obs_properties_add_group(prs, P_RATECONTROL_KEYFRAME,
			                                   TRANSLATE(P_RATECONTROL_KEYFRAME), OBS_GROUP_NORMAL, grp);

			obs_property_set_visible(gp, !(this->avcodec_ptr->capabilities & AV_CODEC_CAP_INTRA_ONLY));
		}
		{
			auto p = obs_properties_add_int(prs, P_RATECONTROL_BITRATE, TRANSLATE(P_RATECONTROL_BITRATE), 1,
			                                std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BITRATE)));
		}
		{
			auto p = obs_properties_add_list(prs, P_RATECONTROL_PROFILE, TRANSLATE(P_RATECONTROL_PROFILE),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			if (this->avcodec_ptr->profiles) {
				auto profile = this->avcodec_ptr->profiles;
				obs_property_list_add_int(p, TRANSLATE(P_RATECONTROL_PROFILE ".None"),
				                          FF_PROFILE_UNKNOWN);
				while (profile->profile != FF_PROFILE_UNKNOWN) {
					obs_property_list_add_int(p, profile->name, profile->profile);
					profile++;
				}
			}
		}
		obs_properties_add_group(props, P_RATECONTROL, TRANSLATE(P_RATECONTROL), OBS_GROUP_NORMAL, prs);
	};
	{
		auto prs = obs_properties_create();
		{
			auto p =
			    obs_properties_add_text(prs, P_FFMPEG_CUSTOMSETTINGS, TRANSLATE(P_FFMPEG_CUSTOMSETTINGS),
			                            obs_text_type::OBS_TEXT_DEFAULT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_FFMPEG_CUSTOMSETTINGS)));
		}
		{
			auto p = obs_properties_add_list(prs, P_FFMPEG_STANDARDCOMPLIANCE,
			                                 TRANSLATE(P_FFMPEG_STANDARDCOMPLIANCE), OBS_COMBO_TYPE_LIST,
			                                 OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_FFMPEG_STANDARDCOMPLIANCE)));
			obs_property_list_add_int(p, TRANSLATE(P_FFMPEG_STANDARDCOMPLIANCE ".VeryStrict"),
			                          FF_COMPLIANCE_VERY_STRICT);
			obs_property_list_add_int(p, TRANSLATE(P_FFMPEG_STANDARDCOMPLIANCE ".Strict"),
			                          FF_COMPLIANCE_STRICT);
			obs_property_list_add_int(p, TRANSLATE(P_FFMPEG_STANDARDCOMPLIANCE ".Normal"),
			                          FF_COMPLIANCE_NORMAL);
			obs_property_list_add_int(p, TRANSLATE(P_FFMPEG_STANDARDCOMPLIANCE ".Unofficial"),
			                          FF_COMPLIANCE_UNOFFICIAL);
			obs_property_list_add_int(p, TRANSLATE(P_FFMPEG_STANDARDCOMPLIANCE ".Experimental"),
			                          FF_COMPLIANCE_EXPERIMENTAL);
		}
		obs_properties_add_group(props, P_FFMPEG, TRANSLATE(P_FFMPEG), OBS_GROUP_NORMAL, prs);
	};
}

AVCodec* encoder::generic_factory::get_avcodec()
{
	return this->avcodec_ptr;
}

bool encoder::generic_factory::modified_ratecontrol_properties(void*, obs_properties_t* props, obs_property_t*,
                                                               obs_data_t* settings)
{
	keyframe_type kft = static_cast<keyframe_type>(obs_data_get_int(settings, P_RATECONTROL_KEYFRAME_TYPE));
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_KEYFRAME_INTERVAL ".Seconds"),
	                         kft == keyframe_type::Seconds);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_KEYFRAME_INTERVAL ".Frames"),
	                         kft == keyframe_type::Frames);
	return true;
}

encoder::generic::generic(obs_data_t* settings, obs_encoder_t* encoder)
    : self(encoder), lag_in_frames(0), frame_count(0)
{
	this->factory = reinterpret_cast<generic_factory*>(obs_encoder_get_type_data(self));

	// Verify that the codec actually still exists.
	this->codec = avcodec_find_encoder(this->factory->get_avcodec()->id);
	if (!this->codec) {
		PLOG_ERROR("Failed to find encoder for codec '%s'.", this->factory->get_avcodec()->name);
		throw std::runtime_error("failed to find codec");
	}

	// Initialize context.
	this->context = avcodec_alloc_context3(this->codec);
	if (!this->context) {
		PLOG_ERROR("Failed to create context for encoder '%s'.", this->codec->name);
		throw std::runtime_error("failed to create context");
	}

	// Settings
	/// Rate Control
	this->context->strict_std_compliance =
	    static_cast<int>(obs_data_get_int(settings, P_FFMPEG_STANDARDCOMPLIANCE));
	this->context->debug = 0;
	/// Threading
	if (this->codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
		this->context->thread_type = FF_THREAD_SLICE;
	} else if (this->codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) {
		this->context->thread_type = FF_THREAD_FRAME;
	} else {
		this->context->thread_type = 0;
	}
	if (this->codec->capabilities & AV_CODEC_CAP_AUTO_THREADS) {
		this->context->thread_count = 0;
		this->lag_in_frames         = std::thread::hardware_concurrency();
	} else {
		this->context->thread_count = std::thread::hardware_concurrency();
		this->lag_in_frames         = this->context->thread_count;
	}

	// Video and Audio exclusive setup
	if (this->codec->type == AVMEDIA_TYPE_VIDEO) {
		// FFmpeg Video Settings
		auto encvideo = obs_encoder_video(this->self);
		auto voi      = video_output_get_info(encvideo);

		// Resolution
		this->context->width  = voi->width;
		this->context->height = voi->height;
		this->swscale.set_source_size(this->context->width, this->context->height);
		this->swscale.set_target_size(this->context->width, this->context->height);

		// Color
		this->context->colorspace  = ffmpeg::tools::obs_videocolorspace_to_avcolorspace(voi->colorspace);
		this->context->color_range = ffmpeg::tools::obs_videorangetype_to_avcolorrange(voi->range);
		this->context->field_order = AV_FIELD_PROGRESSIVE;
		this->swscale.set_source_color(this->context->color_range, this->context->colorspace);
		this->swscale.set_target_color(this->context->color_range, this->context->colorspace);

		// Pixel Format
		{
			// Due to unsupported color formats and ffmpeg not automatically converting formats from A to B,
			// we have to detect the closest format that we can still use and initialize our swscale instance
			// these formats. This has a massive cost attached unfortunately.

			AVPixelFormat source = ffmpeg::tools::obs_videoformat_to_avpixelformat(voi->format);
			AVPixelFormat target = AV_PIX_FMT_NONE;

			int loss = 0;
			target   = avcodec_find_best_pix_fmt_of_list(this->codec->pix_fmts, source, false, &loss);

			this->context->pix_fmt = target;
			this->swscale.set_source_format(source);
			this->swscale.set_target_format(this->context->pix_fmt);
		}

		// Framerate
		this->context->time_base.num   = voi->fps_num;
		this->context->time_base.den   = voi->fps_den;
		this->context->ticks_per_frame = 1;

		/// Group of Pictures
		if (static_cast<keyframe_type>(obs_data_get_int(settings, P_RATECONTROL_KEYFRAME_TYPE))
		    == keyframe_type::Frames) {
			this->context->gop_size =
			    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_KEYFRAME_INTERVAL ".Frames"));
		} else {
			double_t real_gop = obs_data_get_double(settings, P_RATECONTROL_KEYFRAME_INTERVAL ".Seconds");
			this->context->gop_size = static_cast<int>(real_gop * voi->fps_num / voi->fps_den);
		}

	} else if (this->codec->type == AVMEDIA_TYPE_AUDIO) {
	}

	// Update settings
	this->update(settings);

	// Initialize
	int res = avcodec_open2(this->context, this->codec, NULL);
	if (res < 0) {
		PLOG_ERROR("Failed to initialize encoder '%s' due to error code %lld: %s", this->codec->name, res,
		           ffmpeg::tools::get_error_description(res));
		throw std::runtime_error(ffmpeg::tools::get_error_description(res));
	}

	// Video/Audio exclusive setup part 2.
	if (this->codec->type == AVMEDIA_TYPE_VIDEO) {
		// Create Scaler
		if (!swscale.initialize(SWS_FAST_BILINEAR)) {
			PLOG_ERROR(
			    " Failed to initialize Software Scaler for pixel format '%s' with color space '%s' and "
			    "range '%s'.",
			    ffmpeg::tools::get_pixel_format_name(this->context->pix_fmt),
			    ffmpeg::tools::get_color_space_name(this->context->colorspace),
			    this->swscale.is_source_full_range() ? "Full" : "Partial");
			throw std::runtime_error("failed to initialize swscaler.");
		}

		// Create Frame queue
		this->frame_queue.set_pixel_format(this->context->pix_fmt);
		this->frame_queue.set_resolution(this->context->width, this->context->height);
		this->frame_queue.precache(std::thread::hardware_concurrency());
	} else if (this->codec->type == AVMEDIA_TYPE_AUDIO) {
	}

	// Create Packet
	this->current_packet = av_packet_alloc();
	if (!this->current_packet) {
		PLOG_ERROR("Failed to allocate packet storage.");
		throw std::runtime_error("Failed to allocate packet storage.");
	}
}

encoder::generic::~generic()
{
	this->frame_queue.clear();
	this->frame_queue_used.clear();
	this->swscale.finalize();

	if (this->context) {
		avcodec_close(this->context);
		avcodec_free_context(&this->context);
	}
}

void encoder::generic::get_properties(obs_properties_t*) {}

bool encoder::generic::update(obs_data_t* settings)
{
#ifdef GENERATE_OPTIONS
	// Apply UI options.
	if (this->context->priv_data) {
		const AVOption* opt = nullptr;
		while ((opt = av_opt_next(this->context->priv_data, opt)) != nullptr) {
			if (opt->type == AV_OPT_TYPE_CONST && opt->unit != nullptr) {
				continue;
			}

			switch (opt->type) {
			case AV_OPT_TYPE_BOOL:
				av_opt_set_int(this->context->priv_data, opt->name,
				               obs_data_get_bool(settings, opt->name) ? 1 : 0, 0);
				break;
			case AV_OPT_TYPE_INT:
			case AV_OPT_TYPE_INT64:
			case AV_OPT_TYPE_UINT64:
				av_opt_set_int(this->context->priv_data, opt->name,
				               obs_data_get_int(settings, opt->name), 0);
				break;
			case AV_OPT_TYPE_FLOAT:
			case AV_OPT_TYPE_DOUBLE:
				av_opt_set_double(this->context->priv_data, opt->name,
				                  obs_data_get_double(settings, opt->name), 0);
				break;
			case AV_OPT_TYPE_STRING:
				av_opt_set(this->context->priv_data, opt->name,
				           obs_data_get_string(settings, opt->name), 0);
				break;
			}
		}
	}
#endif

	// Apply default options
	this->context->profile  = static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_PROFILE));
	this->context->bit_rate = static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BITRATE));

	// Apply custom options.
	av_opt_set_from_string(this->context, obs_data_get_string(settings, P_FFMPEG_CUSTOMSETTINGS), nullptr, ";",
	                       "=");
	return false;
}

void encoder::generic::get_audio_info(audio_convert_info*) {}

size_t encoder::generic::get_frame_size()
{
	return size_t();
}

bool encoder::generic::audio_encode(encoder_frame*, encoder_packet*, bool*)
{
	return false;
}

void encoder::generic::get_video_info(video_scale_info*) {}

bool encoder::generic::get_sei_data(uint8_t**, size_t*)
{
	return false;
}

bool encoder::generic::get_extra_data(uint8_t** extra_data, size_t* size)
{
	if (!this->context->extradata) {
		return false;
	}
	*extra_data = this->context->extradata;
	*size       = this->context->extradata_size;
	return true;
}

bool encoder::generic::video_encode(encoder_frame* frame, encoder_packet* packet, bool* received_packet)
{
	// Try and receive packet early.
	{
		ScopeProfiler profile_inner("recieve_early");

		int res = receive_packet(received_packet, packet);
		switch (res) {
		case 0:
		case AVERROR(EAGAIN):
		case AVERROR(EOF):
			break;
		default:
			PLOG_ERROR("Failed to receive packet: %s (%ld).", ffmpeg::tools::get_error_description(res),
			           res);
			return false;
		}
	}

	// Convert frame.
	std::shared_ptr<AVFrame> vframe = frame_queue.pop(); // Retrieve an empty frame.
	{
		ScopeProfiler profile("convert");

		vframe->color_range = this->context->color_range;
		vframe->colorspace  = this->context->colorspace;

		int res =
		    swscale.convert(reinterpret_cast<uint8_t**>(frame->data), reinterpret_cast<int*>(frame->linesize),
		                    0, this->context->height, vframe->data, vframe->linesize);
		if (res <= 0) {
			PLOG_ERROR("Failed to convert frame: %s (%ld).", ffmpeg::tools::get_error_description(res),
			           res);
			return false;
		}
	}

	// Send a new frame.
	{
		ScopeProfiler profile_inner("send");

		vframe->pts = frame->pts;

		int res = send_frame(vframe);
		switch (res) {
		case 0:
			break;
		case AVERROR(EAGAIN):
			// This means we should call receive_packet again, but what do we do with that data?
			// Why can't we queue on both? Do I really have to implement threading for this stuff?
			if (*received_packet == true) {
				PLOG_ERROR(
				    "Encoder wanted us to retrieve a packet before allowing us to submit more. Skipped "
				    "frame.");
			}
			break;
		case AVERROR(EOF):
			break;
		default:
			PLOG_ERROR("Failed to encode frame: %s (%ld).", ffmpeg::tools::get_error_description(res), res);
			return false;
		}
	}

	// Try and receive packet late.
	{
		ScopeProfiler profile_inner("recieve_late");

		bool should_lag = (lag_in_frames - frame_count) <= 0;
		bool early_exit = false;

		while ((should_lag && !*received_packet) && !early_exit) {
			int res = receive_packet(received_packet, packet);
			switch (res) {
			case 0:
				break;
			case AVERROR(EAGAIN):
			case AVERROR(EOF):
				early_exit = true;
				break;
			default:
				PLOG_ERROR("Failed to receive packet: %s (%ld).",
				           ffmpeg::tools::get_error_description(res), res);
				return false;
			}

			if (!*received_packet) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}

	return true;
}

bool encoder::generic::video_encode_texture(uint32_t, int64_t, uint64_t, uint64_t*, encoder_packet*, bool*)
{
	return false;
}

int encoder::generic::receive_packet(bool* received_packet, struct encoder_packet* packet)
{
	int res = avcodec_receive_packet(this->context, this->current_packet);
	if (res == 0) {
		packet->type          = OBS_ENCODER_VIDEO;
		packet->pts           = this->current_packet->pts;
		packet->dts           = this->current_packet->pts;
		packet->data          = this->current_packet->data;
		packet->size          = this->current_packet->size;
		packet->keyframe      = !!(this->current_packet->flags & AV_PKT_FLAG_KEY);
		packet->drop_priority = 0;
		*received_packet      = true;

		{
			std::shared_ptr<AVFrame> uframe = frame_queue_used.pop_only();
			if (frame_queue.empty()) {
				frame_queue.push(uframe);
			}
		}
	}
	return res;
}

int encoder::generic::send_frame(std::shared_ptr<AVFrame> frame)
{
	int res = avcodec_send_frame(this->context, frame.get());
	switch (res) {
	case 0:
		frame_count++;
	case AVERROR(EAGAIN):
	case AVERROR(EOF):
		break;
	}
	return res;
}
