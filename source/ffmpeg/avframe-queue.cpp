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

#include "avframe-queue.hpp"
#include "tools.hpp"

AVFrame* ffmpeg::avframe_queue::create_frame()
{
	AVFrame* frame = av_frame_alloc();
	frame->width   = this->resolution.first;
	frame->height  = this->resolution.second;
	frame->format  = this->format;
	int res        = av_frame_get_buffer(frame, 0);
	if (res < 0) {
		throw std::exception(ffmpeg::tools::get_error_description(res));
	}

	return frame;
}

void ffmpeg::avframe_queue::destroy_frame(AVFrame* frame)
{
	if (frame == nullptr)
		return;

	av_frame_free(&frame);
}

ffmpeg::avframe_queue::avframe_queue() {}

ffmpeg::avframe_queue::~avframe_queue()
{
	clear();
}

void ffmpeg::avframe_queue::set_resolution(uint32_t width, uint32_t height)
{
	this->resolution.first  = width;
	this->resolution.second = height;
}

void ffmpeg::avframe_queue::get_resolution(uint32_t& width, uint32_t& height)
{
	width  = this->resolution.first;
	height = this->resolution.second;
}

uint32_t ffmpeg::avframe_queue::get_width()
{
	return this->resolution.first;
}

uint32_t ffmpeg::avframe_queue::get_height()
{
	return this->resolution.second;
}

void ffmpeg::avframe_queue::set_pixel_format(AVPixelFormat format)
{
	this->format = format;
}

AVPixelFormat ffmpeg::avframe_queue::get_pixel_format()
{
	return this->format;
}

void ffmpeg::avframe_queue::precache(size_t count)
{
	for (size_t n = 0; n < count; n++) {
		push(create_frame());
	}
}

void ffmpeg::avframe_queue::clear()
{
	std::unique_lock<std::mutex> ulock(this->lock);
	for (AVFrame* frame : frames) {
		destroy_frame(frame);
	}
	frames.clear();
}

void ffmpeg::avframe_queue::push(AVFrame* frame)
{
	std::unique_lock<std::mutex> ulock(this->lock);
	frames.push_back(frame);
}

AVFrame* ffmpeg::avframe_queue::pop()
{
	std::unique_lock<std::mutex> ulock(this->lock);

	AVFrame* ret = nullptr;
	while (ret == nullptr) {
		if (frames.size() == 0) {
			ret = create_frame();
		} else {
			ret = frames.front();
			if (ret == nullptr) {
				ret = create_frame();
			} else {
				frames.pop_front();
				if ((ret->width != this->resolution.first) || (ret->height != this->resolution.second)
				    || (ret->format != this->format)) {
					destroy_frame(ret);
					ret = nullptr;
				}
			}
		}
	}
	return ret;
}

AVFrame* ffmpeg::avframe_queue::pop_only()
{
	std::unique_lock<std::mutex> ulock(this->lock);
	if (frames.size() == 0) {
		return nullptr;
	}
	AVFrame* ret = frames.front();
	if (ret == nullptr) {
		return nullptr;
	}
	frames.pop_front();
	return ret;
}

bool ffmpeg::avframe_queue::empty()
{
	return frames.empty();
}

size_t ffmpeg::avframe_queue::size()
{
	return frames.size();
}
