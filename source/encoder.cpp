// FFMPEG Video Encoder Integration for OBS Studio
// Copyright (c) 2019 Michael Fabian Dirks <info@xaymar.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "encoder.hpp"
#include <iomanip>
#include <set>
#include <sstream>
#include <thread>
#include <util/profiler.hpp>
#include "codecs/hevc.hpp"
#include "ffmpeg/tools.hpp"
#include "plugin.hpp"
#include "strings.hpp"
#include "utility.hpp"

extern "C" {
#include <obs-avc.h>
#include <obs-module.h>
#pragma warning(push)
#pragma warning(disable : 4244)
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#pragma warning(pop)
}

// FFmpeg
#define ST_FFMPEG "FFmpeg"
#define ST_FFMPEG_CUSTOMSETTINGS "FFmpeg.CustomSettings"
#define ST_FFMPEG_THREADS "FFmpeg.Threads"
#define ST_FFMPEG_COLORFORMAT "FFmpeg.ColorFormat"
#define ST_FFMPEG_STANDARDCOMPLIANCE "FFmpeg.StandardCompliance"

enum class keyframe_type { SECONDS, FRAMES };

obsffmpeg::encoder_factory::encoder_factory(const AVCodec* codec) : avcodec_ptr(codec), info()
{
	// Unique Id is FFmpeg name.
	info.uid = avcodec_ptr->name;

	// Also generate a human readable name while we're at it.
	{
		std::stringstream sstr;
		if (!obsffmpeg::has_codec_handler(avcodec_ptr->name)) {
			sstr << "[UNSUPPORTED] ";
		}
		sstr << (avcodec_ptr->long_name ? avcodec_ptr->long_name : avcodec_ptr->name);
		if (avcodec_ptr->long_name) {
			sstr << " (" << avcodec_ptr->name << ")";
		}
		info.readable_name = sstr.str();

		// Allow UI Handler to replace visible name.
		obsffmpeg::find_codec_handler(avcodec_ptr->name)
		    ->override_visible_name(avcodec_ptr, info.readable_name);
	}

	// Assign Ids.
	{
		const AVCodecDescriptor* desc = avcodec_descriptor_get(avcodec_ptr->id);
		if (desc) {
			info.codec = desc->name;
		} else {
			// Fall back to encoder name in the case that FFmpeg itself doesn't know
			// what codec this actually is.
			info.codec = avcodec_ptr->name;
		}
	}

	info.oei.id    = info.uid.c_str();
	info.oei.codec = info.codec.c_str();

#ifndef _DEBUG
	// Is this a deprecated encoder?
	if (!obsffmpeg::has_codec_handler(avcodec_ptr->name)) {
		info.oei.caps |= OBS_ENCODER_CAP_DEPRECATED;
	}
#endif
}

obsffmpeg::encoder_factory::~encoder_factory() {}

void obsffmpeg::encoder_factory::register_encoder()
{
	// Detect encoder type (only Video and Audio supported)
	if (avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
		info.oei.type = obs_encoder_type::OBS_ENCODER_VIDEO;
	} else if (avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
		info.oei.type = obs_encoder_type::OBS_ENCODER_AUDIO;
	} else {
		throw std::invalid_argument("unsupported codec type");
	}

	// Register functions.
	info.oei.create = [](obs_data_t* settings, obs_encoder_t* encoder) {
		try {
			return reinterpret_cast<void*>(new obsffmpeg::encoder(settings, encoder));
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			return reinterpret_cast<void*>(0);
		} catch (...) {
			PLOG_ERROR("unknown exception");
			return reinterpret_cast<void*>(0);
		}
	};
	info.oei.destroy = [](void* ptr) {
		try {
			delete reinterpret_cast<encoder*>(ptr);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	info.oei.get_name = [](void* type_data) {
		try {
			return reinterpret_cast<encoder_factory*>(type_data)->get_name();
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	info.oei.get_defaults2 = [](obs_data_t* settings, void* type_data) {
		try {
			reinterpret_cast<encoder_factory*>(type_data)->get_defaults(settings);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	info.oei.get_properties2 = [](void* ptr, void* type_data) {
		try {
			obs_properties_t* props = obs_properties_create();
			if (type_data != nullptr) {
				reinterpret_cast<encoder_factory*>(type_data)->get_properties(props);
			}
			if (ptr != nullptr) {
				reinterpret_cast<encoder*>(ptr)->get_properties(props);
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
	info.oei.update = [](void* ptr, obs_data_t* settings) {
		try {
			return reinterpret_cast<encoder*>(ptr)->update(settings);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	info.oei.get_sei_data = [](void* ptr, uint8_t** sei_data, size_t* size) {
		try {
			return reinterpret_cast<encoder*>(ptr)->get_sei_data(sei_data, size);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};
	info.oei.get_extra_data = [](void* ptr, uint8_t** extra_data, size_t* size) {
		try {
			return reinterpret_cast<encoder*>(ptr)->get_extra_data(extra_data, size);
		} catch (std::exception const& e) {
			PLOG_ERROR("exception: %s", e.what());
			throw e;
		} catch (...) {
			PLOG_ERROR("unknown exception");
			throw;
		}
	};

	if (avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
		info.oei.get_video_info = [](void* ptr, struct video_scale_info* info) {
			try {
				reinterpret_cast<encoder*>(ptr)->get_video_info(info);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		info.oei.encode = [](void* ptr, struct encoder_frame* frame, struct encoder_packet* packet,
		                     bool* received_packet) {
			try {
				return reinterpret_cast<encoder*>(ptr)->video_encode(frame, packet, received_packet);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		info.oei.encode_texture = [](void* ptr, uint32_t handle, int64_t pts, uint64_t lock_key,
		                             uint64_t* next_key, struct encoder_packet* packet, bool* received_packet) {
			try {
				return reinterpret_cast<encoder*>(ptr)->video_encode_texture(
				    handle, pts, lock_key, next_key, packet, received_packet);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};

	} else if (avcodec_ptr->type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
		info.oei.get_audio_info = [](void* ptr, struct audio_convert_info* info) {
			try {
				reinterpret_cast<encoder*>(ptr)->get_audio_info(info);
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		info.oei.get_frame_size = [](void* ptr) {
			try {
				return reinterpret_cast<encoder*>(ptr)->get_frame_size();
			} catch (std::exception const& e) {
				PLOG_ERROR("exception: %s", e.what());
				throw e;
			} catch (...) {
				PLOG_ERROR("unknown exception");
				throw;
			}
		};
		info.oei.encode = [](void* ptr, struct encoder_frame* frame, struct encoder_packet* packet,
		                     bool* received_packet) {
			try {
				return reinterpret_cast<encoder*>(ptr)->audio_encode(frame, packet, received_packet);
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
	info.oei.type_data = this;

	obs_register_encoder(&info.oei);
	PLOG_DEBUG("Registered encoder #%llX with name '%s' and long name '%s' and caps %llX", avcodec_ptr,
	           avcodec_ptr->name, avcodec_ptr->long_name, avcodec_ptr->capabilities);
}

const char* obsffmpeg::encoder_factory::get_name()
{
	return info.readable_name.c_str();
}

void obsffmpeg::encoder_factory::get_defaults(obs_data_t* settings)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(avcodec_ptr->name);
		if (ptr) {
			ptr->get_defaults(settings, avcodec_ptr, nullptr);
		}
	}

	if ((avcodec_ptr->capabilities & AV_CODEC_CAP_INTRA_ONLY) == 0) {
		obs_data_set_default_int(settings, S_KEYFRAMES_INTERVALTYPE, 0);
		obs_data_set_default_double(settings, S_KEYFRAMES_INTERVAL_SECONDS, 2.0);
		obs_data_set_default_int(settings, S_KEYFRAMES_INTERVAL_FRAMES, 300);
	}

	{ // Integrated Options
		// FFmpeg
		obs_data_set_default_string(settings, ST_FFMPEG_CUSTOMSETTINGS, "");
		obs_data_set_default_int(settings, ST_FFMPEG_COLORFORMAT, static_cast<int64_t>(AV_PIX_FMT_NONE));
		obs_data_set_default_int(settings, ST_FFMPEG_THREADS, 0);
		obs_data_set_default_int(settings, ST_FFMPEG_STANDARDCOMPLIANCE, FF_COMPLIANCE_STRICT);
	}
}

static bool modified_keyframes(obs_properties_t* props, obs_property_t*, obs_data_t* settings)
{
	bool is_seconds = obs_data_get_int(settings, S_KEYFRAMES_INTERVALTYPE) == 0;
	obs_property_set_visible(obs_properties_get(props, S_KEYFRAMES_INTERVAL_FRAMES), !is_seconds);
	obs_property_set_visible(obs_properties_get(props, S_KEYFRAMES_INTERVAL_SECONDS), is_seconds);
	return true;
}

void obsffmpeg::encoder_factory::get_properties(obs_properties_t* props)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(avcodec_ptr->name);
		if (ptr) {
			ptr->get_properties(props, avcodec_ptr, nullptr);
		}
	}

	if ((avcodec_ptr->capabilities & AV_CODEC_CAP_INTRA_ONLY) == 0) {
		// Key-Frame Options
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, S_KEYFRAMES, TRANSLATE(S_KEYFRAMES), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p =
			    obs_properties_add_list(grp, S_KEYFRAMES_INTERVALTYPE, TRANSLATE(S_KEYFRAMES_INTERVALTYPE),
			                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(S_KEYFRAMES_INTERVALTYPE)));
			obs_property_set_modified_callback(p, modified_keyframes);
			obs_property_list_add_int(p, TRANSLATE(S_KEYFRAMES_INTERVALTYPE_(Seconds)), 0);
			obs_property_list_add_int(p, TRANSLATE(S_KEYFRAMES_INTERVALTYPE_(Frames)), 1);
		}
		{
			auto p =
			    obs_properties_add_float(grp, S_KEYFRAMES_INTERVAL_SECONDS, TRANSLATE(S_KEYFRAMES_INTERVAL),
			                             0.00, std::numeric_limits<int16_t>::max(), 0.01);
			obs_property_set_long_description(p, TRANSLATE(DESC(S_KEYFRAMES_INTERVAL)));
			obs_property_float_set_suffix(p, " seconds");
		}
		{
			auto p =
			    obs_properties_add_int(grp, S_KEYFRAMES_INTERVAL_FRAMES, TRANSLATE(S_KEYFRAMES_INTERVAL), 0,
			                           std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(S_KEYFRAMES_INTERVAL)));
			obs_property_int_set_suffix(p, " frames");
		}
	}

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			auto prs = obs_properties_create();
			obs_properties_add_group(props, ST_FFMPEG, TRANSLATE(ST_FFMPEG), OBS_GROUP_NORMAL, prs);
			grp = prs;
		}

		{
			auto p =
			    obs_properties_add_text(grp, ST_FFMPEG_CUSTOMSETTINGS, TRANSLATE(ST_FFMPEG_CUSTOMSETTINGS),
			                            obs_text_type::OBS_TEXT_DEFAULT);
			obs_property_set_long_description(p, TRANSLATE(DESC(ST_FFMPEG_CUSTOMSETTINGS)));
		}
		if (avcodec_ptr->pix_fmts) {
			auto p = obs_properties_add_list(grp, ST_FFMPEG_COLORFORMAT, TRANSLATE(ST_FFMPEG_COLORFORMAT),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(ST_FFMPEG_COLORFORMAT)));
			obs_property_list_add_int(p, TRANSLATE(S_STATE_AUTOMATIC),
			                          static_cast<int64_t>(AV_PIX_FMT_NONE));
			for (auto ptr = avcodec_ptr->pix_fmts; *ptr != AV_PIX_FMT_NONE; ptr++) {
				obs_property_list_add_int(p, ffmpeg::tools::get_pixel_format_name(*ptr),
				                          static_cast<int64_t>(*ptr));
			}
		}
		if (avcodec_ptr->capabilities & (AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS)) {
			auto p = obs_properties_add_int_slider(grp, ST_FFMPEG_THREADS, TRANSLATE(ST_FFMPEG_THREADS), 0,
			                                       std::thread::hardware_concurrency() * 2, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(ST_FFMPEG_THREADS)));
		}
		{
			auto p = obs_properties_add_list(grp, ST_FFMPEG_STANDARDCOMPLIANCE,
			                                 TRANSLATE(ST_FFMPEG_STANDARDCOMPLIANCE), OBS_COMBO_TYPE_LIST,
			                                 OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(ST_FFMPEG_STANDARDCOMPLIANCE)));
			obs_property_list_add_int(p, TRANSLATE(ST_FFMPEG_STANDARDCOMPLIANCE ".VeryStrict"),
			                          FF_COMPLIANCE_VERY_STRICT);
			obs_property_list_add_int(p, TRANSLATE(ST_FFMPEG_STANDARDCOMPLIANCE ".Strict"),
			                          FF_COMPLIANCE_STRICT);
			obs_property_list_add_int(p, TRANSLATE(ST_FFMPEG_STANDARDCOMPLIANCE ".Normal"),
			                          FF_COMPLIANCE_NORMAL);
			obs_property_list_add_int(p, TRANSLATE(ST_FFMPEG_STANDARDCOMPLIANCE ".Unofficial"),
			                          FF_COMPLIANCE_UNOFFICIAL);
			obs_property_list_add_int(p, TRANSLATE(ST_FFMPEG_STANDARDCOMPLIANCE ".Experimental"),
			                          FF_COMPLIANCE_EXPERIMENTAL);
		}
	};
}

const AVCodec* obsffmpeg::encoder_factory::get_avcodec()
{
	return avcodec_ptr;
}

obsffmpeg::encoder::encoder(obs_data_t* settings, obs_encoder_t* encoder)
    : _self(encoder), _lag_in_frames(0), _count_send_frames(0), _have_first_frame(false), _initialized(false)
{
	_factory = reinterpret_cast<encoder_factory*>(obs_encoder_get_type_data(_self));

	// Verify that the codec actually still exists.
	_codec = avcodec_find_encoder_by_name(_factory->get_avcodec()->name);
	if (!_codec) {
		PLOG_ERROR("Failed to find encoder for codec '%s'.", _factory->get_avcodec()->name);
		throw std::runtime_error("failed to find codec");
	}

	// Initialize context.
	_context = avcodec_alloc_context3(_codec);
	if (!_context) {
		PLOG_ERROR("Failed to create context for encoder '%s'.", _codec->name);
		throw std::runtime_error("failed to create context");
	}

	// Settings
	/// Rate Control
	_context->strict_std_compliance = static_cast<int>(obs_data_get_int(settings, ST_FFMPEG_STANDARDCOMPLIANCE));
	_context->debug                 = 0;
	/// Threading
	if (_codec->capabilities
	    & (AV_CODEC_CAP_AUTO_THREADS | AV_CODEC_CAP_FRAME_THREADS | AV_CODEC_CAP_SLICE_THREADS)) {
		if (_codec->capabilities & AV_CODEC_CAP_FRAME_THREADS) {
			_context->thread_type |= FF_THREAD_FRAME;
		}
		if (_codec->capabilities & AV_CODEC_CAP_SLICE_THREADS) {
			_context->thread_type |= FF_THREAD_SLICE;
		}
		int64_t threads = obs_data_get_int(settings, ST_FFMPEG_THREADS);
		if (threads > 0) {
			_context->thread_count = static_cast<int>(threads);
			_lag_in_frames         = _context->thread_count;
		} else {
			_context->thread_count = std::thread::hardware_concurrency();
			_lag_in_frames         = _context->thread_count;
		}
	}

	// Update settings
	update(settings);

	av_init_packet(&_current_packet);
	av_new_packet(&_current_packet, 8 * 1024 * 1024); // 8 MB precached Packet size.
}

obsffmpeg::encoder::~encoder()
{
	if (_context) {
		// Flush encoders that require it.
		if ((_codec->capabilities & AV_CODEC_CAP_DELAY) != 0) {
			avcodec_send_frame(_context, nullptr);
			while (avcodec_receive_packet(_context, &_current_packet) >= 0) {
				avcodec_send_frame(_context, nullptr);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}

		// Close and free context.
		avcodec_close(_context);
		avcodec_free_context(&_context);
	}

	av_packet_unref(&_current_packet);

	_frame_queue.clear();
	_frame_queue_used.clear();
	_swscale.finalize();
}

bool obsffmpeg::encoder::initialize()
{
	if (_initialized)
		return true;

	// Detect supported
	bool is_format_supported = false;
	for (auto ptr = _codec->pix_fmts; *ptr != AV_PIX_FMT_NONE; ptr++) {
		if (*ptr == _format) {
			is_format_supported = true;
		}
	}
	if (!is_format_supported) {
		std::stringstream sstr;
		sstr << "The color format " << ffmpeg::tools::get_pixel_format_name(_format)
		     << " is not supported by the encoder.";
		throw std::exception(sstr.str().c_str());
	}

	if (_codec->type == AVMEDIA_TYPE_VIDEO) {
		auto encvideo = obs_encoder_video(_self);
		auto voi      = video_output_get_info(encvideo);

		// Set Resolution
		_context->width  = static_cast<int>(_resolution.first);
		_context->height = static_cast<int>(_resolution.second);

		// Set Color Space, Format and Range
		_context->colorspace  = ffmpeg::tools::obs_videocolorspace_to_avcolorspace(voi->colorspace);
		_context->color_range = ffmpeg::tools::obs_videorangetype_to_avcolorrange(voi->range);
		_context->pix_fmt     = _format;

		// Set Framerate and Field Order
		_context->field_order     = AV_FIELD_PROGRESSIVE;
		_context->time_base.num   = voi->fps_den;
		_context->time_base.den   = voi->fps_num;
		_context->ticks_per_frame = 1;
	}

	// Initialize
	int res = avcodec_open2(_context, _codec, NULL);
	if (res < 0) {
		std::stringstream sstr;
		sstr << "Failed to initalized encoder '" << _codec->name
		     << "' due to error: " << ffmpeg::tools::get_error_description(res) << "(" << res
		     << ")";
		    throw std::runtime_error(sstr.str().c_str());
	}

	// Initialize Scaler and Frame Queue
	if (_codec->type == AVMEDIA_TYPE_VIDEO) {
		_swscale.set_source_format(_format);
		_swscale.set_target_format(_context->pix_fmt);
		_swscale.set_target_size(_context->width, _context->height);
		_swscale.set_source_size(_context->width, _context->height);
		_swscale.set_target_size(_context->width, _context->height);
		_swscale.set_source_color(_context->color_range, _context->colorspace);
		_swscale.set_target_color(_context->color_range, _context->colorspace);

		// Create Scaler
		if (!_swscale.initialize(SWS_FAST_BILINEAR)) {
			std::stringstream sstr;
			sstr << "Failed to initialize frame scaler for pixel format '"
			     << ffmpeg::tools::get_pixel_format_name(_context->pix_fmt) << "' with color space '"
			     << ffmpeg::tools::get_color_space_name(_context->colorspace) << "' and "
			     << (_swscale.is_source_full_range() ? "full" : "partial") << " color range.";
			throw std::runtime_error(sstr.str().c_str());
		}

		// Create Frame queue
		_frame_queue.set_pixel_format(_context->pix_fmt);
		_frame_queue.set_resolution(_context->width, _context->height);
		_frame_queue.precache(2);
	}

	_initialized = true;
	return true;
}

void obsffmpeg::encoder::get_properties(obs_properties_t* props)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(_codec->name);
		if (ptr) {
			ptr->get_properties(props, _codec, _context);
		}
	}

	obs_property_set_enabled(obs_properties_get(props, S_KEYFRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, S_KEYFRAMES_INTERVALTYPE), false);
	obs_property_set_enabled(obs_properties_get(props, S_KEYFRAMES_INTERVAL_SECONDS), false);
	obs_property_set_enabled(obs_properties_get(props, S_KEYFRAMES_INTERVAL_FRAMES), false);

	obs_property_set_enabled(obs_properties_get(props, ST_FFMPEG_COLORFORMAT), false);
	obs_property_set_enabled(obs_properties_get(props, ST_FFMPEG_THREADS), false);
	obs_property_set_enabled(obs_properties_get(props, ST_FFMPEG_STANDARDCOMPLIANCE), false);
}

bool obsffmpeg::encoder::update(obs_data_t* settings)
{
	{ // Handler
		auto ptr = obsffmpeg::find_codec_handler(_codec->name);
		if (ptr) {
			ptr->update(settings, _codec, _context);
		}
	}

	if ((_codec->capabilities & AV_CODEC_CAP_INTRA_ONLY) == 0) {
		// Key-Frame Options
		obs_video_info ovi;
		if (!obs_get_video_info(&ovi)) {
			throw std::runtime_error("no video info");
		}

		int64_t kf_type    = obs_data_get_int(settings, S_KEYFRAMES_INTERVALTYPE);
		bool    is_seconds = (kf_type == 0);

		if (is_seconds) {
			_context->gop_size = static_cast<int>(
			    obs_data_get_double(settings, S_KEYFRAMES_INTERVAL_SECONDS) * (ovi.fps_num / ovi.fps_den));
		} else {
			_context->gop_size = static_cast<int>(obs_data_get_int(settings, S_KEYFRAMES_INTERVAL_FRAMES));
		}
		_context->keyint_min = _context->gop_size;
	}

	{ // FFmpeg
		// Apply custom options.
		av_opt_set_from_string(_context->priv_data, obs_data_get_string(settings, ST_FFMPEG_CUSTOMSETTINGS),
		                       nullptr, "=", ";");
	}
	return false;
}

void obsffmpeg::encoder::get_audio_info(audio_convert_info*) {}

size_t obsffmpeg::encoder::get_frame_size()
{
	return size_t();
}

bool obsffmpeg::encoder::audio_encode(encoder_frame*, encoder_packet*, bool*)
{
	return false;
}

void obsffmpeg::encoder::get_video_info(video_scale_info* vsi)
{
	obs_data_t* settings = obs_encoder_get_settings(_self);

	AVPixelFormat avformat  = static_cast<AVPixelFormat>(obs_data_get_int(settings, ST_FFMPEG_COLORFORMAT));
	video_format  obsformat = ffmpeg::tools::avpixelformat_to_obs_videoformat(avformat);

	if (obsformat != VIDEO_FORMAT_NONE) {
		vsi->format = obsformat;
	}

	_resolution.first  = vsi->width;
	_resolution.second = vsi->height;
	_format            = ffmpeg::tools::obs_videoformat_to_avpixelformat(vsi->format);
}

bool obsffmpeg::encoder::get_sei_data(uint8_t** data, size_t* size)
{
	if (_sei_data.size() == 0)
		return false;

	*data = _sei_data.data();
	*size = _sei_data.size();
	return true;
}

bool obsffmpeg::encoder::get_extra_data(uint8_t** data, size_t* size)
{
	if (_extra_data.size() == 0)
		return false;

	*data = _extra_data.data();
	*size = _extra_data.size();
	return true;
}

static inline void copy_data(encoder_frame* frame, AVFrame* vframe)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(static_cast<AVPixelFormat>(vframe->format), &h_chroma_shift, &v_chroma_shift);

	for (size_t idx = 0; idx < MAX_AV_PLANES; idx++) {
		if (!frame->data[idx] || !vframe->data[idx])
			continue;

		size_t plane_height = vframe->height >> (idx ? v_chroma_shift : 0);

		if (static_cast<uint32_t>(vframe->linesize[idx]) == frame->linesize[idx]) {
			std::memcpy(vframe->data[idx], frame->data[idx], frame->linesize[idx] * plane_height);
		} else {
			size_t ls_in  = frame->linesize[idx];
			size_t ls_out = vframe->linesize[idx];
			size_t bytes  = ls_in < ls_out ? ls_in : ls_out;

			uint8_t* to   = vframe->data[idx];
			uint8_t* from = frame->data[idx];

			for (size_t y = 0; y < plane_height; y++) {
				std::memcpy(to, from, bytes);
				to += ls_out;
				from += ls_in;
			}
		}
	}
}

bool obsffmpeg::encoder::video_encode(encoder_frame* frame, encoder_packet* packet, bool* received_packet)
{
	// Ensure that the encoder was initialized.
	try {
		if (!initialize())
			return false;
	} catch (std::exception& ex) {
		PLOG_ERROR("%s", ex.what());
		return false;
	}

	// Convert frame.
	std::shared_ptr<AVFrame> vframe = _frame_queue.pop(); // Retrieve an empty frame.
	{
#ifdef _DEBUG
		ScopeProfiler profile("convert");
#endif

		vframe->height      = _context->height;
		vframe->format      = _context->pix_fmt;
		vframe->color_range = _context->color_range;
		vframe->colorspace  = _context->colorspace;
		vframe->pts         = frame->pts;

		if ((_swscale.is_source_full_range() == _swscale.is_target_full_range())
		    && (_swscale.get_source_colorspace() == _swscale.get_target_colorspace())
		    && (_swscale.get_source_format() == _swscale.get_target_format())) {
			copy_data(frame, vframe.get());
		} else {
			int res = _swscale.convert(reinterpret_cast<uint8_t**>(frame->data),
			                           reinterpret_cast<int*>(frame->linesize), 0, _context->height,
			                           vframe->data, vframe->linesize);
			if (res <= 0) {
				PLOG_ERROR("Failed to convert frame: %s (%ld).",
				           ffmpeg::tools::get_error_description(res), res);
				return false;
			}
		}
	}

	// Send and receive frames.
	{
#ifdef _DEBUG
		ScopeProfiler profile("loop");
#endif

		bool sent_frame  = false;
		bool recv_packet = false;
		bool should_lag  = (_lag_in_frames - _count_send_frames) <= 0;

		auto loop_begin = std::chrono::high_resolution_clock::now();
		auto loop_end   = loop_begin + std::chrono::milliseconds(50);

		while ((!sent_frame || (should_lag && !recv_packet))
		       && !(std::chrono::high_resolution_clock::now() > loop_end)) {
			bool eagain_is_stupid = false;

			if (!sent_frame) {
#ifdef _DEBUG
				ScopeProfiler profile_inner("send");
#endif
				int res = send_frame(vframe);
				switch (res) {
				case 0:
					sent_frame = true;
					vframe     = nullptr;
					break;
				case AVERROR(EAGAIN):
					// This means we should call receive_packet again, but what do we do with that data?
					// Why can't we queue on both? Do I really have to implement threading for this stuff?
					if (*received_packet == true) {
						PLOG_WARNING(
						    "Skipped frame due to EAGAIN when a packet was already returned.");
						sent_frame = true;
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
#ifdef _DEBUG
				ScopeProfiler profile_inner("recieve");
#endif
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

	if (vframe != nullptr) {
		_frame_queue.push(vframe);
	}

	return true;
}

bool obsffmpeg::encoder::video_encode_texture(uint32_t, int64_t, uint64_t, uint64_t*, encoder_packet*, bool*)
{
	return false;
}

int obsffmpeg::encoder::receive_packet(bool* received_packet, struct encoder_packet* packet)
{
	int res = avcodec_receive_packet(_context, &_current_packet);
	if (res == 0) {
		if (!_have_first_frame) {
			if (_codec->id == AV_CODEC_ID_H264) {
				uint8_t* tmp_packet;
				uint8_t* tmp_header;
				uint8_t* tmp_sei;
				size_t   sz_packet, sz_header, sz_sei;

				obs_extract_avc_headers(_current_packet.data, _current_packet.size, &tmp_packet,
				                        &sz_packet, &tmp_header, &sz_header, &tmp_sei, &sz_sei);

				if (sz_header) {
					_extra_data.resize(sz_header);
					std::memcpy(_extra_data.data(), tmp_header, sz_header);
				}

				if (sz_sei) {
					_sei_data.resize(sz_sei);
					std::memcpy(_sei_data.data(), tmp_sei, sz_sei);
				}

				std::memcpy(_current_packet.data, tmp_packet, sz_packet);
				_current_packet.size = static_cast<int>(sz_packet);

				bfree(tmp_packet);
				bfree(tmp_header);
				bfree(tmp_sei);
			} else if (_codec->id == AV_CODEC_ID_HEVC) {
				obsffmpeg::codecs::hevc::extract_header_sei(_current_packet.data, _current_packet.size,
				                                            _extra_data, _sei_data);
			}
			_have_first_frame = true;
		}

		packet->type          = OBS_ENCODER_VIDEO;
		packet->pts           = _current_packet.pts;
		packet->dts           = _current_packet.dts;
		packet->data          = _current_packet.data;
		packet->size          = _current_packet.size;
		packet->keyframe      = !!(_current_packet.flags & AV_PKT_FLAG_KEY);
		packet->drop_priority = packet->keyframe ? 0 : 1;
		*received_packet      = true;

		{
			std::shared_ptr<AVFrame> uframe = _frame_queue_used.pop_only();
			_frame_queue.push(uframe);
		}
	}

	return res;
}

int obsffmpeg::encoder::send_frame(std::shared_ptr<AVFrame> const frame)
{
	int res = avcodec_send_frame(_context, frame.get());
	switch (res) {
	case 0:
		_frame_queue_used.push(frame);
		_count_send_frames++;
	case AVERROR(EAGAIN):
	case AVERROR(EOF):
		break;
	}
	return res;
}
