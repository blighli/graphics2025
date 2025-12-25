#pragma once

#include "Camera.h"
#include "Setting.h"
#include <iostream>

// callback
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        newcamera.constrainPitch = false;
    }
    else 
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        newcamera.constrainPitch = true;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
    {
        newcamera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
    {
        newcamera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
    {
        newcamera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
    {
        newcamera.ProcessKeyboard(RIGHT, deltaTime);
    }

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    //if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
    //    mouse = !mouse;
    //    if(mouse)
    //        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    //    else 
    //        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //    return;
    //}
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;   //坐标系关系这y得反的

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
    newcamera.ProcessMouseMovement(xoffset, yoffset, newcamera.constrainPitch);
}

void scroll_callback(GLFWwindow* window, double xoffest, double yoffest)
{
    camera.ProcessMouseScroll(static_cast<float>(xoffest), static_cast<float>(yoffest));
    newcamera.ProcessMouseScroll(static_cast<float>(xoffest), static_cast<float>(yoffest));
}