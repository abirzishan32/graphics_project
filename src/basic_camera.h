#ifndef BASIC_CAMERA_H
#define BASIC_CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  8.5f;  // Walking speed (m/s)
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

// First-person camera class for pedestrian-scale navigation
class BasicCamera {
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    // Euler Angles
    float Yaw;
    float Pitch;
    
    // Camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    
    // Fixed height for pedestrian view
    float EyeHeight;

    // Constructor with vectors
    BasicCamera(glm::vec3 position = glm::vec3(0.0f, 1.7f, 0.0f), 
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
                float yaw = YAW, float pitch = PITCH) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM) 
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        EyeHeight = position.y;
        updateCameraVectors();
    }
    
    // Constructor with scalar values
    BasicCamera(float posX, float posY, float posZ, 
                float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f, 
                float yaw = YAW, float pitch = PITCH) 
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
          MovementSpeed(SPEED), 
          MouseSensitivity(SENSITIVITY), 
          Zoom(ZOOM) 
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        EyeHeight = posY;
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Processes input received from any keyboard-like input system
    // Accepts input parameter in the form of camera defined ENUM
    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        
        // Create horizontal movement vectors (ignore Y component for walking)
        glm::vec3 frontFlat = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
        glm::vec3 rightFlat = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));
        
        if (direction == FORWARD)
            Position += frontFlat * velocity;
        if (direction == BACKWARD)
            Position -= frontFlat * velocity;
        if (direction == LEFT)
            Position -= rightFlat * velocity;
        if (direction == RIGHT)
            Position += rightFlat * velocity;
        
        // Keep camera at eye height (pedestrian walking)
        Position.y = EyeHeight;
    }

    // Processes input received from a mouse input system
    // Expects the offset value in both the x and y direction
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // Processes input received from a mouse scroll-wheel event
    void ProcessMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors() {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        
        // Also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
