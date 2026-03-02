#pragma once
#include <vector>

#include "maths.hpp"
#include "framebuffer.hpp"
#include "model.hpp"
#include "IShader.hpp"

vec3f viewport(vec3f ndc_coord, int width, int height);

mat4f lookAt(vec3f pos, vec3f target, vec3f up);

mat4f perspective(float fovY, float aspect_ratio, float near, float far);

mat4f ortho(float right, float top, float near, float far);

mat4f translate(float tx, float ty, float tz);

mat4f scale(float sx, float sy, float sz);

mat4f rotate(vec3f n, float radians);

vec3f barycentric(vec3f P, std::vector<vec3f> triangle_points);

void draw_line(int x1, int y1, int x2, int y2, vec4f color, FrameBuffer* buffer);

void draw_triangle_normal(std::vector<vec3f> points, FrameBuffer* buffer, vec4f color);

void draw_triangle_bbox(std::vector<vec4f> points, FrameBuffer* buffer, vec4f color);

void draw_triangle_bbox_texture(Model& model, std::vector<vec4f> points, std::vector<vec2f> uvs, std::vector<vec3f> normals, vec3f light_dir, FrameBuffer* buffer);

void rasterize_triangle(const std::vector<shader_data_v2f>& v2fs, IShader& shader, FrameBuffer* buffer);

void draw_model(Model& model, IShader& shader, FrameBuffer* buffer);

void rasterize_triangle_shadow(const std::vector<shader_data_v2f>& v2fs, IShader& shader, FrameBuffer* buffer);

void draw_shadow_map(Model& model, ShadowShader& shader, FrameBuffer* shadow_buffer);