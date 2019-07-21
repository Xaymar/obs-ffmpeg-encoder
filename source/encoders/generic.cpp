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
#include <libavutil/dict.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#pragma warning(pop)
}

// Generic
#define P_AUTOMATIC "Automatic"

// FFmpeg
#define P_FFMPEG "FFmpeg"
#define P_FFMPEG_CUSTOMSETTINGS "FFmpeg.CustomSettings"
#define P_FFMPEG_THREADS "FFmpeg.Threads"
#define P_FFMPEG_COLORFORMAT "FFmpeg.ColorFormat"
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
		sstr << (avcodec_ptr->long_name ? avcodec_ptr->long_name : avcodec_ptr->name) << " ("
		     << avcodec_ptr->name << ")";
		std::string caps = ffmpeg::tools::translate_encoder_capabilities(avcodec_ptr->capabilities);
		if (caps.length() != 0) {
			sstr << " [" << caps << "]";
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

	// Is this a deprecated encoder?
#ifndef _DEBUG
	if (!obsffmpeg::has_codec_handler(avcodec_ptr->name)) {
		this->info.oei.caps |= OBS_ENCODER_CAP_DEPRECATED;
	}
#endif

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
	PLOG_DEBUG("Registered encoder #%llX with name '%s' and long name '%s' and caps %llX", avcodec_ptr,
	           avcodec_ptr->name, avcodec_ptr->long_name, avcodec_ptr->capabilities);
}

const char* encoder::generic_factory::get_name()
{
	return this->info.readable_name.c_str();
}

void encoder::generic_factory::get_defaults(obs_data_t* settings)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(this->avcodec_ptr->name);
		if (ptr) {
			ptr->get_defaults(settings, this->avcodec_ptr, nullptr);
		}
	}

	{ // Integrated Options
		// FFmpeg
		obs_data_set_default_string(settings, P_FFMPEG_CUSTOMSETTINGS, "");
		obs_data_set_default_int(settings, P_FFMPEG_COLORFORMAT, static_cast<int64_t>(AV_PIX_FMT_NONE));
		obs_data_set_default_int(settings, P_FFMPEG_THREADS, 0);
		obs_data_set_default_int(settings, P_FFMPEG_STANDARDCOMPLIANCE, FF_COMPLIANCE_STRICT);
	}
}

void encoder::generic_factory::get_properties(obs_properties_t* props)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(this->avcodec_ptr->name);
		if (ptr) {
			ptr->get_properties(props, this->avcodec_ptr, nullptr);
		}
	}

	{
		auto prs = obs_properties_create();
		{
			auto p =
			    obs_properties_add_text(prs, P_FFMPEG_CUSTOMSETTINGS, TRANSLATE(P_FFMPEG_CUSTOMSETTINGS),
			                            obs_text_type::OBS_TEXT_DEFAULT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_FFMPEG_CUSTOMSETTINGS)));
		}
		if (this->avcodec_ptr->pix_fmts) {
			auto p = obs_properties_add_list(prs, P_FFMPEG_COLORFORMAT, TRANSLATE(P_FFMPEG_COLORFORMAT),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_FFMPEG_COLORFORMAT)));
			obs_property_list_add_int(p, TRANSLATE(P_AUTOMATIC), static_cast<int64_t>(AV_PIX_FMT_NONE));
			for (auto ptr = this->avcodec_ptr->pix_fmts; *ptr != AV_PIX_FMT_NONE; ptr++) {
				obs_property_list_add_int(p, ffmpeg::tools::get_pixel_format_name(*ptr),
				                          static_cast<int64_t>(*ptr));
			}
		}
		{
			auto p = obs_properties_add_int_slider(prs, P_FFMPEG_THREADS, TRANSLATE(P_FFMPEG_THREADS), 0,
			                                       std::thread::hardware_concurrency() * 2, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_FFMPEG_THREADS)));
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

encoder::generic::generic(obs_data_t* settings, obs_encoder_t* encoder)
    : self(encoder), lag_in_frames(0), frame_count(0)
{
	this->factory = reinterpret_cast<generic_factory*>(obs_encoder_get_type_data(self));

	// Verify that the codec actually still exists.
	this->codec = avcodec_find_encoder_by_name(this->factory->get_avcodec()->name);
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
	int64_t threads = obs_data_get_int(settings, P_FFMPEG_THREADS);
	if (threads > 0) {
		this->context->thread_count = static_cast<int>(threads);
		this->lag_in_frames         = this->context->thread_count;
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

			PLOG_INFO("Automatically detected target format '%s' for source format '%s'.",
			          ffmpeg::tools::get_pixel_format_name(target),
			          ffmpeg::tools::get_pixel_format_name(source));
		}
		AVPixelFormat color_format_override =
		    static_cast<AVPixelFormat>(obs_data_get_int(settings, P_FFMPEG_COLORFORMAT));
		if (color_format_override != AV_PIX_FMT_NONE) {
			// User specified override for color format.
			this->context->pix_fmt = color_format_override;
			this->swscale.set_target_format(this->context->pix_fmt);
			PLOG_INFO("User specified target format override '%s'.",
			          ffmpeg::tools::get_pixel_format_name(this->context->pix_fmt));
		}

		// Framerate
		this->context->time_base.num   = voi->fps_den;
		this->context->time_base.den   = voi->fps_num;
		this->context->ticks_per_frame = 1;
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
		this->frame_queue.precache(std::thread::hardware_concurrency() / 4);
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

void encoder::generic::get_properties(obs_properties_t* props)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(this->codec->name);
		if (ptr) {
			ptr->get_properties(props, this->codec, this->context);
		}
	}

	obs_property_set_enabled(obs_properties_get(props, P_FFMPEG_COLORFORMAT), false);
	obs_property_set_enabled(obs_properties_get(props, P_FFMPEG_THREADS), false);
	obs_property_set_enabled(obs_properties_get(props, P_FFMPEG_STANDARDCOMPLIANCE), false);
}

bool encoder::generic::update(obs_data_t* settings)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(this->codec->name);
		if (ptr) {
			ptr->update(settings, this->codec, this->context);
		}
	}

	{ // FFmpeg
		// Apply custom options.
		av_opt_set_from_string(this->context->priv_data, obs_data_get_string(settings, P_FFMPEG_CUSTOMSETTINGS),
		                       nullptr, "=", ";");
	}
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

	// Send and receive frames.
	{
		ScopeProfiler profile("loop");
		bool          sent_frame  = false;
		bool          recv_packet = false;
		bool          should_lag  = (lag_in_frames - frame_count) <= 0;

		auto loop_begin = std::chrono::high_resolution_clock::now();
		auto loop_end   = loop_begin + std::chrono::milliseconds(50);

		while ((!sent_frame || (should_lag && !recv_packet))
		       && !(std::chrono::high_resolution_clock::now() > loop_end)) {
			bool eagain_is_stupid = false;

			if (!sent_frame) {
				ScopeProfiler profile_inner("send");

				vframe->pts = frame->pts;

				int res = send_frame(vframe);
				switch (res) {
				case 0:
					sent_frame = true;
					frame_count++;
					break;
				case AVERROR(EAGAIN):
					// This means we should call receive_packet again, but what do we do with that data?
					// Why can't we queue on both? Do I really have to implement threading for this stuff?
					if (*received_packet == true) {
						PLOG_WARNING(
						    "Skipped frame due to EAGAIN when a packet was already returned.");
						sent_frame = true;
						frame_count++;
					}
					eagain_is_stupid = true;
					break;
				case AVERROR(EOF):
					PLOG_ERROR("Skipped frame due to end of stream.");
					sent_frame = true;
					break;
				default:
					PLOG_ERROR("Failed to encode frame: %s (%ld).",
					           ffmpeg::tools::get_error_description(res), res);
					return false;
				}
			}

			if (!recv_packet) {
				ScopeProfiler profile_inner("recieve");

				int res = receive_packet(received_packet, packet);
				switch (res) {
				case 0:
					recv_packet = true;
					break;
				case AVERROR(EOF):
					PLOG_ERROR("Received end of file.");
					recv_packet = true;
					break;
				case AVERROR(EAGAIN):
					if (sent_frame) {
						recv_packet = true;
					}
					if (eagain_is_stupid) {
						PLOG_ERROR("Both send and recieve returned EAGAIN, encoder is broken.");
						return false;
					}
					break;
				default:
					PLOG_ERROR("Failed to receive packet: %s (%ld).",
					           ffmpeg::tools::get_error_description(res), res);
					return false;
				}
			}

			if (!sent_frame || !recv_packet) {
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
			frame_queue.push(uframe);
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
