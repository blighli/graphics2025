#include "graphics.hpp"
#include <cassert>

// 将NDC的坐标转换到屏幕坐标
vec3f viewport(vec3f ndc_coord, int width, int height) {
	vec3f screen_coord;
	screen_coord.x = (ndc_coord.x + 1.f) * width / 2;	// [-1, 1] -> [0, width]
	screen_coord.y = (ndc_coord.y + 1.f) * height / 2;	// [-1, 1] -> [0, height] 
	screen_coord.z = (ndc_coord.z + 1.f) / 2;			// [-1, 1] -> [0, 1]
	return screen_coord;
}

mat4f lookAt(vec3f pos, vec3f target, vec3f up) {
	vec3f camera_toward = (pos - target).normalize();
	vec3f camera_right = cross(up, camera_toward).normalize();
	vec3f camera_up = cross(camera_toward, camera_right).normalize();

	mat4f lookat = mat4f::identity();
	lookat[0] = vec4f(camera_right, 0.f);
	lookat[1] = vec4f(camera_up, 0.f);
	lookat[2] = vec4f(camera_toward, 0.f);

	lookat[0][3] = -dot(pos, camera_right);
	lookat[1][3] = -dot(pos, camera_up);
	lookat[2][3] = -dot(pos, camera_toward);

	return lookat;
}

// 此处的fovY为弧度即可
// 参考songho网站：https://www.songho.ca/opengl/gl_projectionmatrix.html
mat4f perspective(float fovY, float aspect_ratio, float near, float far) {
	assert(fovY > 0 && aspect_ratio > 0);
	float tangent = std::tan(fovY / 2);
	float z_range = far - near;

	mat4f ret;
	ret[1][1] = 1 / tangent;						// near/top
	ret[0][0] = ret[1][1] / aspect_ratio;			// near/right
	ret[2][2] = -(far + near) / z_range;			// -(f + n) / (f - n)
	ret[2][3] = -(2 * far * near) / z_range;		// -2fn / (f - n)
	ret[3][2] = -1;
	return ret;
}
// 参考songho网站：https://www.songho.ca/opengl/gl_projectionmatrix.html
mat4f ortho(float right, float top, float near, float far) {
	float z_range = far - near;
	assert(right > 0 && top > 0 && z_range > 0);

	mat4f ret = mat4f::identity();
	ret[0][0] = 1 / right;
	ret[1][1] = 1 / top;
	ret[2][2] = -2 / z_range;
	ret[2][3] = -(far + near) / z_range;
	return ret;
}


// 使用Bresenham画线法
void draw_line(int x1, int y1, int x2, int y2, vec4f color, FrameBuffer* buffer) {
	bool steep = false;
	if (std::abs(x2 - x1) < std::abs(y2 - y1)) {
		std::swap(x1, y1);
		std::swap(x2, y2);
		steep = true;
	}
	if (x1 >= x2) {
		std::swap(x1, x2);
		std::swap(y1, y2);
	}

	int dx = x2 - x1, dx2 = 2 * dx, dy = y2 - y1, dy2 = std::abs(2 * dy);
	int error = dx, step_y = 1, curr_y = y1;
	if (dy < 0) {
		step_y = -1;
	}

	for (int i = x1; i <= x2; i++) {
		if (steep) {
			buffer->set_color(curr_y, i, color);
		}
		else {
			buffer->set_color(i, curr_y, color);
		}
		error -= dy2;
		if (error < 0) {
			error += dx2;
			curr_y += step_y;
		}
	}
}

// 上下两部分逐行扫描
void draw_triangle_normal(std::vector<vec3f> points, FrameBuffer* buffer, vec4f color) {
	if (points[0].y == points[1].y && points[1].y == points[2].y) {
		return;
	}

	if (points[0].y > points[1].y) {
		std::swap(points[0], points[1]);
	}
	if (points[0].y > points[2].y) {
		std::swap(points[0], points[2]);
	}
	if (points[1].y > points[2].y) {
		std::swap(points[1], points[2]);
	}

	int left, right, current_height = points[0].y;
	int dx_02 = points[2].x - points[0].x, dy_02 = points[2].y - points[0].y,
		dx_01 = points[1].x - points[0].x, dy_01 = points[1].y - points[0].y,
		dx_12 = points[2].x - points[1].x, dy_12 = points[2].y - points[1].y;
	bool isHalfed = false;
	for (int i = 0; i <= dy_02; i++, current_height++) {
		isHalfed = (i >= dy_01);
		left = (double)i * dx_02 / dy_02 + points[0].x;
		right = isHalfed ? dy_12 ? (double(i - dy_01) * dx_12 / dy_12 + points[1].x) : points[1].x : ((double)i * dx_01 / dy_01 + points[0].x);
		
		if (left > right) {
			std::swap(left, right);
		}
		for (int j = left; j <= right; j++) {
			buffer->set_color(j, current_height, color);
		}
	}
}

// 计算重心坐标
vec3f barycentric(vec3f P, std::vector<vec3f> triangle_points) {
	vec3f tmp[2];
	for (int i = 0; i < 2; i++) {
		tmp[i].x = triangle_points[0][i] - P[i];						// PA
		tmp[i].y = triangle_points[1][i] - triangle_points[0][i];		// AB
		tmp[i].z = triangle_points[2][i] - triangle_points[0][i];		// AC
	}

	// P = (1 - u - v)*A + u*B + v*C
	// U = [1, u, v];
	vec3f U = cross(tmp[0], tmp[1]);
	U = U / U.x;

	// 三角形非法
	if (std::abs(U[0]) < 2e-3) {
		return vec3f(-1, 1, 1);
	}
	return vec3f(1.f - U.y - U.z, U.y, U.z);
}

// 使用bbox遍历三角形（同games101）
// 此处的points应在 ViewSpace 下
// TODO: 这里是vertex输出与fragment之间的桥梁，得到的某像素的插值数据应该传入FS中得到color
// 得到color后再测试，但这里可以提前
void draw_triangle_bbox(std::vector<vec4f> points, FrameBuffer* buffer, vec4f color) {
	vec2f bbox_rt(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	vec2f bbox_lb(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec2f clamp(buffer->width - 1, buffer->height - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bbox_rt[j] = std::min(clamp[j], (std::max(bbox_rt[j], points[i][j])));
			bbox_lb[j] = std::max(0.f, (std::min(bbox_lb[j], points[i][j])));
		}
	}

	float inter_z;
	vec3f baricentric_coords;
	std::vector<vec3f>triangle_points;
	for (auto point : points) {
		triangle_points.push_back(proj<3>(point));
	}

	for (int i = bbox_lb.y; i <= bbox_rt.y; i++) {
		for (int j = bbox_lb.x; j <= bbox_rt.x; j++) {
			baricentric_coords = barycentric(vec3f(j + 0.5f, i + 0.5f, 1.f), triangle_points);
			if (baricentric_coords.x < 0 || baricentric_coords.y < 0 || baricentric_coords.z < 0) {
				continue;
			}

			// 深度插值纠正
			float recip_w[3], inter_z = 0, correct_depth;
			for (int k = 0; k < 3; k++) {
				recip_w[k] = baricentric_coords[k] / points[k].w;
				inter_z += recip_w[k];
			}
			correct_depth = (1 / inter_z) * (recip_w[0] * points[0].z + recip_w[1] * points[1].z + recip_w[2] * points[2].z);
			
			if (correct_depth < buffer->get_depth(j, i)) {
				buffer->set_depth(j, i, correct_depth);
				buffer->set_color(j, i, color);
			}
		}
	}
}

void draw_triangle_bbox_texture(Model& model, std::vector<vec4f> points, std::vector<vec2f> uvs, std::vector<vec3f> normals, vec3f light_dir, FrameBuffer* buffer) {
	vec2f bbox_rt(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	vec2f bbox_lb(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec2f clamp(buffer->width - 1, buffer->height - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bbox_rt[j] = std::min(clamp[j], (std::max(bbox_rt[j], points[i][j])));
			bbox_lb[j] = std::max(0.f, (std::min(bbox_lb[j], points[i][j])));
		}
	}

	float inter_z;
	vec3f baricentric_coords, normal;
	vec2f uv;
	std::vector<vec3f> triangle_points;
	for (auto point : points) {
		triangle_points.push_back(proj<3>(point));
	}

	for (int i = bbox_lb.y; i <= bbox_rt.y; i++) {
		for (int j = bbox_lb.x; j <= bbox_rt.x; j++) {
			baricentric_coords = barycentric(vec3f(j + 0.5f, i + 0.5f, 1.f), triangle_points);
			if (baricentric_coords.x < 0 || baricentric_coords.y < 0 || baricentric_coords.z < 0) {
				continue;
			}

			// 深度插值纠正
			float recip_w[3], inter_z = 0, correct_depth;
			for (int k = 0; k < 3; k++) {
				recip_w[k] = baricentric_coords[k] / points[k].w;
				inter_z += recip_w[k];
			}
			correct_depth = (1 / inter_z) * (recip_w[0] * points[0].z + recip_w[1] * points[1].z + recip_w[2] * points[2].z);
			// uv插值
			for (int k = 0; k < 2; k++) {
				uv[k] = (1 / inter_z) * (recip_w[0] * uvs[0][k] + recip_w[1] * uvs[1][k] + recip_w[2] * uvs[2][k]);
			}
			// 向量插值
			normal = (1 / inter_z) * (recip_w[0] * normals[0] + recip_w[1] * normals[1] + recip_w[2] * normals[2]);
			normal = normal.normalize();

			if (correct_depth < buffer->get_depth(j, i)) {
				buffer->set_depth(j, i, correct_depth);

				// 简单Blinn-Phone
				vec4f diffuse = ((vec4f)model.diffuse(uv)) / 255.f;
				diffuse = diffuse * std::max(0.f, dot(light_dir, normal));

				buffer->set_color(j, i, diffuse);
			}
		}
	}
}

// 根据world_pos和uvs计算切线
// 参考LearnOpenGL：https://learnopengl-cn.github.io/05%20Advanced%20Lighting/04%20Normal%20Mapping/
vec3f calculate_tangent(const std::vector<shader_data_v2f>& v2fs) {
	vec3f AB = v2fs[1].world_pos - v2fs[0].world_pos;
	vec3f AC = v2fs[2].world_pos - v2fs[0].world_pos;
	float delta_u1 = v2fs[1].uv.u - v2fs[0].uv.u,
		delta_v1 = v2fs[1].uv.v - v2fs[0].uv.v,
		delta_u2 = v2fs[2].uv.u - v2fs[0].uv.u,
		delta_v2 = v2fs[2].uv.v - v2fs[0].uv.v;
	
	float deno = (delta_v2 * delta_u1) - (delta_v1 * delta_u2);

	return ((delta_v2 * AB - delta_v1 * AC) / deno).normalize();
}

// 计算三角形向量（默认逆时针顶点）
vec3f calculate_triangle_normal(vec3f A_world_pos, vec3f B_world_pos, vec3f C_world_pos) {
	vec3f AB = B_world_pos - A_world_pos, AC = C_world_pos - A_world_pos;
	return (cross(AB, AC).normalize());
}

// 对vertex得到的顶点数据进行像素级别的插值
shader_data_v2f interpolate_barycentric(const float& z_inver, const float* recip_w, const std::vector<shader_data_v2f>& v2fs) {
	shader_data_v2f inter_v2f;
	
	// 三角重心插值
	inter_v2f.normal = z_inver * (recip_w[0] * v2fs[0].normal + recip_w[1] * v2fs[1].normal + recip_w[2] * v2fs[2].normal);
	inter_v2f.normal = inter_v2f.normal.normalize();
	inter_v2f.world_pos = z_inver * (recip_w[0] * v2fs[0].world_pos + recip_w[1] * v2fs[1].world_pos + recip_w[2] * v2fs[2].world_pos);
	inter_v2f.tangent = calculate_tangent(v2fs);
	inter_v2f.uv = z_inver * (recip_w[0] * v2fs[0].uv + recip_w[1] * v2fs[1].uv + recip_w[2] * v2fs[2].uv);
	inter_v2f.clip_pos = z_inver * (recip_w[0] * v2fs[0].clip_pos + recip_w[1] * v2fs[1].clip_pos + recip_w[2] * v2fs[2].clip_pos);
	inter_v2f.color = z_inver * (recip_w[0] * v2fs[0].color + recip_w[1] * v2fs[1].color + recip_w[2] * v2fs[2].color);

	return inter_v2f;
}

// 作为光栅化全流程的框架函数，输入处理过后的三角形数据，并通过调用传入的shader进行渲染，最终将颜色并写入buffer
// 包含接受v2fs的数据，背面剔除，进行重心插值，深度测试，颜色写入
void rasterize_triangle(const std::vector<shader_data_v2f>& v2fs, IShader& shader, FrameBuffer* buffer) {
	vec2f bbox_rt(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	vec2f bbox_lb(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec2f clamp(buffer->width - 1, buffer->height - 1);

	// 计算ndc和屏幕空间坐标（光栅化准备）
	vec4f ndc_coords[3], screen_coords[3];
	for (int i = 0; i < 3; i++) {
		vec4f cur_clip_pos = v2fs[i].clip_pos;
		ndc_coords[i] = cur_clip_pos / cur_clip_pos.w;
		screen_coords[i] = vec4f(viewport(proj<3>(ndc_coords[i]), buffer->width, buffer->height), cur_clip_pos.z);
	}

	// TODO：根据NDC空间中坐标进行背面剔除

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bbox_rt[j] = std::min(clamp[j], (std::max(bbox_rt[j], screen_coords[i][j])));
			bbox_lb[j] = std::max(0.f, (std::min(bbox_lb[j], screen_coords[i][j])));
		}
	}

	std::vector<vec3f> triangle_points;
	vec3f baricentric_coords;
	for (auto point : screen_coords) {
		triangle_points.push_back(proj<3>(point));
	}
	// bbox遍历三角形（光栅化）
	for (int i = bbox_lb.y; i <= bbox_rt.y; i++) {
		for (int j = bbox_lb.x; j <= bbox_rt.x; j++) {
			baricentric_coords = barycentric(vec3f(j + 0.5f, i + 0.5f, 1.f), triangle_points);
			if (baricentric_coords.x < 0 || baricentric_coords.y < 0 || baricentric_coords.z < 0) {
				continue;
			}

			shader_data_v2f inter_v2f;
			//TODO：这里应该改为对于struct所有字段进行插值（并完成修正插值函数），而不是在这里插值
			float recip_w[3], inter_z = 0, correct_depth;
			for (int k = 0; k < 3; k++) {
				recip_w[k] = baricentric_coords[k] / screen_coords[k].w;
				inter_z += recip_w[k];
			}
			inter_z = 1 / inter_z;
			correct_depth = inter_z * (recip_w[0] * screen_coords[0].z + recip_w[1] * screen_coords[1].z + recip_w[2] * screen_coords[2].z);
			inter_v2f = interpolate_barycentric(inter_z, recip_w, v2fs);

			// 深度测试
			if (correct_depth < buffer->get_depth(j, i)) {
				buffer->set_depth(j, i, correct_depth);

				// 调用fragment着色器进行对应像素的颜色计算
				vec4f color;
				if (shader.fragment_shader(inter_v2f, color)) {
					continue;
				}

				buffer->set_color(j, i, color);
			}
		}
	}
}

// 该函数中调用vertex处理顶点数据得到对应的v2f数据，并三个为一组进行光栅化（相当于图元组装）
// 注意：这里默认输入面只有三角形（不为三角形则按照顶点顺序拆分为三角形）
void draw_model(Model& model, IShader& shader, FrameBuffer* buffer) {
	int n_face = model.nfaces(), j_size;
	std::vector<int> indeies;

	// 对每个三角面调用vertext，并将数据传入进行光栅化
	for (int i = 0; i < n_face; i++) {
		indeies = model.face(i);
		j_size = indeies.size();

		// 将每个面顶点按照顺序进行三角划分
		for (int j = 0; j < j_size; j += 2) {
			std::vector<shader_data_v2f> v2fs(3);
			std::vector<shader_data_a2v> a2vs(3);
			int _index[3]{
				j, (j + 1) % j_size, (j + 2) % j_size
			};

			for (int k = 0; k < 3; k++) {
				a2vs[k].uv = model.uv(i, _index[k]);
				a2vs[k].normal = model.normal(i, _index[k]);
				a2vs[k].world_pos = model.vert(indeies[_index[k]]);
			}

			vec3f triangle_normal = calculate_triangle_normal(a2vs[0].world_pos, a2vs[1].world_pos, a2vs[2].world_pos);
			for (int k = 0; k < 3; k++) {
				a2vs[k].triangle_world_normal = triangle_normal;
				// 得到vertex处理后的顶点数据
				v2fs[k] = shader.vertex_shader(a2vs[k]);
			}

			// 绘制当前三角片面
			rasterize_triangle(v2fs, shader, buffer);
		}
	}
}

// 作为光栅化全流程的框架函数，输入处理过后的三角形数据，并通过调用传入的shader进行渲染，最终将颜色并写入buffer
// 包含接受v2fs的数据，背面剔除，进行重心插值，深度测试，颜色写入
void rasterize_triangle_shadow(const std::vector<shader_data_v2f>& v2fs, IShader& shader, FrameBuffer* buffer) {
	vec2f bbox_rt(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
	vec2f bbox_lb(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	vec2f clamp(buffer->width - 1, buffer->height - 1);

	// 计算ndc和屏幕空间坐标（光栅化准备）
	vec4f ndc_coords[3], screen_coords[3];
	for (int i = 0; i < 3; i++) {
		vec4f cur_clip_pos = v2fs[i].clip_pos;
		ndc_coords[i] = cur_clip_pos / cur_clip_pos.w;
		screen_coords[i] = vec4f(viewport(proj<3>(ndc_coords[i]), buffer->width, buffer->height), cur_clip_pos.z);
	}

	// TODO：这里可以根据NDC空间中坐标进行背面剔除

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bbox_rt[j] = std::min(clamp[j], (std::max(bbox_rt[j], screen_coords[i][j])));
			bbox_lb[j] = std::max(0.f, (std::min(bbox_lb[j], screen_coords[i][j])));
		}
	}

	std::vector<vec3f> triangle_points;
	vec3f baricentric_coords;
	for (auto point : screen_coords) {
		triangle_points.push_back(proj<3>(point));
	}
	// bbox遍历三角形（光栅化）
	for (int i = bbox_lb.y; i <= bbox_rt.y; i++) {
		for (int j = bbox_lb.x; j <= bbox_rt.x; j++) {
			baricentric_coords = barycentric(vec3f(j + 0.5f, i + 0.5f, 1.f), triangle_points);
			if (baricentric_coords.x < 0 || baricentric_coords.y < 0 || baricentric_coords.z < 0) {
				continue;
			}

			shader_data_v2f inter_v2f;
			// 只需要计算插值的深度即可
			float recip_w[3], inter_z = 0, correct_depth;
			for (int k = 0; k < 3; k++) {
				recip_w[k] = baricentric_coords[k] / screen_coords[k].w;
				inter_z += recip_w[k];
			}
			inter_z = 1 / inter_z;
			correct_depth = inter_z * (recip_w[0] * screen_coords[0].z + recip_w[1] * screen_coords[1].z + recip_w[2] * screen_coords[2].z);

			// 深度测试
			if (correct_depth < buffer->get_depth(j, i)) {
				buffer->set_depth(j, i, correct_depth);
				// 这里不需要计算任何颜色
			}
		}
	}
}

// 渲染shadow map
void draw_shadow_map(Model& model, ShadowShader& shader, FrameBuffer* shadow_buffer) {
	int n_face = model.nfaces(), j_size;
	std::vector<int> indeies;

	// 对每个三角面调用vertext，并将数据传入进行光栅化
	for (int i = 0; i < n_face; i++) {
		indeies = model.face(i);
		j_size = indeies.size();

		// 将每个面顶点按照顺序进行三角划分
		for (int j = 0; j < j_size; j += 2) {
			std::vector<shader_data_v2f> v2fs(3);
			std::vector<shader_data_a2v> a2vs(3);
			int _index[3]{
				j, (j + 1) % j_size, (j + 2) % j_size
			};

			for (int k = 0; k < 3; k++) {
				a2vs[k].uv = model.uv(i, _index[k]);
				a2vs[k].normal = model.normal(i, _index[k]);
				a2vs[k].world_pos = model.vert(indeies[_index[k]]);
			}

			for (int k = 0; k < 3; k++) {
				// 得到vertex处理后的顶点数据
				v2fs[k] = shader.vertex_shader(a2vs[k]);
			}

			// 对当前三角片面进行深度测试
			rasterize_triangle_shadow(v2fs, shader, shadow_buffer);
		}
	}
}