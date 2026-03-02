#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// tinyobjloader
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// ======================= Mesh =======================

#include <fstream>
#include <sstream>

static std::string loadTextFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "❌ 无法打开文件: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}



struct Vertex {
    glm::vec3 pos;
    glm::vec3 nrm;
};

template <typename T>
static T clampT(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi ? hi : v);
}

static glm::vec3 safeNormalize(const glm::vec3& v) {
    float len = glm::length(v);
    if (len < 1e-8f) return glm::vec3(0, 1, 0);
    return v / len;
}

static void computeNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    std::vector<glm::vec3> acc(vertices.size(), glm::vec3(0));
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t ia = indices[i], ib = indices[i + 1], ic = indices[i + 2];
        glm::vec3 a = vertices[ia].pos;
        glm::vec3 b = vertices[ib].pos;
        glm::vec3 c = vertices[ic].pos;
        glm::vec3 n = glm::cross(b - a, c - a);
        acc[ia] += n; acc[ib] += n; acc[ic] += n;
    }
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].nrm = safeNormalize(acc[i]);
    }
}

static void centerAndScale(std::vector<Vertex>& verts, float targetSize = 2.0f) {
    if (verts.empty()) return;
    glm::vec3 mn(1e30f), mx(-1e30f);
    for (auto& v : verts) {
        mn = glm::min(mn, v.pos);
        mx = glm::max(mx, v.pos);
    }
    glm::vec3 center = (mn + mx) * 0.5f;
    glm::vec3 extent = (mx - mn);
    float maxE = std::max(extent.x, std::max(extent.y, extent.z));
    float s = (maxE > 1e-6f) ? (targetSize / maxE) : 1.0f;

    for (auto& v : verts) {
        v.pos = (v.pos - center) * s;
    }
}

static bool loadOBJFast(const std::string& path,
    std::vector<Vertex>& outVerts,
    std::vector<uint32_t>& outIdx)
{
    std::cout << "▶ Load OBJ: " << path << "\n";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        path.c_str(), nullptr, true);

    if (!warn.empty()) std::cout << "⚠ " << warn << "\n";
    if (!err.empty())  std::cerr << "❌ " << err << "\n";
    if (!ok) return false;

    size_t vCount = attrib.vertices.size() / 3;
    size_t nCount = attrib.normals.size() / 3;
    std::cout << "  v=" << vCount << " vn=" << nCount << " shapes=" << shapes.size() << "\n";

    outVerts.resize(vCount);
    for (size_t i = 0; i < vCount; ++i) {
        outVerts[i].pos = glm::vec3(
            attrib.vertices[3 * i + 0],
            attrib.vertices[3 * i + 1],
            attrib.vertices[3 * i + 2]
        );
        outVerts[i].nrm = glm::vec3(0);
    }

    // indices: 直接 vertex_index
    size_t totalIdx = 0;
    for (auto& s : shapes) totalIdx += s.mesh.indices.size();
    outIdx.clear();
    outIdx.reserve(totalIdx);

    for (auto& s : shapes) {
        for (auto& id : s.mesh.indices) {
            if (id.vertex_index >= 0) outIdx.push_back((uint32_t)id.vertex_index);
        }
    }

    // 如果 OBJ 有法线，可以直接填；这里为了稳定统一，直接算一遍法线（简化）
    computeNormals(outVerts, outIdx);

    return true;
}

// ======================= Shader helpers =======================
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, 4096, nullptr, log);
        std::cerr << "❌ Shader compile error:\n" << log << "\n";
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, 4096, nullptr, log);
        std::cerr << "❌ Program link error:\n" << log << "\n";
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// ======================= Camera (Orbit + FPS) =======================
enum class CamMode { Orbit = 0, FPS = 1 };

struct OrbitCamera {
    glm::vec3 target{ 0,0,0 };
    float yaw = -90.0f;   // degrees
    float pitch = 15.0f;  // degrees
    float distance = 3.0f;

    glm::vec3 up{ 0,1,0 };

    glm::vec3 position() const {
        float cy = cos(glm::radians(yaw));
        float sy = sin(glm::radians(yaw));
        float cp = cos(glm::radians(pitch));
        float sp = sin(glm::radians(pitch));
        glm::vec3 dir = glm::vec3(cy * cp, sp, sy * cp);
        return target - dir * distance;
    }

    glm::mat4 view() const {
        return glm::lookAt(position(), target, up);
    }
};

struct FPSCamera {
    glm::vec3 pos{ 0, 0.8f, 3.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;

    glm::vec3 forward() const {
        float cy = cos(glm::radians(yaw));
        float sy = sin(glm::radians(yaw));
        float cp = cos(glm::radians(pitch));
        float sp = sin(glm::radians(pitch));
        return safeNormalize(glm::vec3(cy * cp, sp, sy * cp));
    }

    glm::mat4 view() const {
        glm::vec3 f = forward();
        return glm::lookAt(pos, pos + f, glm::vec3(0, 1, 0));
    }
};

// 全局输入状态
static CamMode g_mode = CamMode::Orbit;
static OrbitCamera g_orbit;
static FPSCamera   g_fps;

static bool g_lmb = false;
static bool g_rmb = false;
static double g_lastX = 0, g_lastY = 0;
static bool g_firstMouse = true;

// 鼠标灵敏度
static float g_mouseRot = 0.15f;
static float g_mousePan = 0.0025f;

// 滚轮缩放
static void scroll_callback(GLFWwindow*, double, double yoffset) {
    if (g_mode == CamMode::Orbit) {
        g_orbit.distance *= (yoffset > 0) ? 0.9f : 1.1f;
        g_orbit.distance = clampT(g_orbit.distance, 0.3f, 50.0f);
    }
    else {
        // FPS 模式滚轮可做 FOV 或速度，这里不处理
    }
}

static void mouse_button_callback(GLFWwindow* win, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)  g_lmb = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_RIGHT) g_rmb = (action == GLFW_PRESS);

    if (action == GLFW_PRESS) {
        g_firstMouse = true;
        // FPS 模式左键按住锁定鼠标更像游戏（可选）
        if (g_mode == CamMode::FPS && button == GLFW_MOUSE_BUTTON_LEFT) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (action == GLFW_RELEASE) {
        if (g_mode == CamMode::FPS && button == GLFW_MOUSE_BUTTON_LEFT) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

static void cursor_pos_callback(GLFWwindow*, double xpos, double ypos) {
    if (g_firstMouse) {
        g_lastX = xpos; g_lastY = ypos;
        g_firstMouse = false;
        return;
    }
    double dx = xpos - g_lastX;
    double dy = ypos - g_lastY;
    g_lastX = xpos; g_lastY = ypos;

    if (g_mode == CamMode::Orbit) {
        // 左键旋转
        if (g_lmb) {
            g_orbit.yaw += (float)dx * g_mouseRot;
            g_orbit.pitch -= (float)dy * g_mouseRot;
            g_orbit.pitch = clampT(g_orbit.pitch, -89.0f, 89.0f);
        }
        // 右键平移（pan）
        if (g_rmb) {
            glm::vec3 camPos = g_orbit.position();
            glm::vec3 f = safeNormalize(g_orbit.target - camPos);
            glm::vec3 r = safeNormalize(glm::cross(f, glm::vec3(0, 1, 0)));
            glm::vec3 u = safeNormalize(glm::cross(r, f));
            float scale = g_orbit.distance;
            g_orbit.target += (-r * (float)dx + u * (float)dy) * (g_mousePan * scale);
        }
    }
    else {
        // FPS：只要鼠标在动就旋转（如果你觉得太敏感，可以改为按住左键才旋转）
        g_fps.yaw += (float)dx * g_mouseRot;
        g_fps.pitch -= (float)dy * g_mouseRot;
        g_fps.pitch = clampT(g_fps.pitch, -89.0f, 89.0f);
    }
}

static void process_keyboard(GLFWwindow* win, float dt) {
    // 模式切换
    if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) g_mode = CamMode::Orbit;
    if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) g_mode = CamMode::FPS;

    if (g_mode != CamMode::FPS) return;

    float speed = 2.5f;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed = 6.0f;
    float v = speed * dt;

    glm::vec3 f = g_fps.forward();
    glm::vec3 r = safeNormalize(glm::cross(f, glm::vec3(0, 1, 0)));
    glm::vec3 u = glm::vec3(0, 1, 0);

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) g_fps.pos += f * v;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) g_fps.pos -= f * v;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) g_fps.pos -= r * v;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) g_fps.pos += r * v;
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) g_fps.pos -= u * v;
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) g_fps.pos += u * v;
}

// ======================= Main =======================
int main() {
    std::cout << "===== HW3 OBJ Viewer =====\n";
    if (!glfwInit()) { std::cerr << "glfwInit failed\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1100, 780, "HW3: OBJ + MultiLight + Orbit/FPS", nullptr, nullptr);
    if (!win) { std::cerr << "create window failed\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetScrollCallback(win, scroll_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_pos_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "glad init failed\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    std::cout << "GPU: " << glGetString(GL_RENDERER) << "\n";

    // ===== Load OBJ =====
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const char* objPath = R"(D:\Data\Github\graphics2025\22551012张宏远\Homework1\opengl\main.obj)";
    if (!loadOBJFast(objPath, vertices, indices)) {
        std::cerr << "load obj failed\n";
        return -1;
    }
    centerAndScale(vertices);

    std::cout << "mesh: verts=" << vertices.size() << " idx=" << indices.size() << "\n";

    // ===== Upload Mesh =====
    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nrm));

    glBindVertexArray(0);

    std::string vsCode = loadTextFile("mesh.vert");
    std::string fsCode = loadTextFile("mesh.frag");

    if (vsCode.empty() || fsCode.empty()) {
        std::cerr << "❌ Shader 文件读取失败" << std::endl;
        return -1;
    }

    GLuint prog = linkProgram(
        compileShader(GL_VERTEX_SHADER, vsCode.c_str()),
        compileShader(GL_FRAGMENT_SHADER, fsCode.c_str())
    );

    //GLuint prog = linkProgram(compileShader(GL_VERTEX_SHADER, vsSrc),
    //    compileShader(GL_FRAGMENT_SHADER, fsSrc));

    // ===== Uniform locations（简单用 glGetUniformLocation 直接取）=====
    // lights setup
    glm::vec3 dirDir(-0.3f, -1.0f, -0.2f);
    glm::vec3 dirColor(1.0f, 1.0f, 1.0f);

    struct PointL { glm::vec3 pos, color; float intensity, radius; };
    std::vector<PointL> points = {
        { glm::vec3(2.0f,  2.0f,  2.0f), glm::vec3(1.0f, 0.6f, 0.6f), 2.0f, 6.0f },
        { glm::vec3(-2.0f,  1.5f, -1.5f), glm::vec3(0.6f, 0.7f, 1.0f), 1.8f, 6.0f },
    };

    // ===== Render loop =====
    double lastTime = glfwGetTime();

    std::cout <<
        "Controls:\n"
        "  [1] Orbit  [2] FPS\n"
        "Orbit:\n"
        "  LMB drag rotate, RMB drag pan, Wheel zoom\n"
        "FPS:\n"
        "  Mouse look, WASD move, Q/E down/up, Shift faster\n";

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(win, 1);
        }

        process_keyboard(win, dt);

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        float aspect = (h == 0) ? 1.0f : (float)w / (float)h;

        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);

        glm::mat4 view;
        glm::vec3 camPos;
        if (g_mode == CamMode::Orbit) {
            view = g_orbit.view();
            camPos = g_orbit.position();
        }
        else {
            view = g_fps.view();
            camPos = g_fps.pos;
        }

        glm::mat4 model(1.0f);
        // 轻微旋转演示（你可以注释掉）
        model = glm::rotate(model, (float)now * 0.2f, glm::vec3(0, 1, 0));

        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(prog, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(prog, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));

        glUniform3fv(glGetUniformLocation(prog, "uCamPos"), 1, glm::value_ptr(camPos));

        // material
        glm::vec3 albedo(0.75f, 0.75f, 0.8f);
        glUniform3fv(glGetUniformLocation(prog, "uAlbedo"), 1, glm::value_ptr(albedo));
        glUniform1f(glGetUniformLocation(prog, "uShininess"), 48.0f);

        // directional
        glUniform3fv(glGetUniformLocation(prog, "uDir.dir"), 1, glm::value_ptr(dirDir));
        glUniform3fv(glGetUniformLocation(prog, "uDir.color"), 1, glm::value_ptr(dirColor));
        glUniform1f(glGetUniformLocation(prog, "uDir.intensity"), 1.0f);

        // point lights
        int pc = (int)std::min<size_t>(points.size(), 8);
        glUniform1i(glGetUniformLocation(prog, "uPointCount"), pc);
        for (int i = 0; i < pc; ++i) {
            std::string base = "uPoints[" + std::to_string(i) + "].";
            glUniform3fv(glGetUniformLocation(prog, (base + "pos").c_str()), 1, glm::value_ptr(points[i].pos));
            glUniform3fv(glGetUniformLocation(prog, (base + "color").c_str()), 1, glm::value_ptr(points[i].color));
            glUniform1f(glGetUniformLocation(prog, (base + "intensity").c_str()), points[i].intensity);
            glUniform1f(glGetUniformLocation(prog, (base + "radius").c_str()), points[i].radius);
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteProgram(prog);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
