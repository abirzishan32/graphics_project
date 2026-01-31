#ifndef basic_camera_h
#define basic_camera_h

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class BasicCamera {
public:

    glm::vec3 eye;
    glm::vec3 lookAt;
    
    // Orientation angles
    float Yaw, Pitch, Roll;
    
    // Camera vectors
    glm::vec3 Front;
    glm::vec3 Right;
    glm::vec3 Up;
    glm::vec3 WorldUp;
    
    float Zoom, MouseSensitivity, MovementSpeed;

    BasicCamera(float eyeX = 0.0, float eyeY = 1.0, float eyeZ = 3.0, float lookAtX = 0.0, float lookAtY = 0.0, float lookAtZ = 0.0, glm::vec3 viewUpVector = glm::vec3(0.0f, 1.0f, 0.0f))
    {
        eye = glm::vec3(eyeX, eyeY, eyeZ);
        lookAt = glm::vec3(lookAtX, lookAtY, lookAtZ);
        WorldUp = viewUpVector;

        // Calculate initial yaw and pitch from eye to lookAt
        glm::vec3 direction = glm::normalize(lookAt - eye);
        Yaw = glm::degrees(atan2(direction.z, direction.x));
        Pitch = glm::degrees(asin(direction.y));
        Roll = 0.0f;
        
        MovementSpeed = 2.5f;
        MouseSensitivity = 0.1f;
        Zoom = 45.0f;

        updateCameraVectors();
    }

    glm::mat4 createViewMatrix()
    {
        return glm::lookAt(eye, eye + Front, Up);
    }
    
    // Flying simulator movement
    void moveForward(float deltaTime)
    {
        eye += Front * MovementSpeed * deltaTime;
    }
    
    void moveBackward(float deltaTime)
    {
        eye -= Front * MovementSpeed * deltaTime;
    }
    
    void moveLeft(float deltaTime)
    {
        eye -= Right * MovementSpeed * deltaTime;
    }
    
    void moveRight(float deltaTime)
    {
        eye += Right * MovementSpeed * deltaTime;
    }
    
    void moveUp(float deltaTime)
    {
        eye += Up * MovementSpeed * deltaTime;
    }
    
    void moveDown(float deltaTime)
    {
        eye -= Up * MovementSpeed * deltaTime;
    }
    
    // Rotation controls
    void rotatePitch(float angle)
    {
        Pitch += angle;
        // Clamp pitch to avoid gimbal lock
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateCameraVectors();
    }
    
    void rotateYaw(float angle)
    {
        Yaw += angle;
        updateCameraVectors();
    }
    
    void rotateRoll(float angle)
    {
        Roll += angle;
        updateCameraVectors();
    }
    
    // Orbit around a center point
    void orbitAround(glm::vec3 center, float angle, float deltaTime)
    {
        // Calculate current distance from center
        float radius = glm::length(eye - center);
        
        // Update orbit angle
        float orbitSpeed = angle * deltaTime;
        
        // Rotate eye position around center
        glm::vec3 offset = eye - center;
        float cosA = cos(glm::radians(orbitSpeed));
        float sinA = sin(glm::radians(orbitSpeed));
        
        float newX = offset.x * cosA - offset.z * sinA;
        float newZ = offset.x * sinA + offset.z * cosA;
        
        eye.x = center.x + newX;
        eye.z = center.z + newZ;
        
        // Update yaw to keep looking at center
        Yaw += orbitSpeed;
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    // processes input received from a mouse input system
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        Yaw += xoffset * MouseSensitivity;
        Pitch += yoffset * MouseSensitivity;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors();
    }

private:
    // Calculates the Front, Right, and Up vectors from Euler angles
    void updateCameraVectors()
    {
        // Calculate Front vector from Yaw and Pitch
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        
        // Calculate Right and Up vectors
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
        
        // Apply roll rotation to Up and Right vectors
        if (Roll != 0.0f)
        {
            float cosR = cos(glm::radians(Roll));
            float sinR = sin(glm::radians(Roll));
            glm::vec3 newUp = Up * cosR + Right * sinR;
            glm::vec3 newRight = Right * cosR - Up * sinR;
            Up = glm::normalize(newUp);
            Right = glm::normalize(newRight);
        }
    }
};

#endif /* basic_camera_h */
