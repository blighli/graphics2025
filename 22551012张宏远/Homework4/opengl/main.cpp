// src/main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

// Windows: create directory (avoid std::filesystem)
#include <direct.h> // _mkdir

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Mesh.h"
#include "ObjLoader.h"

// Screenshot
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// -------------------- callbacks --------------------
static void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

// -------------------- helpers --------------------
static glm::vec3 safeNormalize(const glm::vec3& v) {
    float len = glm::length(v);
    if (len < 1e-8f) return glm::vec3(0, 1, 0);
    return v / len;
}

static void ensureDir(const std::string& dir) {
    _mkdir(dir.c_str()); // ok if exists
}

static bool keyPressedOnce(GLFWwindow* win, int key) {
    static bool prev[512] = { false };
    bool now = glfwGetKey(win, key) == GLFW_PRESS;
    bool pressed = now && !prev[key];
    prev[key] = now;
    return pressed;
}

static void saveScreenshotPNG(const std::string& path, int w, int h) {
    std::vector<unsigned char> pixels(w * h * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    std::vector<unsigned char> flipped(w * h * 4);
    for (int y = 0; y < h; y++) {
        memcpy(&flipped[y * w * 4], &pixels[(h - 1 - y) * w * 4], w * 4);
    }

    int ok = stbi_write_png(path.c_str(), w, h, 4, flipped.data(), w * 4);
    std::cout << (ok ? "✔ Screenshot: " : "❌ Screenshot failed: ") << path << "\n";
}

static glm::vec3 neonColor(float t, float phase) {
    return glm::vec3(
        0.5f + 0.5f * std::sin(t * 1.3f + phase + 0.0f),
        0.5f + 0.5f * std::sin(t * 1.3f + phase + 2.1f),
        0.5f + 0.5f * std::sin(t * 1.3f + phase + 4.2f)
    );
}

// Generate a low-poly sphere using VertexPN (pos+nrm)
static void buildSpherePN(int stacks, int slices,
    std::vector<VertexPN>& outV,
    std::vector<uint32_t>& outI)
{
    outV.clear();
    outI.clear();
    outV.reserve((stacks + 1) * (slices + 1));
    outI.reserve(stacks * slices * 6);

    const float PI = 3.1415926535f;

    for (int y = 0; y <= stacks; y++) {
        float v = (float)y / (float)stacks;
        float phi = v * PI; // [0, PI]

        for (int x = 0; x <= slices; x++) {
            float u = (float)x / (float)slices;
            float theta = u * 2.0f * PI; // [0, 2PI]

            float sx = std::sin(phi) * std::cos(theta);
            float sy = std::cos(phi);
            float sz = std::sin(phi) * std::sin(theta);

            VertexPN vert;
            vert.pos = glm::vec3(sx, sy, sz); // radius=1
            vert.nrm = glm::vec3(sx, sy, sz);
            outV.push_back(vert);
        }
    }

    auto idxAt = [slices](int y, int x) {
        return (uint32_t)(y * (slices + 1) + x);
        };

    for (int y = 0; y < stacks; y++) {
        for (int x = 0; x < slices; x++) {
            uint32_t i0 = idxAt(y, x);
            uint32_t i1 = idxAt(y, x + 1);
            uint32_t i2 = idxAt(y + 1, x);
            uint32_t i3 = idxAt(y + 1, x + 1);

            outI.push_back(i0); outI.push_back(i2); outI.push_back(i1);
            outI.push_back(i1); outI.push_back(i2); outI.push_back(i3);
        }
    }
}

// -------------------- input / camera --------------------
struct InputState {
    bool lmb = false, rmb = false;
    bool firstMouse = true;
    double lastX = 0, lastY = 0;

    bool paused = false;
    bool showGrid = true;
    bool modelAutoRotate = true;
    int lightPreset = 2; // 0 warm, 1 cool, 2 neon (default)
};

static InputState g_in;
static CameraSystem g_cam;

static float g_mouseRot = 0.15f;
static float g_mousePan = 0.0025f;

static void scroll_callback(GLFWwindow*, double, double yoffset) {
    if (g_cam.mode == CamMode::Orbit) {
        g_cam.orbit.distance *= (yoffset > 0) ? 0.9f : 1.1f;
        g_cam.orbit.distance = clampT(g_cam.orbit.distance, 0.3f, 80.0f);
    }
}

static void mouse_button_callback(GLFWwindow* win, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)  g_in.lmb = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_RIGHT) g_in.rmb = (action == GLFW_PRESS);

    if (action == GLFW_PRESS) {
        g_in.firstMouse = true;
        if (g_cam.mode == CamMode::FPS && button == GLFW_MOUSE_BUTTON_LEFT) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (action == GLFW_RELEASE) {
        if (g_cam.mode == CamMode::FPS && button == GLFW_MOUSE_BUTTON_LEFT) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

static void cursor_pos_callback(GLFWwindow*, double xpos, double ypos) {
    if (g_in.firstMouse) {
        g_in.lastX = xpos; g_in.lastY = ypos;
        g_in.firstMouse = false;
        return;
    }
    double dx = xpos - g_in.lastX;
    double dy = ypos - g_in.lastY;
    g_in.lastX = xpos; g_in.lastY = ypos;

    if (g_cam.mode == CamMode::Orbit) {
        if (g_in.lmb) {
            g_cam.orbit.yaw += (float)dx * g_mouseRot;
            g_cam.orbit.pitch -= (float)dy * g_mouseRot;
            g_cam.orbit.pitch = clampT(g_cam.orbit.pitch, -89.0f, 89.0f);
        }
        if (g_in.rmb) {
            glm::vec3 camPos = g_cam.orbit.position();
            glm::vec3 f = safeNormalize(g_cam.orbit.target - camPos);
            glm::vec3 r = safeNormalize(glm::cross(f, glm::vec3(0, 1, 0)));
            glm::vec3 u = safeNormalize(glm::cross(r, f));
            float scale = g_cam.orbit.distance;
            g_cam.orbit.target += (-r * (float)dx + u * (float)dy) * (g_mousePan * scale);
        }
    }
    else {
        g_cam.fps.yaw += (float)dx * g_mouseRot;
        g_cam.fps.pitch -= (float)dy * g_mouseRot;
        g_cam.fps.pitch = clampT(g_cam.fps.pitch, -89.0f, 89.0f);
    }
}

static void process_keyboard(GLFWwindow* win, float dt) {
    if (keyPressedOnce(win, GLFW_KEY_1)) g_cam.mode = CamMode::Orbit;
    if (keyPressedOnce(win, GLFW_KEY_2)) g_cam.mode = CamMode::FPS;

    if (keyPressedOnce(win, GLFW_KEY_SPACE)) g_in.paused = !g_in.paused;
    if (keyPressedOnce(win, GLFW_KEY_G)) g_in.showGrid = !g_in.showGrid;
    if (keyPressedOnce(win, GLFW_KEY_M)) g_in.modelAutoRotate = !g_in.modelAutoRotate;
    if (keyPressedOnce(win, GLFW_KEY_L)) g_in.lightPreset = (g_in.lightPreset + 1) % 3;
    if (keyPressedOnce(win, GLFW_KEY_R)) g_cam.reset();

    if (g_cam.mode != CamMode::FPS) return;

    float speed = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 6.0f : 2.5f;
    float v = speed * dt;

    glm::vec3 f = g_cam.fps.forward();
    glm::vec3 r = safeNormalize(glm::cross(f, glm::vec3(0, 1, 0)));
    glm::vec3 u(0, 1, 0);

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) g_cam.fps.pos += f * v;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) g_cam.fps.pos -= f * v;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) g_cam.fps.pos -= r * v;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) g_cam.fps.pos += r * v;
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) g_cam.fps.pos -= u * v;
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) g_cam.fps.pos += u * v;
}

// -------------------- main --------------------
int main() {
    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(1200, 800, "FinalShowroom - Light Show", nullptr, nullptr);
    if (!win) {
        std::cerr << "create window failed\n";
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetScrollCallback(win, scroll_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_pos_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "glad init failed\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    std::cout <<
        "Controls:\n"
        "  [1] Orbit   [2] FPS\n"
        "  Orbit: LMB rotate, RMB pan, Wheel zoom\n"
        "  FPS: Mouse look, WASD move, Q/E down/up, Shift faster (LMB lock cursor)\n"
        "  Space pause, L light preset, G grid, M model rotate, R reset, P screenshot, ESC quit\n";

    // ---------------- Shaders ----------------
    Shader skySh, floorSh, meshSh, lightSh;
    if (!skySh.loadFromFiles("sky.vert", "sky.frag")) return -1;
    if (!floorSh.loadFromFiles("floor.vert", "floor.frag")) return -1;
    if (!meshSh.loadFromFiles("mesh.vert", "mesh.frag")) return -1;
    if (!lightSh.loadFromFiles("light.vert", "light.frag")) return -1;

    // ---------------- Fullscreen quad for sky ----------------
    GLuint skyVAO = 0, skyVBO = 0;
    float skyVerts[] = {
        -1.f, -1.f,
         1.f, -1.f,
         1.f,  1.f,
        -1.f, -1.f,
         1.f,  1.f,
        -1.f,  1.f
    };
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVerts), skyVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // ---------------- Load OBJ ----------------
    std::string objPath = "main.obj";
    // objPath = R"(D:\Data\Github\graphics2025\22551012张宏远\Homework1\opengl\main.obj)";

    std::vector<VertexPN> mVerts;
    std::vector<uint32_t> mIdx;
    if (!loadOBJ_PN_Fast(objPath, mVerts, mIdx)) {
        std::cerr << "OBJ load failed. Check path: " << objPath << "\n";
        return -1;
    }
    centerAndScalePN(mVerts, 2.5f);

    Mesh modelMesh;
    modelMesh.upload(mVerts, mIdx);

    // ---------------- Floor plane ----------------
    std::vector<VertexPN> fVerts = {
        {{-40.f, -1.f, -40.f}, {0,1,0}},
        {{ 40.f, -1.f, -40.f}, {0,1,0}},
        {{ 40.f, -1.f,  40.f}, {0,1,0}},
        {{-40.f, -1.f,  40.f}, {0,1,0}},
    };
    std::vector<uint32_t> fIdx = { 0,1,2, 0,2,3 };
    Mesh floorMesh;
    floorMesh.upload(fVerts, fIdx);

    // ---------------- Light ball mesh (sphere) ----------------
    Mesh lightBallMesh;
    {
        std::vector<VertexPN> sv;
        std::vector<uint32_t> si;
        buildSpherePN(12, 24, sv, si);
        lightBallMesh.upload(sv, si);
    }

    // ---------------- Scene lights ----------------
    glm::vec3 dirDir(-0.3f, -1.0f, -0.2f);

    // screenshot dir
    ensureDir("screenshots");
    int shotId = 0;

    double lastTime = glfwGetTime();
    double animTime = 0.0;

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, 1);

        process_keyboard(win, dt);
        if (!g_in.paused) animTime += dt;

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        float aspect = (h == 0) ? 1.0f : (float)w / (float)h;

        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);
        glm::mat4 view = g_cam.view();
        glm::vec3 camPos = g_cam.camPos();

        // ---------------- Draw sky background ----------------
        glDisable(GL_DEPTH_TEST);
        skySh.use();
        skySh.setFloat("uTime", (float)animTime);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        // ---------------- Animate point lights ----------------
        const int MAXP = 6; // <= 8
        glm::vec3 pointPos[MAXP];
        glm::vec3 pointCol[MAXP];
        float pointIntensity[MAXP];

        float t = (float)animTime;

        for (int i = 0; i < MAXP; i++) {
            float phase = i * 1.2f;

            float R = 4.2f + 0.8f * std::sin(t * 0.7f + phase);
            float y = 1.0f + 0.8f * std::sin(t * 0.9f + phase * 0.6f);
            pointPos[i] = glm::vec3(R * std::cos(t + phase), y, R * std::sin(t + phase));

            float breathe = 0.55f + 0.45f * (0.5f + 0.5f * std::sin(t * 1.6f + phase));
            pointIntensity[i] = 10.0f * breathe; // keep strong but not insane

            if (g_in.lightPreset == 0) { // warm
                glm::vec3 base(1.0f, 0.75f, 0.55f);
                float k = 0.4f + 0.4f * std::sin(t * 0.9f + phase);
                pointCol[i] = glm::mix(base, glm::vec3(1.0f, 0.25f, 0.15f), k);
            }
            else if (g_in.lightPreset == 1) { // cool
                glm::vec3 base(0.55f, 0.8f, 1.0f);
                float k = 0.4f + 0.4f * std::sin(t * 0.9f + phase);
                pointCol[i] = glm::mix(base, glm::vec3(0.15f, 0.55f, 1.0f), k);
            }
            else { // neon
                pointCol[i] = neonColor(t, phase);
            }
        }

        // ---------------- Draw floor (double-sided) ----------------
        glDisable(GL_CULL_FACE);
        floorSh.use();
        floorSh.setMat4("uView", view);
        floorSh.setMat4("uProj", proj);
        floorSh.setInt("uShowGrid", g_in.showGrid ? 1 : 0);
        floorMesh.draw();
        glEnable(GL_CULL_FACE);

        // ---------------- Draw model ----------------
        glm::mat4 model(1.0f);
        if (g_in.modelAutoRotate) {
            model = glm::rotate(model, (float)animTime * 0.25f, glm::vec3(0, 1, 0));
        }

        meshSh.use();
        meshSh.setMat4("uModel", model);
        meshSh.setMat4("uView", view);
        meshSh.setMat4("uProj", proj);
        meshSh.setVec3("uCamPos", camPos);

        // material
        meshSh.setVec3("uAlbedo", glm::vec3(0.80f, 0.80f, 0.86f));
        meshSh.setFloat("uShininess", 64.0f);

        // directional
        meshSh.setVec3("uDir.dir", dirDir);
        meshSh.setVec3("uDir.color", glm::vec3(1, 1, 1));
        meshSh.setFloat("uDir.intensity", 0.18f);

        // exposure (mesh.frag needs this uniform if you applied tone mapping)
        // If your mesh.frag doesn't have uExposure, you can comment this line.
        meshSh.setFloat("uExposure", 0.50f);

        // point light uniforms
        meshSh.setInt("uPointCount", MAXP);
        for (int i = 0; i < MAXP; i++) {
            std::string base = "uPoints[" + std::to_string(i) + "].";
            meshSh.setVec3((base + "pos").c_str(), pointPos[i]);
            meshSh.setVec3((base + "color").c_str(), pointCol[i]);
            meshSh.setFloat((base + "intensity").c_str(), pointIntensity[i]);

            meshSh.setFloat((base + "a").c_str(), 1.0f);
            meshSh.setFloat((base + "b").c_str(), 0.10f);
            meshSh.setFloat((base + "c").c_str(), 0.02f);
        }

        modelMesh.draw();

        // ---------------- Draw point light balls ----------------
        glDisable(GL_CULL_FACE);
        lightSh.use();
        lightSh.setMat4("uView", view);
        lightSh.setMat4("uProj", proj);

        for (int i = 0; i < MAXP; i++) {
            glm::mat4 lm(1.0f);
            lm = glm::translate(lm, pointPos[i]);
            lm = glm::scale(lm, glm::vec3(0.10f)); // size

            lightSh.setMat4("uModel", lm);
            lightSh.setVec3("uColor", pointCol[i]);
            lightSh.setFloat("uBoost", 5.0f); // brighter

            lightBallMesh.draw();
        }
        glEnable(GL_CULL_FACE);

        glfwSwapBuffers(win);
        glfwPollEvents();

        // screenshot
        if (keyPressedOnce(win, GLFW_KEY_P)) {
            std::ostringstream oss;
            oss << "screenshots/shot_" << std::setw(3) << std::setfill('0') << shotId++ << ".png";
            saveScreenshotPNG(oss.str(), w, h);
        }
    }

    // cleanup
    modelMesh.destroy();
    floorMesh.destroy();
    lightBallMesh.destroy();

    glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &skyVAO);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
