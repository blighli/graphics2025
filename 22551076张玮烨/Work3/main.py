import glfw
from OpenGL.GL import *
import OpenGL.GL.shaders
import numpy as np
import trimesh
from pyrr import Matrix44
import math

# -------- 窗口和交互变量 --------
WINDOW_WIDTH = 1600
WINDOW_HEIGHT = 900

rotation_y = 0.0
rotation_x = 0.0
translation = np.array([0.0, 0.0, 0.0], dtype=np.float32)
scale = 1.0

left_pressed = False
right_pressed = False
last_x, last_y = 0, 0

light_colors_list = [
    [np.array([1,1,1], dtype=np.float32), np.array([1,0,0], dtype=np.float32)],
    [np.array([0,1,0], dtype=np.float32), np.array([0,0,1], dtype=np.float32)],
    [np.array([1,1,0], dtype=np.float32), np.array([1,0,1], dtype=np.float32)]
]
light_color_index = 0

# 摄像机
camera_pos = np.array([0, 1, 4], dtype=np.float32)
camera_front = np.array([0,0,-1], dtype=np.float32)
camera_up = np.array([0,1,0], dtype=np.float32)
camera_speed = 0.03  # 调慢速度

# -------- GLFW 回调函数 --------
def scroll_callback(window, xoffset, yoffset):
    global scale
    scale *= 1.0 + 0.1*yoffset
    scale = max(0.01, min(scale, 10.0))

def mouse_button_callback(window, button, action, mods):
    global left_pressed, right_pressed, last_x, last_y, light_color_index
    if button == glfw.MOUSE_BUTTON_LEFT:
        left_pressed = action == glfw.PRESS
    elif button == glfw.MOUSE_BUTTON_RIGHT:
        right_pressed = action == glfw.PRESS
    elif button == glfw.MOUSE_BUTTON_MIDDLE and action == glfw.PRESS:
        light_color_index = (light_color_index + 1) % len(light_colors_list)
    last_x, last_y = glfw.get_cursor_pos(window)

def cursor_pos_callback(window, xpos, ypos):
    global rotation_x, rotation_y, translation, last_x, last_y
    dx = xpos - last_x
    dy = ypos - last_y
    if left_pressed:
        translation[0] += dx * 0.005
        translation[1] -= dy * 0.005
    if right_pressed:
        rotation_y += dx * 0.5
        rotation_x += dy * 0.5
    last_x, last_y = xpos, ypos

def process_input(window):
    global camera_pos, camera_front, camera_up, camera_speed
    if glfw.get_key(window, glfw.KEY_W) == glfw.PRESS:
        camera_pos += camera_front * camera_speed
    if glfw.get_key(window, glfw.KEY_S) == glfw.PRESS:
        camera_pos -= camera_front * camera_speed
    right = np.cross(camera_front, camera_up)
    right = right / np.linalg.norm(right)
    if glfw.get_key(window, glfw.KEY_D) == glfw.PRESS:
        camera_pos += right * camera_speed
    if glfw.get_key(window, glfw.KEY_A) == glfw.PRESS:
        camera_pos -= right * camera_speed

# -------- 初始化 GLFW --------
if not glfw.init():
    raise Exception("GLFW init failed")

window = glfw.create_window(
    WINDOW_WIDTH, WINDOW_HEIGHT, 
    "225510776-张玮烨-3D模型展示 - Scroll: Zoom | Left Drag: Pan | Right Drag: Rotate | Middle Click: Switch Light | WASD: Move",
    None, None
)
glfw.make_context_current(window)
glfw.set_scroll_callback(window, scroll_callback)
glfw.set_mouse_button_callback(window, mouse_button_callback)
glfw.set_cursor_pos_callback(window, cursor_pos_callback)
glEnable(GL_DEPTH_TEST)

# -------- Shader --------
vertex_src = """#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 FragPos;
out vec3 Normal;
void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}"""

fragment_src = """#version 330 core
struct PointLight { vec3 position; vec3 color; };
#define NUM_LIGHTS 2
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;
uniform vec3 viewPos;
uniform PointLight lights[NUM_LIGHTS];
void main()
{
    vec3 ambient = vec3(0.1);
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    for(int i = 0; i < NUM_LIGHTS; ++i)
    {
        vec3 lightDir = normalize(lights[i].position - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lights[i].color;
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * lights[i].color;
        result += diffuse + specular;
    }
    result += ambient;
    FragColor = vec4(result, 1.0);
}"""

shader = OpenGL.GL.shaders.compileProgram(
    OpenGL.GL.shaders.compileShader(vertex_src, GL_VERTEX_SHADER),
    OpenGL.GL.shaders.compileShader(fragment_src, GL_FRAGMENT_SHADER)
)

# -------- 读取模型 --------
scene = trimesh.load(r"models\LibertyStatue\LibertStatue.obj")
if isinstance(scene, trimesh.Scene):
    mesh = trimesh.util.concatenate([g for g in scene.geometry.values()])
else:
    mesh = scene

vertices = mesh.vertices.view(np.ndarray).astype(np.float32)
normals = mesh.vertex_normals.astype(np.float32)
indices = mesh.faces.flatten().astype(np.uint32)

# 模型缩放并居中到窗口正中心
center = mesh.bounding_box.centroid
size = mesh.bounding_box.extents
scale_factor = 2.0 / max(size)
translation = -center * scale_factor  # 居中，不偏下

# -------- VAO / VBO / EBO --------
VAO = glGenVertexArrays(1)
VBO = glGenBuffers(1)
NBO = glGenBuffers(1)
EBO = glGenBuffers(1)

glBindVertexArray(VAO)
glBindBuffer(GL_ARRAY_BUFFER, VBO)
glBufferData(GL_ARRAY_BUFFER, vertices.nbytes, vertices, GL_STATIC_DRAW)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, None)
glEnableVertexAttribArray(0)

glBindBuffer(GL_ARRAY_BUFFER, NBO)
glBufferData(GL_ARRAY_BUFFER, normals.nbytes, normals, GL_STATIC_DRAW)
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, None)
glEnableVertexAttribArray(1)

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO)
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.nbytes, indices, GL_STATIC_DRAW)
glBindVertexArray(0)

# 光源位置
light_positions = [np.array([2, 4, 3], dtype=np.float32), np.array([-2, 4, 3], dtype=np.float32)]

# -------- 主循环 --------
while not glfw.window_should_close(window):
    glfw.poll_events()
    process_input(window)

    glClearColor(0.2, 0.3, 0.3, 1.0)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    glUseProgram(shader)
    model = Matrix44.from_translation(translation) @ \
            Matrix44.from_x_rotation(math.radians(rotation_x)) @ \
            Matrix44.from_y_rotation(math.radians(rotation_y)) @ \
            Matrix44.from_scale([scale*scale_factor]*3)
    view = Matrix44.look_at(camera_pos, camera_pos + camera_front, camera_up)
    projection = Matrix44.perspective_projection(45, WINDOW_WIDTH/WINDOW_HEIGHT, 0.1, 100)

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, model)
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, view)
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, projection)
    glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, camera_pos)

    current_colors = light_colors_list[light_color_index]
    for i in range(len(light_positions)):
        glUniform3fv(glGetUniformLocation(shader, f"lights[{i}].position"), 1, light_positions[i])
        glUniform3fv(glGetUniformLocation(shader, f"lights[{i}].color"), 1, current_colors[i])

    glBindVertexArray(VAO)
    glDrawElements(GL_TRIANGLES, len(indices), GL_UNSIGNED_INT, None)
    glBindVertexArray(0)

    glfw.swap_buffers(window)

glfw.terminate()
