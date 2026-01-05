#pragma once

#include "maths.hpp"

class FrameBuffer {
public:
	FrameBuffer(int width, int height);
	~FrameBuffer();

	int width, height;
	unsigned char* color_buffer;
	float* depth_buffer;

	void set_depth(int x, int y, float depth);
	float get_depth(int x, int y);
	void set_color(int x, int y, vec4f color);
	vec4f get_color(int x, int y);

	void framebuffer_clear_color(vec4f color);
	void framebuffer_clear_depth(float depth);
};