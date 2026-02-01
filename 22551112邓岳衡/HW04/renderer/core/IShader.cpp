#include "IShader.hpp"
#include <iostream>

// Flat着色器
shader_data_v2f FlatShader::vertex_shader(shader_data_a2v& data) {
	shader_data_v2f ret;

	ret.world_pos = data.world_pos;
	ret.normal = data.triangle_world_normal;
	ret.uv = data.uv;
	ret.clip_pos = shader_data->camera_vp_matrix * vec4f(data.world_pos, 1.f);
	// 使用三角片面法线进行颜色计算
	float intensity = std::max(0.f, dot(-shader_data->light_dir, ret.normal));
	ret.color = shader_data->light_color * intensity;
	ret.color.a = 1.f;

	return ret;
}
bool FlatShader::fragment_shader(shader_data_v2f& data, vec4f& color) {
	color = data.color;
	return false;
}

// 基本Gouraud着色器
shader_data_v2f GouraudShader::vertex_shader(shader_data_a2v& data) {
	shader_data_v2f ret;

	ret.world_pos = data.world_pos;
	ret.normal = data.normal.normalize();
	ret.uv = data.uv;
	ret.clip_pos = shader_data->camera_vp_matrix * vec4f(data.world_pos, 1.f);
	
	float intensity = std::max(0.f, dot(-shader_data->light_dir, ret.normal));
	ret.color = shader_data->light_color * intensity;
	ret.color.a = 1.f;

	return ret;
}
bool GouraudShader::fragment_shader(shader_data_v2f& data, vec4f& color) {
	// 直接取插值颜色
	color = data.color;
	return false;
}

// 基本Phone着色器
shader_data_v2f PhoneShader::vertex_shader(shader_data_a2v& data) {
	shader_data_v2f ret;

	ret.world_pos = data.world_pos;
	ret.normal = data.normal.normalize();
	ret.uv = data.uv;
	ret.clip_pos = shader_data->camera_vp_matrix * vec4f(data.world_pos, 1.f);

	return ret;
}
bool PhoneShader::fragment_shader(shader_data_v2f& data, vec4f& color) {
	float intensity = std::max(0.f, dot(-shader_data->light_dir, data.normal));

	if (shader_data->shadow_enable) {
		float shadow_factor = calculate_shadow_factor(data.world_pos, intensity);
		shadow_factor = 1.f - shadow_factor;
		intensity *= shadow_factor;
	}

	color = shader_data->light_color * intensity;
	color.a = 1.f;
	
	return false;
}

// 经典BlinnPhone着色器
shader_data_v2f BlinnPhoneShader::vertex_shader(shader_data_a2v& data) {
	shader_data_v2f ret;

	ret.world_pos = data.world_pos;
	ret.normal = data.normal.normalize();
	ret.uv = data.uv;
	ret.clip_pos = shader_data->camera_vp_matrix * vec4f(data.world_pos, 1.f);

	return ret;
}
bool BlinnPhoneShader::fragment_shader(shader_data_v2f& data, vec4f& color) {
	// ambient
	vec4f ambient = shader_data->ambient_strength * shader_data->light_color;
	ambient.a = 1.f;

	// 这里简单使用Model矩阵进行向量变换（不考虑非对称scale）
	vec3f normal = proj<3>(shader_data->model_matrix * vec4f(data.normal, 1.f)).normalize();
	//// 使用TBN计算normal_map中法线
	//vec3f bitangent = cross(data.normal, data.tangent).normalize();
	//vec3f tangent = cross(bitangent, normal).normalize();
	//mat3f TBN;
	//TBN.setCol(0, tangent);
	//TBN.setCol(1, bitangent);
	//TBN.setCol(2, normal);
	// normal = (TBN * texture_normal(data.uv)).normalize();

	// 本次的法线贴图为直接读取，因此不需要计算TBN
	normal = texture_normal(data.uv);

	// diffuse
	float diff = std::max(0.f, dot(-shader_data->light_dir, normal));
	vec4f diffuse = diff * shader_data->light_color;
	diffuse.a = 1.f;

	// specular
	vec3f view_dir = (shader_data->view_pos - data.world_pos).normalize();
	vec3f half = (view_dir - shader_data->light_dir).normalize();
	float spec_factor = texture_specular(data.uv);
	float spec = std::pow(std::max(0.f, dot(half, normal)), spec_factor);
	vec4f specular = spec * shader_data->light_color;
	specular.a = 1.f;

	if (shader_data->shadow_enable) {
		float shadow_factor = calculate_shadow_factor(data.world_pos, diff);
		//if (shadow_factor) {
		//	std::cerr << "shadow factor is not ZERO!" << std::endl;
		//}
		shadow_factor = 1.f - shadow_factor;
		specular = shadow_factor * specular;
		diffuse = shadow_factor * diffuse;
	}

	color = (ambient + specular + diffuse) * texture_diffuse(data.uv);
	color = clamp(0.f, 1.f, color);

	return false;
}

shader_data_v2f NormalMapShader::vertex_shader(shader_data_a2v& data) {
	shader_data_v2f ret;

	ret.world_pos = data.world_pos;
	ret.normal = data.normal.normalize();
	ret.uv = data.uv;
	ret.clip_pos = shader_data->camera_vp_matrix * vec4f(data.world_pos, 1.f);

	return ret;
}

bool NormalMapShader::fragment_shader(shader_data_v2f& data, vec4f& color) {
	vec3f normal = proj<3>(shader_data->model_matrix * vec4f(data.normal, 1.f)).normalize();
	// 使用TBN计算normal_map中法线
	vec3f bitangent = cross(data.normal, data.tangent).normalize();
	// 这里省略了切线的局部坐标 -> 世界坐标的转换
	// bitangent = proj<3>(shader_data->model_matrix * vec4f(bitangent, 1.f)).normalize();
	//vec3f tangent = cross(bitangent, normal).normalize();
	//mat3f TBN;
	//TBN.setCol(0, tangent);
	//TBN.setCol(1, bitangent);
	//TBN.setCol(2, normal);
	//normal = (TBN * texture_normal(data.uv)).normalize();

	normal = texture_normal(data.uv);

	color = vec4f(normal, 1.f);
	return false;
}

shader_data_v2f ShadowShader::vertex_shader(shader_data_a2v& data) {
	shader_data_v2f ret;
	ret.world_pos = data.world_pos;
	ret.clip_pos = shader_data->camera_vp_matrix * vec4f(data.world_pos, 1.f);

	return ret;
}
bool ShadowShader::fragment_shader(shader_data_v2f& data, vec4f& color) {
	return false;
}