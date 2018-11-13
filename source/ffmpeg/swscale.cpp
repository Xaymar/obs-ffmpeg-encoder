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

#include "swscale.hpp"
#include <stdexcept>

ffmpeg::swscale::swscale() {}

ffmpeg::swscale::~swscale()
{
	finalize();
}

void ffmpeg::swscale::set_source_size(uint32_t width, uint32_t height)
{
	source_size.first  = width;
	source_size.second = height;
}

void ffmpeg::swscale::get_source_size(uint32_t& width, uint32_t& height)
{
	width  = this->source_size.first;
	height = this->source_size.second;
}

std::pair<uint32_t, uint32_t> ffmpeg::swscale::get_source_size()
{
	return this->source_size;
}

uint32_t ffmpeg::swscale::get_source_width()
{
	return this->source_size.first;
}

uint32_t ffmpeg::swscale::get_source_height()
{
	return this->source_size.second;
}

void ffmpeg::swscale::set_source_format(AVPixelFormat format)
{
	source_format = format;
}

AVPixelFormat ffmpeg::swscale::get_source_format()
{
	return this->source_format;
}

void ffmpeg::swscale::set_source_color(bool full_range, AVColorSpace space)
{
	source_full_range = full_range;
	source_colorspace = space;
}

void ffmpeg::swscale::set_source_colorspace(AVColorSpace space)
{
	this->source_colorspace = space;
}

AVColorSpace ffmpeg::swscale::get_source_colorspace()
{
	return this->source_colorspace;
}

void ffmpeg::swscale::set_source_full_range(bool full_range)
{
	this->source_full_range = full_range;
}

bool ffmpeg::swscale::is_source_full_range()
{
	return this->source_full_range;
}

void ffmpeg::swscale::set_target_size(uint32_t width, uint32_t height)
{
	target_size.first  = width;
	target_size.second = height;
}

void ffmpeg::swscale::get_target_size(uint32_t& width, uint32_t& height) {}

std::pair<uint32_t, uint32_t> ffmpeg::swscale::get_target_size()
{
	return this->target_size;
}

uint32_t ffmpeg::swscale::get_target_width()
{
	return this->target_size.first;
}

uint32_t ffmpeg::swscale::get_target_height()
{
	return this->target_size.second;
}

void ffmpeg::swscale::set_target_format(AVPixelFormat format)
{
	target_format = format;
}

AVPixelFormat ffmpeg::swscale::get_target_format()
{
	return this->target_format;
}

void ffmpeg::swscale::set_target_color(bool full_range, AVColorSpace space)
{
	target_full_range = full_range;
	target_colorspace = space;
}

void ffmpeg::swscale::set_target_colorspace(AVColorSpace space)
{
	this->target_colorspace = space;
}

AVColorSpace ffmpeg::swscale::get_target_colorspace()
{
	return this->target_colorspace;
}

void ffmpeg::swscale::set_target_full_range(bool full_range)
{
	this->target_full_range = full_range;
}

bool ffmpeg::swscale::is_target_full_range()
{
	return this->target_full_range;
}

bool ffmpeg::swscale::initialize(int flags)
{
	if (this->context) {
		return false;
	}
	if (source_size.first == 0 || source_size.second == 0 || source_format == AV_PIX_FMT_NONE
	    || source_colorspace == AVCOL_SPC_UNSPECIFIED) {
		throw std::invalid_argument("not all source parameters were set");
	}
	if (target_size.first == 0 || target_size.second == 0 || target_format == AV_PIX_FMT_NONE
	    || target_colorspace == AVCOL_SPC_UNSPECIFIED) {
		throw std::invalid_argument("not all target parameters were set");
	}

	this->context = sws_getContext(source_size.first, source_size.second, source_format, target_size.first,
	                               target_size.second, target_format, flags, nullptr, nullptr, nullptr);
	if (!this->context) {
		return false;
	}

	sws_setColorspaceDetails(this->context, sws_getCoefficients(source_colorspace), source_full_range ? 1 : 0,
	                         sws_getCoefficients(target_colorspace), target_full_range ? 1 : 0, 1l << 16 | 0l,
	                         1l << 16 | 0l, 1l << 16 | 0l);

	return true;
}

bool ffmpeg::swscale::finalize()
{
	if (this->context) {
		sws_freeContext(this->context);
		this->context = nullptr;
		return true;
	}
	return false;
}

int32_t ffmpeg::swscale::convert(const uint8_t* const source_data[], const int source_stride[], int32_t source_row,
                                 int32_t source_rows, uint8_t* const target_data[], const int target_stride[])
{
	if (!this->context) {
		return 0;
	}
	int height =
	    sws_scale(this->context, source_data, source_stride, source_row, source_rows, target_data, target_stride);
	return height;
}
