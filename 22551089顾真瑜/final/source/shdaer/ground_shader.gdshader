shader_type spatial; 

uniform sampler2D mask_texture;
uniform sampler2D snow_albedo;
uniform sampler2D snow_normal;
uniform sampler2D snow_roughness; 
uniform sampler2D dirt_albedo; 
uniform sampler2D dirt_normal; 
uniform sampler2D dirt_roughness; 
uniform float uv_scale = 1.0; 
uniform float snow_height = 1.0;

float overlay(float a, float b) {
	if(a < 0.5) {
		return a * b * 2.0;
	} else {
		return 1.0 - 2.0 * (1.0 - a) * (1.0 - b);
	}
}

 
void fragment( ){
	vec2 uv = UV * uv_scale;
	vec3 snow_a = texture(snow_albedo, uv).rgb;
	vec3 snow_n = texture(snow_normal, uv).rgb;
	float snow_r = texture(snow_roughness, uv).r;
	vec3 dirt_a = texture(dirt_albedo, uv).rgb;
	vec3 dirt_n = texture(dirt_normal, uv).rgb;
	float dirt_r = texture(dirt_roughness, uv).r;
	float trail = texture(mask_texture, uv).r;
	
	float snow_mask = COLOR.r * (trail + 0.1);
	snow_mask = clamp(snow_mask, 0.0, 1.0);
	
	ALBEDO = mix(dirt_a, snow_a, snow_mask);
	NORMAL_MAP = mix(dirt_n, snow_n, snow_mask);
	ROUGHNESS = mix(dirt_r, snow_r, snow_mask);
}
	
void vertex(){
	float snow_mask = COLOR.r;
	vec2 uv = UV * uv_scale;
	float trail = texture(mask_texture, uv).r;
	
	VERTEX.y += snow_mask * snow_height * trail;
}