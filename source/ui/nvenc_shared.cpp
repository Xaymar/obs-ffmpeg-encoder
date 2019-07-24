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

#include "nvenc_shared.hpp"
#include "codecs/hevc.hpp"
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

#define P_PRESET "NVENC.Preset"
#define P_PRESET_(x) P_PRESET "." D_VSTR(x)

#define P_RATECONTROL "NVENC.RateControl"
#define P_RATECONTROL_MODE P_RATECONTROL ".Mode"
#define P_RATECONTROL_MODE_(x) P_RATECONTROL_MODE "." D_VSTR(x)
#define P_RATECONTROL_TWOPASS P_RATECONTROL ".TwoPass"
#define P_RATECONTROL_LOOKAHEAD P_RATECONTROL ".LookAhead"
#define P_RATECONTROL_ADAPTIVEI P_RATECONTROL ".AdaptiveI"
#define P_RATECONTROL_ADAPTIVEB P_RATECONTROL ".AdaptiveB"

#define P_RATECONTROL_BITRATE P_RATECONTROL ".Bitrate"
#define P_RATECONTROL_BITRATE_TARGET P_RATECONTROL_BITRATE ".Target"
#define P_RATECONTROL_BITRATE_MAXIMUM P_RATECONTROL_BITRATE ".Maximum"

#define P_RATECONTROL_QUALITY P_RATECONTROL ".Quality"
#define P_RATECONTROL_QUALITY_MINIMUM P_RATECONTROL_QUALITY ".Minimum"
#define P_RATECONTROL_QUALITY_MAXIMUM P_RATECONTROL_QUALITY ".Maximum"

#define P_RATECONTROL_QP P_RATECONTROL ".QP"
#define P_RATECONTROL_QP_I P_RATECONTROL_QP ".I"
#define P_RATECONTROL_QP_I_INITIAL P_RATECONTROL_QP_I ".Initial"
#define P_RATECONTROL_QP_P P_RATECONTROL_QP ".P"
#define P_RATECONTROL_QP_P_INITIAL P_RATECONTROL_QP_P ".Initial"
#define P_RATECONTROL_QP_B P_RATECONTROL_QP ".B"
#define P_RATECONTROL_QP_B_INITIAL P_RATECONTROL_QP_B ".Initial"

#define P_AQ "NVENC.AQ"
#define P_AQ_SPATIAL P_AQ ".Spatial"
#define P_AQ_TEMPORAL P_AQ ".Temporal"
#define P_AQ_STRENGTH P_AQ ".Strength"

#define P_OTHER "NVENC.Other"
#define P_OTHER_BFRAMES P_OTHER ".BFrames"
#define P_OTHER_BFRAME_REFERENCEMODE P_OTHER ".BFrameReferenceMode"
#define P_OTHER_ZEROLATENCY P_OTHER ".ZeroLatency"
#define P_OTHER_WEIGHTED_PREDICTION P_OTHER ".WeightedPrediction"
#define P_OTHER_NONREFERENCE_PFRAMES P_OTHER ".NonReferencePFrames"

using namespace obsffmpeg::nvenc;

std::map<preset, std::string> obsffmpeg::nvenc::presets{
    {preset::DEFAULT, P_PRESET_(Default)},
    {preset::SLOW, P_PRESET_(Slow)},
    {preset::MEDIUM, P_PRESET_(Medium)},
    {preset::FAST, P_PRESET_(Fast)},
    {preset::HIGH_PERFORMANCE, P_PRESET_(HighPerformance)},
    {preset::HIGH_QUALITY, P_PRESET_(HighQuality)},
    {preset::BLURAYDISC, P_PRESET_(BluRayDisc)},
    {preset::LOW_LATENCY, P_PRESET_(LowLatency)},
    {preset::LOW_LATENCY_HIGH_PERFORMANCE, P_PRESET_(LowLatencyHighPerformance)},
    {preset::LOW_LATENCY_HIGH_QUALITY, P_PRESET_(LowLatencyHighQuality)},
    {preset::LOSSLESS, P_PRESET_(Lossless)},
    {preset::LOSSLESS_HIGH_PERFORMANCE, P_PRESET_(LosslessHighPerformance)},
};

std::map<preset, std::string> obsffmpeg::nvenc::preset_to_opt{
    {preset::DEFAULT, "default"},
    {preset::SLOW, "slow"},
    {preset::MEDIUM, "medium"},
    {preset::FAST, "fast"},
    {preset::HIGH_PERFORMANCE, "hp"},
    {preset::HIGH_QUALITY, "hq"},
    {preset::BLURAYDISC, "bd"},
    {preset::LOW_LATENCY, "ll"},
    {preset::LOW_LATENCY_HIGH_PERFORMANCE, "llhp"},
    {preset::LOW_LATENCY_HIGH_QUALITY, "llhq"},
    {preset::LOSSLESS, "lossless"},
    {preset::LOSSLESS_HIGH_PERFORMANCE, "losslesshp"},
};

std::map<ratecontrolmode, std::string> obsffmpeg::nvenc::ratecontrolmodes{
    {ratecontrolmode::CQP, P_RATECONTROL_MODE_(CQP)},
    {ratecontrolmode::VBR, P_RATECONTROL_MODE_(VBR)},
    {ratecontrolmode::VBR_HQ, P_RATECONTROL_MODE_(VBR_HQ)},
    {ratecontrolmode::CBR, P_RATECONTROL_MODE_(CBR)},
    {ratecontrolmode::CBR_HQ, P_RATECONTROL_MODE_(CBR_HQ)},
    {ratecontrolmode::CBR_LD_HQ, P_RATECONTROL_MODE_(CBR_LD_HQ)},
};

std::map<ratecontrolmode, std::string> obsffmpeg::nvenc::ratecontrolmode_to_opt{
    {ratecontrolmode::CQP, "constqp"}, {ratecontrolmode::VBR, "vbr"},       {ratecontrolmode::VBR_HQ, "vbr_hq"},
    {ratecontrolmode::CBR, "cbr"},     {ratecontrolmode::CBR_HQ, "cbr_hq"}, {ratecontrolmode::CBR_LD_HQ, "cbr_ld_hq"},
};

std::map<b_ref_mode, std::string> obsffmpeg::nvenc::b_ref_modes{
    {b_ref_mode::DISABLED, G_STATE_DISABLED},
    {b_ref_mode::EACH, P_OTHER_BFRAME_REFERENCEMODE ".Each"},
    {b_ref_mode::MIDDLE, P_OTHER_BFRAME_REFERENCEMODE ".Middle"},
};

std::map<b_ref_mode, std::string> obsffmpeg::nvenc::b_ref_mode_to_opt{
    {b_ref_mode::DISABLED, "disabled"},
    {b_ref_mode::EACH, "each"},
    {b_ref_mode::MIDDLE, "middle"},
};

void obsffmpeg::nvenc::get_defaults(obs_data_t* settings, AVCodec*, AVCodecContext*)
{
	obs_data_set_default_int(settings, P_PRESET, static_cast<int64_t>(preset::DEFAULT));

	obs_data_set_default_int(settings, P_RATECONTROL_MODE, static_cast<int64_t>(ratecontrolmode::CBR_HQ));
	obs_data_set_default_int(settings, P_RATECONTROL_TWOPASS, -1);
	obs_data_set_default_int(settings, P_RATECONTROL_LOOKAHEAD, 0);
	obs_data_set_default_bool(settings, P_RATECONTROL_ADAPTIVEI, true);
	obs_data_set_default_bool(settings, P_RATECONTROL_ADAPTIVEB, true);

	obs_data_set_default_int(settings, P_RATECONTROL_BITRATE_TARGET, 6000);
	obs_data_set_default_int(settings, P_RATECONTROL_BITRATE_MAXIMUM, 6000);
	obs_data_set_default_int(settings, G_RATECONTROL_BUFFERSIZE, 12000);

	obs_data_set_default_int(settings, P_RATECONTROL_QUALITY_MINIMUM, 51);
	obs_data_set_default_int(settings, P_RATECONTROL_QUALITY_MAXIMUM, -1);

	obs_data_set_default_int(settings, P_RATECONTROL_QP_I, 21);
	obs_data_set_default_int(settings, P_RATECONTROL_QP_I_INITIAL, -1);
	obs_data_set_default_int(settings, P_RATECONTROL_QP_P, 21);
	obs_data_set_default_int(settings, P_RATECONTROL_QP_P_INITIAL, -1);
	obs_data_set_default_int(settings, P_RATECONTROL_QP_B, 21);
	obs_data_set_default_int(settings, P_RATECONTROL_QP_B_INITIAL, -1);

	obs_data_set_default_int(settings, G_KEYFRAMES_INTERVALTYPE, 0);
	obs_data_set_default_double(settings, G_KEYFRAMES_INTERVAL_SECONDS, 2.0);
	obs_data_set_default_int(settings, G_KEYFRAMES_INTERVAL_FRAMES, 300);

	obs_data_set_default_bool(settings, P_AQ_SPATIAL, true);
	obs_data_set_default_int(settings, P_AQ_STRENGTH, 8);
	obs_data_set_default_bool(settings, P_AQ_TEMPORAL, true);

	obs_data_set_default_int(settings, P_OTHER_BFRAMES, 2);
	obs_data_set_default_int(settings, P_OTHER_BFRAME_REFERENCEMODE, static_cast<int64_t>(b_ref_mode::DISABLED));
	obs_data_set_default_bool(settings, P_OTHER_ZEROLATENCY, false);
	obs_data_set_default_bool(settings, P_OTHER_WEIGHTED_PREDICTION, false);
	obs_data_set_default_bool(settings, P_OTHER_NONREFERENCE_PFRAMES, false);
}

static bool modified_ratecontrol(obs_properties_t* props, obs_property_t*, obs_data_t* settings)
{
	using namespace obsffmpeg::nvenc;

	bool have_bitrate     = false;
	bool have_bitrate_max = false;
	bool have_quality     = false;
	bool have_qp          = false;
	bool have_qp_init     = false;

	ratecontrolmode rc = static_cast<ratecontrolmode>(obs_data_get_int(settings, P_RATECONTROL_MODE));
	switch (rc) {
	case ratecontrolmode::CQP:
		have_qp = true;
		break;
	case ratecontrolmode::CBR:
	case ratecontrolmode::CBR_HQ:
	case ratecontrolmode::CBR_LD_HQ:
		have_bitrate = true;
		break;
	case ratecontrolmode::VBR:
	case ratecontrolmode::VBR_HQ:
		have_bitrate     = true;
		have_bitrate_max = true;
		have_quality     = true;
		have_qp_init     = true;
		break;
	}

	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BITRATE), have_bitrate || have_bitrate_max);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BITRATE_TARGET), have_bitrate);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_BITRATE_MAXIMUM), have_bitrate_max);
	obs_property_set_visible(obs_properties_get(props, G_RATECONTROL_BUFFERSIZE), have_bitrate || have_bitrate_max);

	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QUALITY), have_quality);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QUALITY_MINIMUM), have_quality);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QUALITY_MAXIMUM), have_quality);

	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QP_I), have_qp);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QP_P), have_qp);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QP_B), have_qp);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QP_I_INITIAL), have_qp_init);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QP_P_INITIAL), have_qp_init);
	obs_property_set_visible(obs_properties_get(props, P_RATECONTROL_QP_B_INITIAL), have_qp_init);

	return true;
}

static bool modified_quality(obs_properties_t* props, obs_property_t*, obs_data_t* settings)
{
	bool enabled = obs_data_get_bool(settings, P_RATECONTROL_QUALITY);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QUALITY_MINIMUM), enabled);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QUALITY_MAXIMUM), enabled);
	return true;
}

static bool modified_keyframes(obs_properties_t* props, obs_property_t*, obs_data_t* settings)
{
	bool is_seconds = obs_data_get_int(settings, G_KEYFRAMES_INTERVALTYPE) == 0;
	obs_property_set_visible(obs_properties_get(props, G_KEYFRAMES_INTERVAL_FRAMES), !is_seconds);
	obs_property_set_visible(obs_properties_get(props, G_KEYFRAMES_INTERVAL_SECONDS), is_seconds);
	return true;
}

static bool modified_aq(obs_properties_t* props, obs_property_t*, obs_data_t* settings)
{
	bool spatial_aq = obs_data_get_bool(settings, P_AQ_SPATIAL);
	obs_property_set_visible(obs_properties_get(props, P_AQ_STRENGTH), spatial_aq);
	return true;
}

void obsffmpeg::nvenc::get_properties_pre(obs_properties_t* props, AVCodec*)
{
	{
		auto p = obs_properties_add_list(props, P_PRESET, TRANSLATE(P_PRESET), OBS_COMBO_TYPE_LIST,
		                                 OBS_COMBO_FORMAT_INT);
		obs_property_set_long_description(p, TRANSLATE(DESC(P_PRESET)));
		for (auto kv : presets) {
			obs_property_list_add_int(p, TRANSLATE(kv.second.c_str()), static_cast<int64_t>(kv.first));
		}
	}
}

void obsffmpeg::nvenc::get_properties_post(obs_properties_t* props, AVCodec* codec)
{
	{ // Rate Control
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, P_RATECONTROL, TRANSLATE(P_RATECONTROL), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_list(grp, P_RATECONTROL_MODE, TRANSLATE(P_RATECONTROL_MODE),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_MODE)));
			obs_property_set_modified_callback(p, modified_ratecontrol);
			for (auto kv : ratecontrolmodes) {
				obs_property_list_add_int(p, TRANSLATE(kv.second.c_str()),
				                          static_cast<int64_t>(kv.first));
			}
		}

		{
			auto p = obs_properties_add_list(grp, P_RATECONTROL_TWOPASS, TRANSLATE(P_RATECONTROL_TWOPASS),
			                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_TWOPASS)));
			obs_property_list_add_int(p, TRANSLATE(G_STATE_DEFAULT), -1);
			obs_property_list_add_int(p, TRANSLATE(G_STATE_DISABLED), 0);
			obs_property_list_add_int(p, TRANSLATE(G_STATE_ENABLED), 1);
		}

		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_LOOKAHEAD,
			                                       TRANSLATE(P_RATECONTROL_LOOKAHEAD), 0, 60, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_LOOKAHEAD)));
			obs_property_int_set_suffix(p, " frames");
		}
		{
			auto p =
			    obs_properties_add_bool(grp, P_RATECONTROL_ADAPTIVEI, TRANSLATE(P_RATECONTROL_ADAPTIVEI));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_ADAPTIVEI)));
		}
		if (strcmp(codec->name, "h264_nvenc") == 0) {
			auto p =
			    obs_properties_add_bool(grp, P_RATECONTROL_ADAPTIVEB, TRANSLATE(P_RATECONTROL_ADAPTIVEB));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_ADAPTIVEB)));
		}
	}

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, P_RATECONTROL_BITRATE, TRANSLATE(P_RATECONTROL_BITRATE),
			                         OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_int(grp, P_RATECONTROL_BITRATE_TARGET,
			                                TRANSLATE(P_RATECONTROL_BITRATE_TARGET), 1,
			                                std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BITRATE_TARGET)));
			obs_property_int_set_suffix(p, " kbit/s");
		}
		{
			auto p = obs_properties_add_int(grp, P_RATECONTROL_BITRATE_MAXIMUM,
			                                TRANSLATE(P_RATECONTROL_BITRATE_MAXIMUM), 0,
			                                std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_BITRATE_MAXIMUM)));
			obs_property_int_set_suffix(p, " kbit/s");
		}
		{
			auto p =
			    obs_properties_add_int(grp, G_RATECONTROL_BUFFERSIZE, TRANSLATE(G_RATECONTROL_BUFFERSIZE),
			                           0, std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(G_RATECONTROL_BUFFERSIZE)));
			obs_property_int_set_suffix(p, " kbit");
		}
	}

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp    = obs_properties_create();
			auto p = obs_properties_add_group(props, P_RATECONTROL_QUALITY,
			                                  TRANSLATE(P_RATECONTROL_QUALITY), OBS_GROUP_CHECKABLE, grp);
			obs_property_set_modified_callback(p, modified_quality);
		} else {
			auto p =
			    obs_properties_add_bool(props, P_RATECONTROL_QUALITY, TRANSLATE(P_RATECONTROL_QUALITY));
			obs_property_set_modified_callback(p, modified_quality);
		}

		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QUALITY_MINIMUM,
			                                       TRANSLATE(P_RATECONTROL_QUALITY_MINIMUM), 0, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QUALITY_MINIMUM)));
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QUALITY_MAXIMUM,
			                                       TRANSLATE(P_RATECONTROL_QUALITY_MAXIMUM), -1, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QUALITY_MAXIMUM)));
		}
	}
	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp    = obs_properties_create();
			auto p = obs_properties_add_group(props, P_RATECONTROL_QP, TRANSLATE(P_RATECONTROL_QP),
			                                  OBS_GROUP_CHECKABLE, grp);
			obs_property_set_modified_callback(p, modified_quality);
		}

		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QP_I, TRANSLATE(P_RATECONTROL_QP_I),
			                                       0, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QP_I)));
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QP_I_INITIAL,
			                                       TRANSLATE(P_RATECONTROL_QP_I_INITIAL), -1, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QP_I_INITIAL)));
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QP_P, TRANSLATE(P_RATECONTROL_QP_P),
			                                       0, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QP_P)));
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QP_P_INITIAL,
			                                       TRANSLATE(P_RATECONTROL_QP_P_INITIAL), -1, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QP_P_INITIAL)));
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QP_B, TRANSLATE(P_RATECONTROL_QP_B),
			                                       0, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QP_B)));
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_RATECONTROL_QP_B_INITIAL,
			                                       TRANSLATE(P_RATECONTROL_QP_B_INITIAL), -1, 51, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_RATECONTROL_QP_B_INITIAL)));
		}
	}

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, G_KEYFRAMES, TRANSLATE(G_KEYFRAMES), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p =
			    obs_properties_add_list(grp, G_KEYFRAMES_INTERVALTYPE, TRANSLATE(G_KEYFRAMES_INTERVALTYPE),
			                            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(G_KEYFRAMES_INTERVALTYPE)));
			obs_property_set_modified_callback(p, modified_keyframes);
			obs_property_list_add_int(p, TRANSLATE(G_KEYFRAMES_INTERVALTYPE_(Seconds)), 0);
			obs_property_list_add_int(p, TRANSLATE(G_KEYFRAMES_INTERVALTYPE_(Frames)), 1);
		}
		{
			auto p =
			    obs_properties_add_float(grp, G_KEYFRAMES_INTERVAL_SECONDS, TRANSLATE(G_KEYFRAMES_INTERVAL),
			                             0.00, std::numeric_limits<int16_t>::max(), 0.01);
			obs_property_set_long_description(p, TRANSLATE(DESC(G_KEYFRAMES_INTERVAL)));
			obs_property_float_set_suffix(p, " seconds");
		}
		{
			auto p =
			    obs_properties_add_int(grp, G_KEYFRAMES_INTERVAL_FRAMES, TRANSLATE(G_KEYFRAMES_INTERVAL), 0,
			                           std::numeric_limits<int32_t>::max(), 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(G_KEYFRAMES_INTERVAL)));
			obs_property_int_set_suffix(p, " frames");
		}
	}

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, P_AQ, TRANSLATE(P_AQ), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p = obs_properties_add_bool(grp, P_AQ_SPATIAL, TRANSLATE(P_AQ_SPATIAL));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_AQ_SPATIAL)));
			obs_property_set_modified_callback(p, modified_aq);
		}
		{
			auto p = obs_properties_add_int_slider(grp, P_AQ_STRENGTH, TRANSLATE(P_AQ_STRENGTH), 1, 15, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_AQ_STRENGTH)));
		}
		{
			auto p = obs_properties_add_bool(grp, P_AQ_TEMPORAL, TRANSLATE(P_AQ_TEMPORAL));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_AQ_TEMPORAL)));
		}
	}

	{
		obs_properties_t* grp = props;
		if (!obsffmpeg::are_property_groups_broken()) {
			grp = obs_properties_create();
			obs_properties_add_group(props, P_OTHER, TRANSLATE(P_OTHER), OBS_GROUP_NORMAL, grp);
		}

		{
			auto p =
			    obs_properties_add_int_slider(grp, P_OTHER_BFRAMES, TRANSLATE(P_OTHER_BFRAMES), 0, 4, 1);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_OTHER_BFRAMES)));
			obs_property_int_set_suffix(p, " frames");
		}

		{
			auto p = obs_properties_add_list(grp, P_OTHER_BFRAME_REFERENCEMODE,
			                                 TRANSLATE(P_OTHER_BFRAME_REFERENCEMODE), OBS_COMBO_TYPE_LIST,
			                                 OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, TRANSLATE(DESC(P_OTHER_BFRAME_REFERENCEMODE)));
			for (auto kv : b_ref_modes) {
				obs_property_list_add_int(p, TRANSLATE(kv.second.c_str()),
				                          static_cast<int64_t>(kv.first));
			}
		}

		{
			auto p = obs_properties_add_bool(grp, P_OTHER_ZEROLATENCY, TRANSLATE(P_OTHER_ZEROLATENCY));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_OTHER_ZEROLATENCY)));
		}

		{
			auto p = obs_properties_add_bool(grp, P_OTHER_WEIGHTED_PREDICTION,
			                                 TRANSLATE(P_OTHER_WEIGHTED_PREDICTION));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_OTHER_WEIGHTED_PREDICTION)));
		}

		{
			auto p = obs_properties_add_bool(grp, P_OTHER_NONREFERENCE_PFRAMES,
			                                 TRANSLATE(P_OTHER_NONREFERENCE_PFRAMES));
			obs_property_set_long_description(p, TRANSLATE(DESC(P_OTHER_NONREFERENCE_PFRAMES)));
		}
	}
}

void obsffmpeg::nvenc::get_runtime_properties(obs_properties_t* props, AVCodec*, AVCodecContext*)
{
	obs_property_set_enabled(obs_properties_get(props, P_PRESET), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_MODE), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_TWOPASS), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_LOOKAHEAD), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_ADAPTIVEI), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_ADAPTIVEB), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_BITRATE), true);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_BITRATE_TARGET), true);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_BITRATE_MAXIMUM), true);
	obs_property_set_enabled(obs_properties_get(props, G_RATECONTROL_BUFFERSIZE), true);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QUALITY), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QUALITY_MINIMUM), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QUALITY_MAXIMUM), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP_I), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP_I_INITIAL), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP_P), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP_P_INITIAL), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP_B), false);
	obs_property_set_enabled(obs_properties_get(props, P_RATECONTROL_QP_B_INITIAL), false);
	obs_property_set_enabled(obs_properties_get(props, G_KEYFRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, G_KEYFRAMES_INTERVALTYPE), false);
	obs_property_set_enabled(obs_properties_get(props, G_KEYFRAMES_INTERVAL_SECONDS), false);
	obs_property_set_enabled(obs_properties_get(props, G_KEYFRAMES_INTERVAL_FRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, P_AQ), false);
	obs_property_set_enabled(obs_properties_get(props, P_AQ_SPATIAL), false);
	obs_property_set_enabled(obs_properties_get(props, P_AQ_STRENGTH), false);
	obs_property_set_enabled(obs_properties_get(props, P_AQ_TEMPORAL), false);
	obs_property_set_enabled(obs_properties_get(props, P_OTHER), false);
	obs_property_set_enabled(obs_properties_get(props, P_OTHER_BFRAMES), false);
	obs_property_set_enabled(obs_properties_get(props, P_OTHER_BFRAME_REFERENCEMODE), false);
	obs_property_set_enabled(obs_properties_get(props, P_OTHER_ZEROLATENCY), false);
	obs_property_set_enabled(obs_properties_get(props, P_OTHER_WEIGHTED_PREDICTION), false);
	obs_property_set_enabled(obs_properties_get(props, P_OTHER_NONREFERENCE_PFRAMES), false);
}

void obsffmpeg::nvenc::update(obs_data_t* settings, AVCodec* codec, AVCodecContext* context)
{
	{
		preset c_preset = static_cast<preset>(obs_data_get_int(settings, P_PRESET));
		auto   found    = preset_to_opt.find(c_preset);
		if (found != preset_to_opt.end()) {
			av_opt_set(context->priv_data, "preset", found->second.c_str(), 0);
		} else {
			av_opt_set(context->priv_data, "preset", nullptr, 0);
		}
	}

	{ // Rate Control
		bool have_bitrate     = false;
		bool have_bitrate_max = false;
		bool have_quality_min = false;
		bool have_quality_max = false;
		bool have_qp          = false;
		bool have_qp_init     = false;

		ratecontrolmode rc    = static_cast<ratecontrolmode>(obs_data_get_int(settings, P_RATECONTROL_MODE));
		auto            rcopt = nvenc::ratecontrolmode_to_opt.find(rc);
		if (rcopt != nvenc::ratecontrolmode_to_opt.end()) {
			av_opt_set(context->priv_data, "rc", rcopt->second.c_str(), 0);
		}

		switch (rc) {
		case ratecontrolmode::CQP:
			have_qp = true;
			break;
		case ratecontrolmode::CBR:
		case ratecontrolmode::CBR_HQ:
		case ratecontrolmode::CBR_LD_HQ:
			have_bitrate = true;
			av_opt_set_int(context->priv_data, "cbr", 1, 0);
			break;
		case ratecontrolmode::VBR:
		case ratecontrolmode::VBR_HQ:
			have_bitrate_max = true;
			have_bitrate     = true;
			have_quality_min = true;
			have_quality_max = true;
			have_qp_init     = true;
			break;
		}

		int tp = static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_TWOPASS));
		if (tp >= 0) {
			av_opt_set_int(context->priv_data, "2pass", tp ? 1 : 0, 0);
		}

		int la = static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_LOOKAHEAD));
		av_opt_set_int(context->priv_data, "lookahead", la, 0);
		if (la > 0) {
			bool adapt_i = obs_data_get_bool(settings, P_RATECONTROL_ADAPTIVEI);
			av_opt_set_int(context->priv_data, "no-scenecut", !adapt_i ? 1 : 0, 0);

			if (strcmp(codec->name, "h264_nvenc")) {
				bool adapt_b = obs_data_get_bool(settings, P_RATECONTROL_ADAPTIVEB);
				av_opt_set_int(context->priv_data, "b_adapt", adapt_b ? 1 : 0, 0);
			}
		}

		if (have_bitrate)
			context->bit_rate =
			    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BITRATE_TARGET) * 1000);
		if (have_bitrate_max)
			context->rc_max_rate =
			    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_BITRATE_MAXIMUM) * 1000);
		if (have_bitrate || have_bitrate_max)
			context->rc_buffer_size =
			    static_cast<int>(obs_data_get_int(settings, G_RATECONTROL_BUFFERSIZE) * 1000);

		if (have_quality_min && obs_data_get_bool(settings, P_RATECONTROL_QUALITY)) {
			int qmin      = static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_QUALITY_MINIMUM));
			context->qmin = qmin;
			if ((qmin >= 0) && (have_quality_max)) {
				context->qmax =
				    static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_QUALITY_MAXIMUM));
			}
		}

		if (have_qp) {
			av_opt_set_int(context->priv_data, "init_qpI",
			               static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_QP_I)), 0);
			av_opt_set_int(context->priv_data, "init_qpP",
			               static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_QP_P)), 0);
			av_opt_set_int(context->priv_data, "init_qpB",
			               static_cast<int>(obs_data_get_int(settings, P_RATECONTROL_QP_B)), 0);
		}
		if (have_qp_init) {
			av_opt_set_int(context->priv_data, "init_qpI",
			               obs_data_get_int(settings, P_RATECONTROL_QP_I_INITIAL), 0);
			av_opt_set_int(context->priv_data, "init_qpP",
			               obs_data_get_int(settings, P_RATECONTROL_QP_P_INITIAL), 0);
			av_opt_set_int(context->priv_data, "init_qpB",
			               obs_data_get_int(settings, P_RATECONTROL_QP_B_INITIAL), 0);
		}
	}

	{ // Key Frames
		obs_video_info ovi;
		if (!obs_get_video_info(&ovi)) {
			throw std::runtime_error("no video info");
		}

		int64_t kf_type    = obs_data_get_int(settings, G_KEYFRAMES_INTERVALTYPE);
		bool    is_seconds = (kf_type == 0);

		if (is_seconds) {
			context->gop_size = static_cast<int>(obs_data_get_double(settings, G_KEYFRAMES_INTERVAL_SECONDS)
			                                     * (ovi.fps_num / ovi.fps_den));
		} else {
			context->gop_size = static_cast<int>(obs_data_get_int(settings, G_KEYFRAMES_INTERVAL_FRAMES));
		}
		context->keyint_min = context->gop_size;
	}

	{ // AQ
		bool saq = obs_data_get_bool(settings, P_AQ_SPATIAL);
		bool taq = obs_data_get_bool(settings, P_AQ_TEMPORAL);

		if (strcmp(codec->name, "h264_nvenc")) {
			av_opt_set_int(context->priv_data, "spatial-aq", saq ? 1 : 0, 0);
			av_opt_set_int(context->priv_data, "temporal-aq", taq ? 1 : 0, 0);
		} else {
			av_opt_set_int(context->priv_data, "spatial_aq", saq ? 1 : 0, 0);
			av_opt_set_int(context->priv_data, "temporal_aq", taq ? 1 : 0, 0);
		}
		if (saq) {
			av_opt_set_int(context->priv_data, "aq-strength",
			               static_cast<int>(obs_data_get_int(settings, P_AQ_STRENGTH)), 0);
		}
	}

	{ // Other
		bool zl  = obs_data_get_bool(settings, P_OTHER_ZEROLATENCY);
		bool wp  = obs_data_get_bool(settings, P_OTHER_WEIGHTED_PREDICTION);
		bool nrp = obs_data_get_bool(settings, P_OTHER_NONREFERENCE_PFRAMES);

		context->max_b_frames = static_cast<int>(obs_data_get_int(settings, P_OTHER_BFRAMES));

		av_opt_set_int(context->priv_data, "zerolatency", zl ? 1 : 0, 0);
		av_opt_set_int(context->priv_data, "nonref_p", nrp ? 1 : 0, 0);

		if ((context->max_b_frames != 0) && wp) {
			PLOG_WARNING(
			    "Automatically disabled weighted prediction due to being incompatible with B-Frames.");
		} else {
			av_opt_set_int(context->priv_data, "weighted_pred", wp ? 1 : 0, 0);
		}

		{
			auto found = b_ref_mode_to_opt.find(
			    static_cast<b_ref_mode>(obs_data_get_int(settings, P_OTHER_BFRAME_REFERENCEMODE)));
			if (found != b_ref_mode_to_opt.end()) {
				av_opt_set(context->priv_data, "b_ref_mode", found->second.c_str(), 0);
			}
		}
	}
}
