#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <vector>
#include <algorithm>

#include "Shader.h"
#include "setting.h"
#include "Callback.h"

#include "Scene.h"
#include "Model.h"

#include "stb_image/stb_image.h"

static glm::vec3 get_pos(int cnt, float r, glm::vec3 center, float ratio)
{
    glm::vec3 offset = r * glm::vec3(glm::cos(cnt / ratio), 0, glm::sin(cnt / ratio));
    return center + offset;
}

unsigned int get_texture_data(std::string path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Triangle", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSwapInterval(1);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    int rot_cnt = 0;

    Material sun_mat = Material(glm::vec3(1.0f, 0.5f, 0.2f));
    sun_mat.emissive = glm::vec3(glm::vec3(1.0f));

    float earth_oribt_size = 1.6f;

    glm::vec3 sun_pos = glm::vec3(0, 0, -3);
    glm::vec3 earth_pos = get_pos(rot_cnt, earth_oribt_size, sun_pos, 10.);
    glm::vec3 moon_pos = get_pos(rot_cnt, 0.5, earth_pos, 3.);

    std::unique_ptr<Sphere> sun = std::make_unique<Sphere>(sun_pos, 0.8f, sun_mat, 0);
    std::unique_ptr<Sphere> earth = std::make_unique<Sphere>(earth_pos, 0.2f, Material(glm::vec3(0.3f, 0.3f, 1.0f)), 1);
    std::unique_ptr<Sphere> moon = std::make_unique<Sphere>(moon_pos, 0.1f, Material(glm::vec3(1)), 2);

    std::vector<shape*> shapes = { sun.get(), earth.get(), moon.get() };

    auto scene = std::make_unique<Scene>(shapes);

    Shader shader("res/shaders/pathTracing.shader");
    shader.Bind();

    shader.SetUniform1i("width", SCR_WIDTH);
    shader.SetUniform1i("height", SCR_HEIGHT);


    std::vector<unsigned int> textures(3);

    textures[0] = get_texture_data("res/textures/sun-texture.jpg");
    textures[1] = get_texture_data("res/textures/earth-planet-day-texture.jpg");
    textures[2] = get_texture_data("res/textures/moon-texture.jpg");


    for (unsigned int i = 0; i < 3; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        std::string uniformName = "textures[" + std::to_string(i) + "]";
        std::cout << uniformName << " " << textures[i] <<std::endl;
        shader.SetUniform1i(uniformName, i);
    }

    extern newCamera newcamera;
    extern float deltaTime;
    extern float lastFrame;

    int frameCounter = 0;
    glm::vec3 lower_left_corner = glm::vec3(-1);
    glm::vec3 horizontal = glm::vec3(2, 0, 0);
    glm::vec3 vertical = glm::vec3(0, 2, 0);
    glm::vec3 origin = glm::vec3(0);


    const float time_for_per_frame = 1.0f;
    float sum_time = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        sum_time += deltaTime;

        // input
        // -----
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene->clearData();
        rot_cnt++;

        earth_pos = get_pos(rot_cnt, earth_oribt_size, sun_pos, 365.);
        moon_pos = get_pos(rot_cnt, 0.5, earth_pos, 30.);

        // std::cout << earth_pos[0] << " " << earth_pos[1] << " " << earth_pos[2] << std::endl;

        earth = std::make_unique<Sphere>(earth_pos, 0.2f, Material(glm::vec3(0.3f, 0.3f, 1.0f)), 1);
        moon = std::make_unique<Sphere>(moon_pos, 0.1f, Material(glm::vec3(1), 0.9, 0.1), 2);

        scene->push(sun.get());
        scene->push(earth.get());
        scene->push(moon.get());
       

        shader.Bind();
        shader.SetUniform1ui("frameCount", frameCounter++);
        shader.SetUniform3fv("camera.lower_left_corner", lower_left_corner);
        shader.SetUniform3fv("camera.horizontal", horizontal);
        shader.SetUniform3fv("camera.vertical", vertical);
        shader.SetUniform3fv("camera.origin", origin);

        scene->Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}