#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float D_YAW         = -90.0f;
const float D_PITCH       =  0.0f;
const float D_SPEED       =  2.5f;
const float D_SENSITIVITY =  0.05f;
const float D_ZOOM        =  45.0f;

class Camera{

public: 

    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;      // around axis X
    float Pitch;    // around axis Y
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float ZoomScale;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = D_YAW, float pitch = D_PITCH) 
            : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(D_SPEED), MouseSensitivity(D_SENSITIVITY), ZoomScale(D_ZOOM){
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) 
            : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(D_SPEED), MouseSensitivity(D_SENSITIVITY), ZoomScale(D_ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix();

    /// @brief respond to keyboard input, move camera position
    /// @param direction keyboard direction
    /// @param deltaTime the time between frame
    void Move(Camera_Movement direction, float deltaTime);

    /// @brief respond to cursor input, move camera pitch and yaw angle
    void Rotate(float d_pitch, float d_yaw, GLboolean constrainPitch = true);

    /// @brief respond to scroll input, modiry zoom
    void Zoom(float scale);

private:

    void updateCameraVectors();
};


#endif