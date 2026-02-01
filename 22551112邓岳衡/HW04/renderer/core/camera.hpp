#pragma once
#include <cassert>
#include "maths.hpp"
#include "../win32/win32.hpp"

struct Motion { vec2f orbit; vec2f pan; float dolly; };

struct Record {
	/* orbit */
	int is_orbiting;
	vec2f orbit_pos;
	vec2f orbit_delta;
	/* pan */
	int is_panning;
	vec2f pan_pos;
	vec2f pan_delta;
	/* zoom */
	float dolly_delta;
	/* light */
	float light_theta;
	float light_phi;
	/* click */
	float press_time;
	float release_time;
	vec2f press_pos;
	vec2f release_pos;
	int single_click;
	int double_click;
	vec2f click_pos;
	float window_width;
	float window_height;
	Record() {
		is_orbiting = 0;
		is_panning = 0;
		orbit_delta = vec2f(0, 0);
		pan_delta = vec2f(0, 0);
		dolly_delta = 0;
		single_click = 0;
		double_click = 0;
	}
};

class Camera {
public:
	enum class Projection { Orthographics, Persperctive };
private:
	static const vec3f DEFAULT_POS, DEFAULT_TARGET, DEFAULT_UP;
	Projection type;
	vec3f up_direction;
	vec3f position;
	vec3f target;
	float near_plane, far_plane;
	float fov, aspect;

	static vec2f get_cursor_pos(window_t* window);
	static vec2f get_pos_delta(vec2f old_pos, vec2f new_pos, float windnow_height);
	vec3f calculate_pan(vec3f from_camera, Motion motion);
	vec3f calculate_offset(vec3f from_target, Motion motion);

public:
	Camera(const vec3f _position = DEFAULT_POS, const vec3f _target = DEFAULT_TARGET, const vec3f& _up_direction = DEFAULT_UP, 
		float _near_plane = 0.1f, float _far_plane = 2000.f, float _fov = to_radians(90.f), float _aspect = 1, Projection _type = Projection::Persperctive)
		: position(_position), target(_target), up_direction(_up_direction), 
		near_plane(_near_plane), far_plane(_far_plane), fov(_fov), aspect(_aspect), type(_type) {
		assert((_position - _target).norm() > 2e-2);
	}
	~Camera() {}

	vec3f get_position() const;
	vec3f get_target() const;
	vec3f get_toward() const;
	vec3f get_up_direction() const;

	mat4f get_view_matrix() const;
	mat4f get_perspective_matrix() const;

	void set_transform(vec3f new_position, vec3f new_target);
	void update_transform(Motion motion);

	static void scroll_callback(window_t* window, float offset);
	static void button_callback(window_t* window, button_t button, int pressed);
	static void update_camera(window_t* window, Camera* camera, Record* record);
};

