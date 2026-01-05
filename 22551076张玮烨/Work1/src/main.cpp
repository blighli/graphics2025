//张玮烨-22551076

#define _USE_MATH_DEFINES
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <iostream>

bool drag_left = false;
double x_last = 0.0, y_last = 0.0;
float angle_x = 0.0f, angle_y = 0.0f;
float auto_speed = 60.0f * (float)M_PI / 180.0f;  
double last_t = 0.0;

// 顶点/片段着色器
static const char* vs_src = R"(
#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
out vec3 v_col;
uniform mat4 mvp;
void main(){
    gl_Position = mvp * vec4(pos,1.0);
    v_col = col;
}
)";

static const char* fs_src = R"(
#version 330 core
in vec3 v_col;
out vec4 f_color;
void main(){
    f_color = vec4(v_col,1.0);
}
)";

// 着色器编译函数
GLuint compile_shader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    return sh;
}

GLuint make_program()
{
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    return prog;
}

// 鼠标回调
void mouse_button_callback(GLFWwindow* wnd, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        drag_left = (action == GLFW_PRESS);
        if (drag_left) glfwGetCursorPos(wnd, &x_last, &y_last);
    }
}

void cursor_position_callback(GLFWwindow* wnd, double x, double y)
{
    if (!drag_left) return;
    double dx = x - x_last;
    double dy = y - y_last;
    x_last = x; y_last = y;
    angle_x += (float)dy * 0.05f;
    angle_y += (float)dx * 0.05f;
}

void framebuffer_size_callback(GLFWwindow* wnd, int w, int h) {
    if (h == 0) return;
    glViewport(0, 0, w, h);
}

// 列主序透视投影矩阵
void perspective(float fovy_rad, float aspect, float zNear, float zFar, float out[16]) {
    for (int i = 0; i < 16; ++i) out[i] = 0.0f;
    float f = 1.0f / std::tan(fovy_rad * 0.5f);
    out[0] = f / aspect; 
    out[5] = f;          
    out[10] = (zFar + zNear) / (zNear - zFar); 
    out[11] = -1.0f;     
    out[14] = (2.0f * zFar * zNear) / (zNear - zFar); 
    out[15] = 0.0f;
}

//平移矩阵 (列主序)
void translate(float tx, float ty, float tz, float out[16]) {

    for (int i = 0; i < 16; ++i) out[i] = 0.0f;
    out[0]=1; out[5]=1; out[10]=1; out[15]=1;
    out[12] = tx;
    out[13] = ty;
    out[14] = tz;
}

// 绕X再绕Y 的旋转矩阵 (列主序)
void rotate_xy(float ax, float ay, float out[16]) {
    float cx = std::cos(ax), sx = std::sin(ax);
    float cy = std::cos(ay), sy = std::sin(ay);

    // Rx
    float Rx[16] = {
        1,0,0,0,
        0,cx,sx,0,
        0,-sx,cx,0,
        0,0,0,1
    };
    // Ry
    float Ry[16] = {
        cy,0,-sy,0,
        0,1,0,0,
        sy,0,cy,0,
        0,0,0,1
    };

    //Ry * Rx
    for (int r=0; r<4; ++r) {
        for (int c=0; c<4; ++c) {
            float sum = 0.0f;
            for (int k=0; k<4; ++k) {
                sum += Ry[k*4 + r] * Rx[c*4 + k];
            }
            out[c*4 + r] = sum;
        }
    }
}

// 矩阵乘法 C = A * B  (列主序)
void mat4_mul(const float A[16], const float B[16], float C[16]) {
    for (int r=0; r<4; ++r) {
        for (int c=0; c<4; ++c) {
            float sum = 0.0f;
            for (int k=0; k<4; ++k) {
                sum += A[k*4 + r] * B[c*4 + k];
            }
            C[c*4 + r] = sum;
        }
    }
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "张玮烨-22551076-彩色三角形", NULL, NULL);


    glfwMakeContextCurrent(win);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_position_callback);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


    // 初始视口
    int w, h;
    glfwGetFramebufferSize(win, &w, &h);
    glViewport(0,0,w,h);

    // 三个顶点（每个顶点 3pos + 3color）
    float verts[] = {
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // 0
        0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // 1
        0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // 2
    };

    // 索引，绘制一个三角形
    unsigned int indices[] = {0, 1, 2};


    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint prog = make_program();
    GLint mvp_loc = glGetUniformLocation(prog, "mvp");

    last_t = glfwGetTime();

    
    float proj[16];
    perspective(45.0f * (float)M_PI / 180.0f, (float)w / (float)h, 0.1f, 100.0f, proj);

    //相机移动-3
    float view[16];
    translate(0.0f, 0.0f, -3.0f, view);


    while (!glfwWindowShouldClose(win)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double t = glfwGetTime();
        double dt = t - last_t;
        last_t = t;

        if (!drag_left) angle_y += auto_speed * (float)dt;

        // model = rotation
        float model[16];
        rotate_xy(angle_x, angle_y, model);

        // mv = view * model
        float mv[16];
        mat4_mul(view, model, mv);

        // mvp = proj * mv
        float mvp[16];
        mat4_mul(proj, mv, mvp);

        glUseProgram(prog);

        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
} 