#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream> // Include iostream for console output

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Default camera values
const float YAW = 0.0f;
const float PITCH = 0.0f;
const float SPEED = 20.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 80.0f;
const float ZOOM_RANGE = 20.0f;
const float ZOOM_SPEED = 2.0f;
const float STADIA = 1000.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }


    // added 260109 returns the projection matrix
    glm::mat4 GetProjMatrix(float aspect)
    {
        return glm::perspective(glm::radians(Zoom), aspect, 0.1f, STADIA);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += WorldUp * velocity;
        if (direction == DOWN)
            Position -= WorldUp * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        if (Zoom >= 1.0f && Zoom <= ZOOM)
            Zoom -= yoffset;
        if (Zoom <= 1.0f)
            Zoom = 1.0f;
        if (Zoom >= ZOOM)
            Zoom = ZOOM;
    }

    // Prints the current camera state to the console
    void PrintState() const
    {
        std::cout << "Camera State:" << std::endl;
        std::cout << "Position: (" << Position.x << ", " << Position.y << ", " << Position.z << ")" << std::endl;
        std::cout << "Front: (" << Front.x << ", " << Front.y << ", " << Front.z << ")" << std::endl;
        std::cout << "Up: (" << Up.x << ", " << Up.y << ", " << Up.z << ")" << std::endl;
        std::cout << "Yaw: " << Yaw << " Pitch: " << Pitch << std::endl;
        std::cout << "Zoom: " << Zoom << std::endl;
    }

     // calculates the front vector from the Camera's (updated) Euler Angles
    // 通过相机的欧拉角计算 Front、Right、Up 向量
    void updateCameraVectors()
    {
        // 计算新的 Front 向量
        glm::vec3 front;
        front.x = -sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = -cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // 重新计算 Right 和 Up 向量
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    // Sets the camera to a fixed position and orientation
    void FixView(glm::vec3 position, float yaw)
    {
        Position = position;
        Yaw = yaw;
        Pitch = 0.0f;
        updateCameraVectors();
    }


    // ---------------- add zoom in/out functions ----------------

    void ZoomIn()
    {
        if (Zoom >= ZOOM - ZOOM_RANGE)
            Zoom -= ZOOM_SPEED;
    }

    void ZoomOut()
    {
        if (Zoom <= ZOOM + ZOOM_RANGE)
            Zoom += ZOOM_SPEED;
    }

    // Gradually recovers Zoom to default value
    void ZoomRecover()
    {
        if (Zoom < ZOOM)
            Zoom += ZOOM_SPEED / 2;
        if (Zoom > ZOOM)
            Zoom -= ZOOM_SPEED / 2;
    }

private:

};
#endif
