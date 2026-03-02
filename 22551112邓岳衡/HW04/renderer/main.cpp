#include "core/graphics.hpp"
#include "iostream"
#include "win32/win32.hpp"
#include "core/model.hpp"
#include "test/test.hpp"

static const char* const WINDOW_TITLE = "raymond's Renderer";
static const int WINDOW_WIDTH = 800;
static const int WINDOW_HEIGHT = 800;
static const int WINDOW_TEXT_WIDTH = 250;
static const int WINDOW_TEXT_HEIGHT = 310;
static const int SHADOW_MAP_WIDTH = 1024;
static const int SHADOW_MAP_HEIGHT = 1024;

static const char* const ASSETS_PATH = "../../../assets";

// ����ģ��
auto model = std::make_unique<Model>((std::string(ASSETS_PATH) + std::string("/obj/african_head.obj")).c_str());
// ��ɫ�������Ӱ��ͼ
auto frame_buffer = std::make_unique<FrameBuffer>(WINDOW_WIDTH, WINDOW_HEIGHT);
auto shadow_map_buffer = std::make_unique<FrameBuffer>(SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT);
// ������Shaders
BlinnPhoneShader blinn_phone_shader;
NormalMapShader normal_map_shader;
PhoneShader phone_shader;
FlatShader flat_shader;
GouraudShader gouraud_shader;
std::vector<IShader*> shaders{
	&blinn_phone_shader,
	&normal_map_shader,
	&phone_shader,
	&flat_shader,
	&gouraud_shader
};
size_t current_shader_index = 0;
IShader* current_shader = shaders[current_shader_index];

// ��������������Դ
Camera camera(vec3f(0.8f, 0.5f, 1.3f), vec3f(0.f, 0.f, 0.f), vec3f(0.f, 1.f, 0.f), 0.1, 2000.f);
Camera light(vec3f(-3.f, -3.f, 3.f), vec3f(0.f, 0.f, 0.f), vec3f(0.f, 1.f, 0.f), 0.1f, 2000.f, 3.f, 3.f, Camera::Projection::Orthographics);
float light_angle_speed = 0.05, light_y_radience = 0;

// ���̼����Ļص�����
void key_callback(window_t* window, keycode_t key, int pressed) {
	if (pressed) {
		switch (key) {
		//������Ӱ
		case KEY_Q: {
			bool status = current_shader->shader_data->shadow_enable;
			std::cout << std::string("Shadow has been switched to��") + (status ? "Off" : "On") << std::endl;
			current_shader->shader_data->shadow_enable = !status;
			break;
		}
		// TODO����AD������xzƽ������ת��Դ
		case KEY_A: {
			vec3f light_pos = light.get_position();
			if (!light_y_radience) {
				std::cout << "initialize light radience to z_axis!" << std::endl;
				light_y_radience = vec2f(light_pos.x, light_pos.z).norm();
			}
			float current_angle = atan2(light_pos.z, light_pos.x);
			current_angle += light_angle_speed;
			vec3f new_pos{ light_y_radience * cos(current_angle), light_pos.y, light_y_radience * sin(current_angle)};
			light.set_transform(new_pos, light.get_target());

			break;
		}
		// ��E�����л�Shader
		case KEY_E: {
			current_shader_index += 1;
			current_shader_index %= shaders.size();
			auto shader_data = current_shader->shader_data;
			current_shader = shaders[current_shader_index];
			if (!current_shader->shader_data) {
				current_shader->shader_data = shader_data;
			}
			std::cout << "Shader has been changed!" << std::endl;
			break;
		}
		default:
			break;
		}
	}
}


int main()
{
	platform_initialize();
	window_t* window;
	Record record;
	callbacks_t callbacks = callbacks_t();
	float prev_time = platform_get_time();

	float print_time = prev_time;
	int num_frames = 0;
	int show_num_frames = 0;
	int show_avg_millis = 0;

	window = window_create(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TEXT_WIDTH, WINDOW_TEXT_HEIGHT);

	callbacks.key_callback = key_callback;
	callbacks.scroll_callback = Camera::scroll_callback;
	callbacks.button_callback = Camera::button_callback;

	record.window_height = WINDOW_HEIGHT;
	record.window_width = WINDOW_WIDTH;
	window_set_userdata(window, &record);
	input_set_callbacks(window, callbacks);

	// ��Ⱦ��ɫ����ʼ��
	auto material = std::make_unique<Material>();
	material->diffuse_map = model->get_diffuse_map();
	material->normal_map = model->get_normal_map();
	material->specular_map = model->get_specular_map();
	auto shader_data = std::make_unique<ShaderData>();
	shader_data->material = material.get();
	shader_data->buffer = frame_buffer.get();
	shader_data->ambient_strength = 0.1f;
	shader_data->light_color = vec4f(1.f, 1.f, 1.f, 1.f);
	shader_data->light_dir = camera.get_toward();
	shader_data->model_matrix = mat4f::identity();
	shader_data->model_matrix_invers = mat4f::identity();
	shader_data->projective_matrix = camera.get_perspective_matrix();
	shader_data->view_matrix = camera.get_view_matrix();
	shader_data->camera_vp_matrix = camera.get_perspective_matrix() * camera.get_view_matrix();

	current_shader->shader_data = shader_data.get();
	// ������Ӱ����Ӱ��ɫ����ʼ����
	current_shader->shader_data->shadow_enable = false;
	current_shader->shader_data->shadow_bias = 2e-4;
	current_shader->shader_data->shadow_map = shadow_map_buffer.get();
	ShadowShader shadow_shader;
	auto shadow_shader_data = std::make_unique<ShaderData>();
	shadow_shader.shader_data = shadow_shader_data.get();
	shadow_shader_data->buffer = shadow_map_buffer.get();
	shadow_shader_data->projective_matrix = light.get_perspective_matrix();
	shadow_shader_data->view_matrix = light.get_view_matrix();
	shadow_shader_data->camera_vp_matrix = light.get_perspective_matrix() * light.get_view_matrix();

	while (!window_should_close(window)) {
		float curr_time = platform_get_time();
		float delta_time = curr_time - prev_time;

		// �������
		Camera::update_camera(window, &camera, &record);

		// ���Ժ���
		// test_model_triangle(*model, frame_buffer.get());
		// test_barycentic_with_triangle(frame_buffer.get());
		// test_model_triangle_with_camera(*model, camera, frame_buffer.get());
		test_myShadingPipeLine(*model, camera, light, *current_shader, shadow_shader, frame_buffer.get());

		std::cout << "One frame is Done!" << std::endl;

		// ����֡�ʺͺ�ʱ
		num_frames += 1;
		if (curr_time - print_time >= 1) {
			int sum_millis = (int)((curr_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;

			show_num_frames = num_frames;
			show_avg_millis = avg_millis;
			num_frames = 0;
			print_time = curr_time;
		}
		prev_time = curr_time;

		// ��֡������Ƶ�UI����
		window_draw_buffer(window, frame_buffer.get());

		// ����������������
		record.orbit_delta = vec2f(0, 0);
		record.pan_delta = vec2f(0, 0);
		record.dolly_delta = 0;
		record.single_click = 0;
		record.double_click = 0;

		// �����ɫ�������Ȼ���
		frame_buffer->framebuffer_clear_color(Color::Black);
		frame_buffer->framebuffer_clear_depth(1);

		input_poll_events();
	}

	window_destroy(window);
}
