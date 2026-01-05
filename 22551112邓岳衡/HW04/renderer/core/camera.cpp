#include "camera.hpp"
#include "graphics.hpp"
#include <iostream>

const vec3f Camera::DEFAULT_POS = vec3f(0.f, 0.f, 1.3f);
const vec3f Camera::DEFAULT_TARGET = vec3f(0.f, 0.f, 0.f);
const vec3f Camera::DEFAULT_UP = vec3f(0.f, 1.f, 0.f);
static const float CLICK_DELAY = 0.25f;

vec3f Camera::get_position() const {
	return position;
}
vec3f Camera::get_target() const {
	return target;
}
vec3f Camera::get_toward() const {
	return (target - position).normalize();
}
vec3f Camera::get_up_direction() const {
	return up_direction;
}

mat4f Camera::get_view_matrix() const {
	return lookAt(position, target, up_direction);
}

mat4f Camera::get_perspective_matrix() const {
	// TODO：这里需要修改正交情况下的right和top为buffer->width和buffer->height
	if (type == Projection::Orthographics) {
		return ortho(fov, aspect, near_plane, far_plane);
	}
	else if (type == Projection::Persperctive) {
		return perspective(fov, aspect, near_plane, far_plane);
	}
	else {
		// 默认返回透视矩阵
		return perspective(fov, aspect, near_plane, far_plane);
	}
}

void Camera::set_transform(vec3f new_position, vec3f new_target) {
	assert((new_position - new_target).norm() > 2e-2);
	position = new_position;
	target = new_target;
}

// 计算平面平移量
vec3f Camera::calculate_pan(vec3f from_camera, Motion motion) {
	vec3f forward = from_camera.normalize();
	vec3f left = -cross(forward, up_direction);
	vec3f up = cross(-left, forward);

	// TODO：这里的right和pan.x方向是否对应？
	float distance = from_camera.norm();
	float factor = distance * (float)tan(fov / 2) * 2;
	vec3f delta_x = left * motion.pan.x * factor;
	vec3f delta_y = up * motion.pan.y * factor;
	return delta_x + delta_y;
}
// 计算球面旋转量（基于物体的极坐标）
vec3f Camera::calculate_offset(vec3f from_target, Motion motion) {
	float radius = from_target.norm();
	float theta = (float)atan2(from_target.x, from_target.z);
	float phi = (float)acos(from_target.y / radius);
	float factor = PI * 2;
	vec3f offset;

	// TODO：不太明白这里radius的调整
	radius *= (float)pow(0.95, motion.dolly);
	theta -= motion.orbit.x * factor;
	phi -= motion.orbit.y * factor;
	phi = clamp(phi, 0.f, (float)PI);

	offset.x = radius * sin(phi) * sin(theta);
	offset.z = radius * sin(phi) * cos(theta);
	offset.y = radius * cos(phi);

	return offset;
}

void Camera::update_transform(Motion motion) {
	vec3f from_camera = target - position;
	vec3f from_target = position - target;
	vec3f pan = calculate_pan(from_camera, motion);
	vec3f offset = calculate_offset(from_target, motion);

	target = target + pan;
	position = target + offset;
	//std::cout << "offset is " << offset.x << " " << offset.y << " " << offset.z << std::endl;
	//std::cout << "Camera Position has been updated!" << std::endl;
	//std::cout << "new position is" << position << " and new target is " << target << std::endl;
}

// TODO：缩放一定比例的位置差？
vec2f Camera::get_pos_delta(vec2f old_pos, vec2f new_pos, float window_height) {
	vec2f delta = new_pos - old_pos;
	return delta / window_height;
}

// 鼠标坐标查询（以窗口左上角作为原点？）
vec2f Camera::get_cursor_pos(window_t* window) {
	float xpos, ypos;
	input_query_cursor(window, &xpos, &ypos);
	return vec2f(xpos, ypos);
}

// 记录滚轮的offset
void Camera::scroll_callback(window_t* window, float offset) {
	Record* record = (Record*)window_get_userdata(window);
	record->dolly_delta += offset;
}

// TODO：待注释
void Camera::button_callback(window_t* window, button_t button, int pressed) {
	Record* record = (Record*)window_get_userdata(window);
	vec2f cursor_pos = get_cursor_pos(window);
	if (button == BUTTON_L) {
		float curr_time = platform_get_time();
		if (pressed) {
			record->is_orbiting = 1;
			record->orbit_pos = cursor_pos;
			record->press_time = curr_time;
			record->press_pos = cursor_pos;
		}
		else {
			float prev_time = record->release_time;
			vec2f pos_delta = get_pos_delta(record->orbit_pos, cursor_pos, record->window_height);
			record->is_orbiting = 0;
			record->orbit_delta = record->orbit_delta + pos_delta;
			if (prev_time && curr_time - prev_time < CLICK_DELAY) {
				record->double_click = 1;
				record->release_time = 0;
			}
			else {
				record->release_time = curr_time;
				record->release_pos = cursor_pos;
			}
		}
	}
	else if (button == BUTTON_R) {
		if (pressed) {
			record->is_panning = 1;
			record->pan_pos = cursor_pos;
		}
		else {
			vec2f pos_delta = get_pos_delta(record->pan_pos, cursor_pos, record->window_height);
			record->is_panning = 0;
			record->pan_delta = record->pan_delta + pos_delta;
		}
	}
}

// TODO：待注释
void Camera::update_camera(window_t* window, Camera* camera, Record* record) {
	vec2f cursor_pos = get_cursor_pos(window);
	if (record->is_orbiting) {
		vec2f pos_delta = get_pos_delta(record->orbit_pos, cursor_pos, record->window_height);
		record->orbit_delta = record->orbit_delta + pos_delta;
		record->orbit_pos = cursor_pos;
	}
	if (record->is_panning) {
		vec2f pos_delta = get_pos_delta(record->pan_pos, cursor_pos, record->window_height);
		record->pan_delta = record->pan_delta + pos_delta;
		record->pan_pos = cursor_pos;
	}
	if (input_key_pressed(window, KEY_SPACE)) {
		camera->set_transform(DEFAULT_POS, DEFAULT_TARGET);
	}
	else {
		Motion motion;
		motion.orbit = record->orbit_delta;
		motion.pan = record->pan_delta;
		motion.dolly = record->dolly_delta;
		camera->update_transform(motion);
	}
}


