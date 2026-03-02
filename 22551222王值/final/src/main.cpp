#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"  // 矩阵运算相关
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "camera.h"     // 摄像头相关
#include "renderAll.h"
#include "box.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <windows.h>
#include <cmath>

// 回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// 摄像机
Camera camera(glm::vec3(0.0f, -5.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool isBothPlaced = false, bluePlaced = false, orangePlaced = false;

RenderAll* renderAll = nullptr;
bg::Bg *selectedThing = nullptr;

float lastFrame = 0;
float deltaTime = 0;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                  // version 3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                  // versino .3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // core mode

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Portal", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

   renderAll = new RenderAll(camera, SCR_WIDTH, SCR_HEIGHT);

    // render loop
    while (!glfwWindowShouldClose(window)) {

        glEnable(GL_DEPTH_TEST);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // input
        processInput(window);

        // 清空颜色缓冲
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render sth
        renderAll->RenderScene(camera);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
        Sleep(10);
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    delete(renderAll);
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    int res = 0;
    glm::vec3 newPos;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        res = camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        res = camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        res = camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        res = camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        res = camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        res = camera.ProcessKeyboard(DOWN, deltaTime);
    if (res == Trigger::INBLUE) {
        // std::cout << "in blue" << std::endl;
        newPos = renderAll->getPortalPtr(true)->ConvertPointToOutPortal(camera.GetPos());
        camera.SetPos(newPos);
        camera.SetFront(renderAll->getPortalPtr(true)->ConvertDirectionToOutPortal(camera.GetFront()));
        cameraBox.get()->setP1(glm::vec3(newPos.x + BOXSIZE / 2, newPos.y + BOXSIZE / 2, newPos.z + BOXSIZE / 2));
        cameraBox.get()->setP2(glm::vec3(newPos.x - BOXSIZE / 2, newPos.y - BOXSIZE / 2, newPos.z - BOXSIZE / 2));
    } else if (res == Trigger::INORANGE) {
        // std::cout << "in orange" << std::endl;
        newPos = renderAll->getPortalPtr(false)->ConvertPointToOutPortal(camera.GetPos());
        camera.SetPos(newPos);
        camera.SetFront(renderAll->getPortalPtr(false)->ConvertDirectionToOutPortal(camera.GetFront()));
        cameraBox.get()->setP1(glm::vec3(newPos.x + BOXSIZE / 2, newPos.y + BOXSIZE / 2, newPos.z + BOXSIZE / 2));
        cameraBox.get()->setP2(glm::vec3(newPos.x - BOXSIZE / 2, newPos.y - BOXSIZE / 2, newPos.z - BOXSIZE / 2));
    }
    if(selectedThing)
    {
        glm::vec3 delta = glm::vec3(0.f);
        float velocity = camera.MovementSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            delta = glm::normalize(glm::vec3(1.f, 0.f, 1.f) * camera.Front);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            delta = -glm::normalize(glm::vec3(1.f, 0.f, 1.f) * camera.Front);
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            delta = -camera.Right;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            delta = camera.Right;
        if (glfwGetKey(window, GLFW_KEY_KP_MULTIPLY) == GLFW_PRESS)
            delta = glm::vec3(0.f, 1.f, 0.f);
        if (glfwGetKey(window, GLFW_KEY_KP_DIVIDE) == GLFW_PRESS)
            delta = -glm::vec3(0.f, 1.f, 0.f);
        if(delta != glm::vec3(0.f))
        {
            selectedThing->SetPosition(delta * velocity);
            glm::vec3 p1, p2;
            selectedThing->getRenderable()->GetP1P2(p1, p2);
            (*selectedThing->box).setP1(p1);
            (*selectedThing->box).setP2(p2);
            if(selectedThing == renderAll->bgs.back())
            {
                renderAll->lightRenderer->lightPosition =  renderAll->lightRenderer->getLightPosition() + delta;
            }
        }
    }
    
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback([[maybe_unused]]GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback([[maybe_unused]]GLFWwindow* window, double xpos, double ypos) {
    float xpos_f = static_cast<float>(xpos);
    float ypos_f = static_cast<float>(ypos);
    if (firstMouse) {
        lastX = xpos_f;
        lastY = ypos_f;
        firstMouse = false;
    }
    float xoffset = xpos_f - lastX;
    float yoffset = ypos_f - lastY;
    lastX = xpos_f;
    lastY = ypos_f;
    // 这里设置了一下反转y轴
    camera.ProcessMouseMovement(xoffset, -yoffset);
}

void mouse_button_callback([[maybe_unused]]GLFWwindow* window, int button, int action, [[maybe_unused]]int mods) {
    if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS) {
        // std::cout << "Mouse button pressed" << std::endl;
        glm::vec3 resPos, resDir;
        resPos = glm::vec3(INF, INF, INF);
        bool isPlace = false;
        for (auto b : wallBoxList) {
            if (Box::checkPlaceAt(*b.get(), camera.Position, camera.GetFront(), resPos, resDir)) {
                isPlace = true;
            }
        }
        if (isPlace) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                // std::cout << "Left mouse button pressed" << std::endl;
                renderAll->getPortalPtr(true)->PlaceAt(resPos, resDir);
                bluePlaced = true;
                if(bluePlaced && orangePlaced) isBothPlaced = true;
            } else {
                // std::cout << "Right mouse button pressed" << std::endl;
                renderAll->getPortalPtr(false)->PlaceAt(resPos, resDir);
                orangePlaced = true;
                if(bluePlaced && orangePlaced) isBothPlaced = true;
            }
            // std::cout << "Pos  " << resPos.x << " " << resPos.y << " " << resPos.z << std::endl;
            // std::cout << "Norm " << resDir.x << " " << resDir.y << " " << resDir.z << std::endl;
        }
    }else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glm::vec3 resPos, resDir;
        resPos = glm::vec3(INF, INF, INF);
        auto t_selected_thing = selectedThing;
        auto t_resPos = resPos;
        for(auto bg: renderAll->bgs)
        {
            t_resPos = resPos;
            if(Box::checkPlaceAt(*bg->box, camera.Position, camera.GetFront(), resPos, resDir))
            {
                std::cout << "Thing selected!" << std::endl;
                if(t_resPos != resPos) t_selected_thing = bg;
            }
        }
        selectedThing = t_selected_thing;
    }
}

void scroll_callback([[maybe_unused]]GLFWwindow* window, [[maybe_unused]]double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}