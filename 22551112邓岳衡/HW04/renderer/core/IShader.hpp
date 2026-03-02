#pragma once
#include "framebuffer.hpp"
#include "tgaimage.hpp"

// 前向声明
vec3f viewport(vec3f ndc_coord, int width, int height);

struct Material {
	TGAImage* diffuse_map;
	TGAImage* normal_map;
	TGAImage* specular_map;
	vec4f color;			// 不使用贴图时的基础颜色
	vec4f specular;			// 反射光颜色
	Material(TGAImage* _diffuse_map = nullptr, TGAImage* _normal_map = nullptr, 
		TGAImage* _specular_map = nullptr, vec4f _color = vec4f(0.f, 0.f, 0.f, 1.f), vec4f _specular = vec4f(10.f,10.f,10.f,1.f)) :
		diffuse_map(_diffuse_map), normal_map(_normal_map), specular_map(_specular_map), color(_color), specular(_specular) {}
};

// 所有可能用到的渲染数据（需要在传入draw函数前进行初始化）
struct ShaderData {
	Material* material;
	FrameBuffer* buffer;
	FrameBuffer* shadow_map;
	bool shadow_enable;
	float shadow_bias;

	bool isculling;				// backface culling

	float ambient_strength;
	vec3f light_pos;			// world space
	vec3f light_dir;			// world space
	vec4f light_color;
	vec3f view_pos;				// world space

	mat4f model_matrix;
	mat4f model_matrix_invers;
	mat4f view_matrix;
	mat4f projective_matrix;
	mat4f light_view_matrix;
	mat4f camera_vp_matrix;		// perspective * view
};

// vertext着色器输入
struct shader_data_a2v {
	vec3f world_pos;
	vec3f normal;
	vec3f triangle_world_normal;
	vec2f uv;
};

// vertext着色器传入fragement着色器数据
struct shader_data_v2f {
	vec4f clip_pos;
	vec3f world_pos;
	vec3f normal;
	vec2f uv;
	vec4f color;		// 用于flat shading
	vec3f tangent;		// 切线：用于TBN计算
};

class IShader {
public:
	ShaderData* shader_data;
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) = 0;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) = 0;

	vec4f texture_diffuse(const vec2f& uvf) {
		TGAImage* diffuse = shader_data->material->diffuse_map;
		vec2i uv(uvf[0] * diffuse->get_width(), uvf[1] * diffuse->get_height());
		return vec4f(diffuse->get(uv[0], uv[1])) / 255.f;
	}

	vec3f texture_normal(const vec2f& uvf) {
		TGAImage* normal = shader_data->material->normal_map;
		vec2i uv(uvf[0] * normal->get_width(), uvf[1] * normal->get_height());
		TGAColor c = normal->get(uv[0], uv[1]);
		vec3f res;
		for (int i = 0; i < 3; i++)
			res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f;
		return res.normalize();
	}

	float texture_specular(const vec2f& uvf) {
		TGAImage* specular = shader_data->material->specular_map;
		vec2i uv(uvf[0] * specular->get_width(), uvf[1] * specular->get_height());
		return specular->get(uv[0], uv[1])[0] / 1.f;
	}

	// 在光相机空间中计算，使用PCF计算阴影因子
	// 参考LearnOpenGL：https://learnopengl-cn.github.io/05%20Advanced%20Lighting/03%20Shadows/01%20Shadow%20Mapping/#pcf
	float calculate_shadow_factor(vec3f world_pos, float normal_dot_lightDir) {
		// 计算逛光空间坐标
		vec4f light_clip_pos = shader_data->light_view_matrix * vec4f(world_pos, 1.f);
		vec3f light_screen_pos = viewport(proj<3>(light_clip_pos / light_clip_pos.w), shader_data->shadow_map->width, shader_data->shadow_map->height);
		vec2f shadow_map_uv(light_screen_pos.x, light_screen_pos.y);
		float target_depth = light_screen_pos.z, current_depth, bias = shader_data->shadow_bias * (1.f - normal_dot_lightDir);
		float shadow_factor = 0;

		// 使用双线性插值采样
		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				current_depth = shader_data->shadow_map->get_depth(int(shadow_map_uv.u + i), int(shadow_map_uv.v + j));
				shadow_factor += ((current_depth + bias < target_depth) ? 1.f : 0.f);
			}
		}

		shadow_factor /= 9.f;
		return shadow_factor;
	}
};

class FlatShader : public IShader {
public:
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) override;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) override;
};

class GouraudShader : public IShader {
public:
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) override;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) override;
};

class PhoneShader : public IShader {
public:
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) override;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) override;
};

class BlinnPhoneShader : public IShader {
public:
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) override;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) override;
};

class NormalMapShader : public IShader {
public:
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) override;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) override;
};

class ShadowShader : public IShader {
public:
	virtual shader_data_v2f vertex_shader(shader_data_a2v& data) override;
	virtual bool fragment_shader(shader_data_v2f& data, vec4f& color) override;
};