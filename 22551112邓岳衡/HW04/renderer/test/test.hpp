#pragma once

#include <memory>
#include "../core/api.hpp"

// 测试Bresenham画线算法
inline void test(FrameBuffer* buffer) {
	int x1, x2, y1, y2, target_index;
	x1 = 0, y1 = 0;
	x2 = 500, y2 = 600;

	draw_line(x1, y1, x2, y2, Color::White, buffer);
}
// 测试线框模型绘制
inline void test_model_line(Model& model, FrameBuffer* buffer) {
	int f_size = model.nfaces(), v_size = model.nverts();
	int x1, y1, x2, y2;
	vec3f p1, p2;
	std::vector<int> indeies;

	for (int i = 0; i < f_size; i++) {
		//std::cout << "max f_size = " << f_size << " current i = " << i << std::endl;
		indeies = model.face(i);
		int j_size = indeies.size();
		//std::cout << "current j_size = " << j_size << std::endl;
		for (int j = 0; j < j_size; j++) {
			p1 = model.vert(indeies[j]);
			p2 = model.vert(indeies[(j + 1) % j_size]);
			x1 = (p1.x + 1.f) * (buffer->width - 1) / 2.f;
			y1 = (p1.y + 1.f) * (buffer->height - 1) / 2.f;
			x2 = (p2.x + 1.f) * (buffer->width - 1) / 2.f;
			y2 = (p2.y + 1.f) * (buffer->height - 1) / 2.f;

			//std::cerr << "(x1,y1):" << x1 << " " << y1 << " (x2,y2):" << x2 << " " << y2 << std::endl;

			draw_line(x1, y1, x2, y2, Color::White, buffer);
		}
	}
}
inline void test_model_triangle(Model& model, FrameBuffer* buffer) {
	int f_size = model.nfaces();
	vec3f p;
	int x, y;
	std::vector<int> indeies;
	std::vector<vec4f> screen_coords(3);
	std::vector<vec3f> world_coord(3);

	// 临时平行光
	vec3f light_dir(0, 0, -1), trian_normal;

	for (int i = 0; i < f_size; i++) {
		//std::cout << "max f_size = " << f_size << " current i = " << i << std::endl;
		indeies = model.face(i);
		int j_size = indeies.size();
		screen_coords.clear();
		world_coord.clear();

		//std::cout << "current j_size = " << j_size << std::endl;
		for (int j = 0; j < j_size; j++) {
			p = model.vert(indeies[j]);
			// 简单的正交变换 + ViewPort：[-1, 1] -> [0, width * height]
			screen_coords.push_back(vec4f(viewport(p, buffer->width, buffer->height), p.z));
			world_coord.push_back(p);

			//std::cerr << "x,y: " << x << ", " << y << std::endl;
		}
		//std::cerr << "face over--------" << std::endl;
		trian_normal = cross(world_coord[2] - world_coord[0], world_coord[1] - world_coord[0]);
		trian_normal = trian_normal.normalize();

		float intensity = dot(light_dir, trian_normal);

		// 类似背面剔除，负数则与视线（这里与光线同向）方向相同剔除
		if (intensity > 0) {
			draw_triangle_bbox(screen_coords, buffer, vec4f(intensity, intensity, intensity, 1.0));
		}
	}
}

inline void test_model_triangle_with_camera(Model& model, const Camera& camera, FrameBuffer* buffer) {
	int f_size = model.nfaces();
	vec4f p;
	vec2i uv;
	int x, y;
	std::vector<int> indeies;
	std::vector<vec4f> screen_coords;
	std::vector<vec3f> world_coord;
	std::vector<vec2f> uvs;
	std::vector<vec3f> normals;

	// 临时平行光（同相机方向）
	vec3f light_dir, trian_normal;
	light_dir = -camera.get_toward();
	light_dir = light_dir.normalize();

	// 透视矩阵
	mat4f project = camera.get_perspective_matrix();

	for (int i = 0; i < f_size; i++) {
		//std::cout << "max f_size = " << f_size << " current i = " << i << std::endl;
		indeies = model.face(i);
		int j_size = indeies.size();
		screen_coords.clear();
		world_coord.clear();
		uvs.clear();
		normals.clear();

		//std::cout << "current j_size = " << j_size << std::endl;
		for (int j = 0; j < j_size; j++) {
			p = vec4f(model.vert(indeies[j]), 1.f);
			uvs.push_back(model.uv(i, j));
			normals.push_back(model.normal(i, j));
			mat4f view = camera.get_view_matrix();
			mat4f mvp = project * view;
			vec4f clip_coord = mvp * p;	// MVP变换：局部空间 -> 世界空间 -> 观察空间 -> 裁剪空间
			//std::cout << "world_coord: " << p.x << ", " << p.y << ", " << p.z << ", " << p.w << std::endl;
			//std::cout << "clip_coord: " << clip_coord.x << ", " << clip_coord.y << ", " << clip_coord.z << ", " << clip_coord.w << std::endl;
			
			vec4f ndc_coord = clip_coord / clip_coord.w;	// 齐次裁剪：裁剪空间 -> NDC
			//std::cout << "ndc_coord: " << p.x << ", " << p.y << ", " << p.z << ", " << p.w << std::endl;
			world_coord.push_back(proj<3>(p));

			// 简单的正交变换 + ViewPort：[-1, 1] -> [0, width * height]
			screen_coords.push_back(vec4f(viewport(proj<3>(ndc_coord), buffer->width, buffer->height), clip_coord.z));

			//std::cerr << "x,y: " << x << ", " << y << std::endl;
		}
		//std::cerr << "face over--------" << std::endl;

		for (int j = 0; j < j_size; j += 2) {
			size_t index_A = j, index_B = (j + 1) % j_size, index_C = (j + 2) % j_size;
			trian_normal = cross(world_coord[index_B] - world_coord[index_A], world_coord[index_C] - world_coord[index_A]);
			trian_normal = trian_normal.normalize();
			float intensity = dot(light_dir, trian_normal);
			
			std::vector<vec4f> draw_points{
				screen_coords[index_A],
				screen_coords[index_B],
				screen_coords[index_C]
			};
			
			// 类似背面剔除，负数则与视线（这里与光线同向）方向相同剔除
			if (intensity > 0) {
				// draw_triangle_bbox(draw_points, buffer, vec4f(intensity, intensity, intensity, 1.0));
				draw_triangle_bbox_texture(model, draw_points, uvs, normals, light_dir, buffer);
			}
			else {
				draw_triangle_bbox(draw_points, buffer, Color::Red);
			}
		}
	}
}

// 测试三角重心计算是否正确
static vec3f barycentic_for_TEST(vec3f P, std::vector<vec3f> triangle_points) {
	vec3f s[2];
	vec3f A = triangle_points[0], B = triangle_points[1], C = triangle_points[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	vec3f u = cross(s[0], s[1]);
	if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
		return vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}
inline void test_barycentic_with_triangle(FrameBuffer* buffer) {
	std::vector<vec3f> triangle_points;
	triangle_points.push_back(vec3f(350.f, 200.f, 0.f));	// A: Red
	triangle_points.push_back(vec3f(200.f, 350.f, 0.f));	// B: Blue
	triangle_points.push_back(vec3f(700.f, 700.f, 0.f));	// C: Green
	
	vec2f bbox_max(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	vec2f bbox_min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec2f clamp(buffer->width - 1, buffer->height - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bbox_max[j] = std::min(clamp[j], (std::max(bbox_max[j], triangle_points[i][j])));
			bbox_min[j] = std::max(0.f, (std::min(bbox_min[j], triangle_points[i][j])));
		}
	}

	for (int i = bbox_min.x; i <= (int)bbox_max.x; i++) {
		for (int j = bbox_min.y; j <= (int)bbox_max.y; j++) {
			vec3f barycentic_weights = barycentric(vec3f(i, j, 1.f), triangle_points);
			// vec3f barycentic_weights = barycentic_for_TEST(vec3f(i, j, 1.f), triangle_points);

			if (barycentic_weights.x < 0 || barycentic_weights.y < 0 || barycentic_weights.z < 0) {
				continue;
			}

			vec4f color = barycentic_weights[0] * Color::Red + barycentic_weights[1] * Color::Blue + barycentic_weights[2] * Color::Green;
			vec4f _color = vec4f(barycentic_weights, 1.f);
			buffer->set_color(i, j, color);
		}
	}
}

// 测试IShader渲染管线是否成功
// 本函数功能包括：每帧根据Camera更新Shader
void test_myShadingPipeLine(Model& model, Camera& camera, Camera& light, IShader& shader, ShadowShader& shadow_shader, FrameBuffer* buffer) {
	// 更新光照和阴影属性
	shadow_shader.shader_data->camera_vp_matrix = light.get_perspective_matrix() * light.get_view_matrix();
	
	// 更新着色器数据
	shader.shader_data->light_pos = light.get_position();
	shader.shader_data->light_dir = light.get_toward().normalize();
	shader.shader_data->light_view_matrix = shadow_shader.shader_data->camera_vp_matrix;

	shader.shader_data->projective_matrix = camera.get_perspective_matrix();
	shader.shader_data->view_pos = camera.get_position();
	shader.shader_data->view_matrix = camera.get_view_matrix();
	shader.shader_data->camera_vp_matrix = camera.get_perspective_matrix() * camera.get_view_matrix();

	// 渲染shadow map
	if (shader.shader_data->shadow_enable) {
		shadow_shader.shader_data->buffer->framebuffer_clear_depth(1);
		draw_shadow_map(model, shadow_shader, shadow_shader.shader_data->buffer);
	}

	// 渲染模型
	draw_model(model, shader, buffer);
}