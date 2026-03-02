#include "framebuffer.hpp"
#include <cassert>
#include <iostream>

FrameBuffer::FrameBuffer(int width, int height) {
	int color_buffer_size = width * height * 4;
	//int depth_buffer_size = sizeof(float) * width * height;
	int depth_buffer_size = width * height;
	vec4f default_color = { 0.f, 0.f, 0.f, 1.f };
	float default_depth = 1;

	assert(width > 0 && height > 0);

	this->width = width;
	this->height = height;
	this->color_buffer = new unsigned char[color_buffer_size];
	this->depth_buffer = new float[depth_buffer_size];

	this->framebuffer_clear_color(default_color);
	this->framebuffer_clear_depth(default_depth);
}

FrameBuffer::~FrameBuffer()
{
	delete[] color_buffer;
	delete[] depth_buffer;
}


void FrameBuffer::set_depth(int x, int y, float depth) {
	int index = y * width + x;
	depth_buffer[index] = depth;
}

float FrameBuffer::get_depth(int x, int y)
{
	int index = y * width + x;
	return depth_buffer[index];
}

void FrameBuffer::set_color(int x, int y, vec4f color)
{
	int index = (y * width + x) * 4;
	//if (index >= width * height * 4) {
	//	std::cerr << "fail x: " << x << " y: " << y << std::endl;
	//}
	color_buffer[index + 0] = color.r * 255;
	color_buffer[index + 1] = color.g * 255;
	color_buffer[index + 2] = color.b * 255;
}

vec4f FrameBuffer::get_color(int x, int y)
{
	int index = (y * width + x) * 4;
	return vec4f(color_buffer[index + 0] / 255.f, color_buffer[index + 1] / 255.f, color_buffer[index + 2] / 255.f);
}

void FrameBuffer::framebuffer_clear_color(vec4f color) {
	int num_pixels = this->width * this->height;
	int i;
	for (i = 0; i < num_pixels; i++) {
		this->color_buffer[i * 4 + 0] = color.r * 255;
		this->color_buffer[i * 4 + 1] = color.g * 255;
		this->color_buffer[i * 4 + 2] = color.b * 255;
		this->color_buffer[i * 4 + 3] = color.a * 255;
	}
}

void FrameBuffer::framebuffer_clear_depth(float depth) {
	int num_pixels = this->width * this->height;
	int i;
	for (i = 0; i < num_pixels; i++) {
		this->depth_buffer[i] = depth;
	}
}