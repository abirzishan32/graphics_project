#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "basic_camera.h"

#include <iostream>
#include <vector>
#include <cmath>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int createCubeVAO();
unsigned int createQuadVAO();
unsigned int createCylinderVAO(int segments);
unsigned int createWedgeVAO();
void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawCylinder(Shader& shader, unsigned int VAO, int segments, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawQuad(Shader& shader, unsigned int VAO, glm::mat4 model, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void setupLighting(Shader& shader);
void drawParkingLot(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO, unsigned int wedgeVAO);
void drawRealisticCar(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, unsigned int wedgeVAO, glm::vec3 position, glm::vec3 carColor, float rotation);
void drawTree(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, glm::vec3 position, float height, float spread);
void drawOutdoorEnvironment(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO);
void drawLightRays(Shader& shader, unsigned int cubeVAO);
void drawStopSign(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float rotation);
void drawSafetyBollard(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, glm::vec3 position);
void drawLightPole(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, glm::vec3 position, float height);
void drawStopStencil(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float rotation);
void drawDirectionalArrow(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float rotation);
void drawSpeedBump(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float width);
void drawParkingSignage(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO);

// Settings
const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 900;

// Parking lot dimensions (meters)
const float LOT_WIDTH = 60.0f;
const float LOT_DEPTH = 40.0f;
const float CEILING_HEIGHT = 3.5f;
const float WALL_THICKNESS = 0.3f;
const float PILLAR_SIZE = 0.5f;
const float SPOT_WIDTH = 2.5f;
const float SPOT_DEPTH = 5.0f;
const float LANE_WIDTH = 6.0f;

// Camera - starts inside the parking lot at pedestrian height
BasicCamera camera(5.0f, 1.7f, LOT_DEPTH/2.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Lights toggle (L key to toggle)
bool lightsOn = true;
bool lKeyPressed = false;

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Parking Lot - Phong Illumination", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable depth testing and blending for light rays
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Build and compile shader program
    Shader shader("pbrVertex.vs", "pbrFragment.fs");

    // Create geometry
    unsigned int cubeVAO = createCubeVAO();
    unsigned int quadVAO = createQuadVAO();
    unsigned int cylVAO = createCylinderVAO(16);
    unsigned int wedgeVAO = createWedgeVAO();

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Clear screen with bright sky blue (daylight outside)
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Set view/projection matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camera.Position);

        // Setup lighting
        setupLighting(shader);

        // Draw outdoor environment first
        drawOutdoorEnvironment(shader, cubeVAO, quadVAO, cylVAO);

        // Draw the parking lot
        drawParkingLot(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);
        
        // Draw parking lot signage and safety features
        drawParkingSignage(shader, cubeVAO, cylVAO);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteVertexArrays(1, &cylVAO);
    glDeleteVertexArrays(1, &wedgeVAO);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
    // L key to toggle lights on/off
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!lKeyPressed) {
            lightsOn = !lightsOn;
            lKeyPressed = true;
        }
    } else {
        lKeyPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int createCubeVAO()
{
    float vertices[] = {
        // positions          // normals           // texture coords
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    return VAO;
}

unsigned int createQuadVAO()
{
    float vertices[] = {
        // positions          // normals        // tex coords
        -0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
         0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    return VAO;
}

unsigned int createCylinderVAO(int segments)
{
    std::vector<float> vertices;
    float PI = 3.14159265359f;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = 2.0f * PI * i / segments;
        float angle2 = 2.0f * PI * (i + 1) / segments;
        float x1 = cos(angle1), z1 = sin(angle1);
        float x2 = cos(angle2), z2 = sin(angle2);
        
        // Side quad (2 triangles)
        // Triangle 1
        vertices.insert(vertices.end(), {x1*0.5f, -0.5f, z1*0.5f, x1, 0.0f, z1, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {x2*0.5f, -0.5f, z2*0.5f, x2, 0.0f, z2, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {x2*0.5f,  0.5f, z2*0.5f, x2, 0.0f, z2, 1.0f, 1.0f});
        // Triangle 2
        vertices.insert(vertices.end(), {x2*0.5f,  0.5f, z2*0.5f, x2, 0.0f, z2, 1.0f, 1.0f});
        vertices.insert(vertices.end(), {x1*0.5f,  0.5f, z1*0.5f, x1, 0.0f, z1, 0.0f, 1.0f});
        vertices.insert(vertices.end(), {x1*0.5f, -0.5f, z1*0.5f, x1, 0.0f, z1, 0.0f, 0.0f});
        
        // Top cap
        vertices.insert(vertices.end(), {0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f});
        vertices.insert(vertices.end(), {x1*0.5f, 0.5f, z1*0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f});
        vertices.insert(vertices.end(), {x2*0.5f, 0.5f, z2*0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f});
        
        // Bottom cap
        vertices.insert(vertices.end(), {0.0f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f});
        vertices.insert(vertices.end(), {x2*0.5f, -0.5f, z2*0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f});
        vertices.insert(vertices.end(), {x1*0.5f, -0.5f, z1*0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f});
    }
    
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    return VAO;
}

unsigned int createWedgeVAO()
{
    // Wedge/ramp shape for car hood/trunk slopes
    float vertices[] = {
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        // Sloped top face (back high, front low)
        -0.5f, -0.5f,  0.5f,  0.0f, 0.707f, 0.707f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.707f, 0.707f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.707f, 0.707f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.707f, 0.707f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.707f, 0.707f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.707f, 0.707f,  0.0f, 0.0f,
        // Back face (high)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        // Left side
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        // Right side
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
    };
    
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    return VAO;
}

void setupLighting(Shader& shader)
{
    // Pass light toggle state to shader
    shader.setBool("lightsOn", lightsOn);
    
    // Window positions for volumetric scattering (4 windows)
    shader.setInt("numWindows", 4);
    float windowSpacing = 8.0f;
    for (int i = 0; i < 4; i++) {
        float z = windowSpacing/2 + i * windowSpacing;
        std::string base = "windowPositions[" + std::to_string(i) + "]";
        shader.setVec3(base, glm::vec3(0.0f, 2.5f, z));
    }
    shader.setVec3("sunDirection", glm::normalize(glm::vec3(1.0f, -0.5f, 0.2f)));
    
    // Directional light (sunlight through windows) - ALWAYS ON for ambient window light
    shader.setBool("useDirLight", true);
    shader.setVec3("dirLight.direction", glm::vec3(-0.5f, -0.7f, -0.3f));
    
    if (lightsOn) {
        // Full lighting when lights are on
        shader.setVec3("dirLight.ambient", glm::vec3(0.25f, 0.22f, 0.18f));
        shader.setVec3("dirLight.diffuse", glm::vec3(1.2f, 1.1f, 0.9f));
        shader.setVec3("dirLight.specular", glm::vec3(1.2f, 1.15f, 1.0f));
        
        // Point lights (ceiling-mounted fixtures) - INCREASED INTENSITY
        int numLights = 12;
        shader.setInt("numPointLights", numLights);
        
        float lightSpacingX = LOT_WIDTH / 4.0f;
        float lightSpacingZ = LOT_DEPTH / 3.0f;
        int lightIdx = 0;
        
        for (int i = 0; i < 4 && lightIdx < numLights; i++) {
            for (int j = 0; j < 3 && lightIdx < numLights; j++) {
                float x = lightSpacingX * (i + 0.5f);
                float z = lightSpacingZ * (j + 0.5f);
                
                std::string base = "pointLights[" + std::to_string(lightIdx) + "]";
                shader.setVec3(base + ".position", glm::vec3(x, CEILING_HEIGHT - 0.3f, z));
                shader.setVec3(base + ".ambient", glm::vec3(0.18f, 0.16f, 0.12f));
                shader.setVec3(base + ".diffuse", glm::vec3(1.4f, 1.3f, 1.1f));
                shader.setVec3(base + ".specular", glm::vec3(1.3f, 1.2f, 1.0f));
                shader.setFloat(base + ".constant", 1.0f);
                shader.setFloat(base + ".linear", 0.045f);
                shader.setFloat(base + ".quadratic", 0.0075f);
                lightIdx++;
            }
        }
    } else {
        // Lights OFF - only very dim ambient from windows
        shader.setVec3("dirLight.ambient", glm::vec3(0.03f, 0.035f, 0.04f));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.15f, 0.14f, 0.12f));
        shader.setVec3("dirLight.specular", glm::vec3(0.1f, 0.1f, 0.08f));
        
        // Turn off point lights completely
        shader.setInt("numPointLights", 0);
    }
}

void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);
    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);
    shader.setInt("textureType", texType);
    shader.setFloat("ambientStrength", ambient);
    shader.setFloat("diffuseStrength", diffuse);
    shader.setFloat("specularStrength", specular);
    shader.setFloat("shininess", shininess);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);
    shader.setInt("textureType", texType);
    shader.setFloat("ambientStrength", ambient);
    shader.setFloat("diffuseStrength", diffuse);
    shader.setFloat("specularStrength", specular);
    shader.setFloat("shininess", shininess);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawCylinder(Shader& shader, unsigned int VAO, int segments, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate to lay flat
    model = glm::scale(model, scale);
    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);
    shader.setInt("textureType", texType);
    shader.setFloat("ambientStrength", ambient);
    shader.setFloat("diffuseStrength", diffuse);
    shader.setFloat("specularStrength", specular);
    shader.setFloat("shininess", shininess);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, segments * 12);
}

void drawQuad(Shader& shader, unsigned int VAO, glm::mat4 model, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess)
{
    shader.setMat4("model", model);
    shader.setVec3("objectColor", color);
    shader.setInt("textureType", texType);
    shader.setFloat("ambientStrength", ambient);
    shader.setFloat("diffuseStrength", diffuse);
    shader.setFloat("specularStrength", specular);
    shader.setFloat("shininess", shininess);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawRealisticCar(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, unsigned int wedgeVAO, 
                      glm::vec3 position, glm::vec3 carColor, float rotation)
{

    
    float baseY = position.y;
    float wheelRadius = 0.325f;
    float wheelWidth = 0.22f;
    float wheelbase = 2.7f;
    float bodyWidth = 1.8f;
    float bodyLength = 4.5f;
    
    // Wheel positions (centered on axles)
    float frontAxleZ = position.z + wheelbase/2 - 0.3f;
    float rearAxleZ = position.z - wheelbase/2 + 0.3f;
    float wheelY = baseY + wheelRadius;
    float wheelX = bodyWidth/2 - 0.1f;
    
    // === WHEELS (Cylinders for realism) ===
    glm::vec3 tireColor(0.15f, 0.15f, 0.15f);
    glm::vec3 rimColor(0.7f, 0.7f, 0.75f);
    
    // Front left wheel
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x - wheelX, wheelY, frontAxleZ),
                 glm::vec3(wheelWidth, wheelRadius*2, wheelRadius*2), tireColor, 3, 0.1f, 0.5f, 0.2f, 8.0f);
    // Rim
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x - wheelX - 0.02f, wheelY, frontAxleZ),
                 glm::vec3(0.05f, wheelRadius*1.4f, wheelRadius*1.4f), rimColor, 3, 0.2f, 0.7f, 0.9f, 64.0f);
    
    // Front right wheel
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x + wheelX, wheelY, frontAxleZ),
                 glm::vec3(wheelWidth, wheelRadius*2, wheelRadius*2), tireColor, 3, 0.1f, 0.5f, 0.2f, 8.0f);
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x + wheelX + 0.02f, wheelY, frontAxleZ),
                 glm::vec3(0.05f, wheelRadius*1.4f, wheelRadius*1.4f), rimColor, 3, 0.2f, 0.7f, 0.9f, 64.0f);
    
    // Rear left wheel
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x - wheelX, wheelY, rearAxleZ),
                 glm::vec3(wheelWidth, wheelRadius*2, wheelRadius*2), tireColor, 3, 0.1f, 0.5f, 0.2f, 8.0f);
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x - wheelX - 0.02f, wheelY, rearAxleZ),
                 glm::vec3(0.05f, wheelRadius*1.4f, wheelRadius*1.4f), rimColor, 3, 0.2f, 0.7f, 0.9f, 64.0f);
    
    // Rear right wheel
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x + wheelX, wheelY, rearAxleZ),
                 glm::vec3(wheelWidth, wheelRadius*2, wheelRadius*2), tireColor, 3, 0.1f, 0.5f, 0.2f, 8.0f);
    drawCylinder(shader, cylVAO, 16, glm::vec3(position.x + wheelX + 0.02f, wheelY, rearAxleZ),
                 glm::vec3(0.05f, wheelRadius*1.4f, wheelRadius*1.4f), rimColor, 3, 0.2f, 0.7f, 0.9f, 64.0f);
    
    // === LOWER BODY (main body shell) ===
    float lowerBodyHeight = 0.45f;
    float lowerBodyY = baseY + wheelRadius + lowerBodyHeight/2;
    
    // Main lower body
    drawCube(shader, cubeVAO, glm::vec3(position.x, lowerBodyY, position.z),
             glm::vec3(bodyWidth, lowerBodyHeight, bodyLength - 0.4f), carColor, 2, 0.15f, 0.8f, 0.9f, 64.0f);
    
    // Front bumper
    drawCube(shader, cubeVAO, glm::vec3(position.x, lowerBodyY - 0.1f, position.z + bodyLength/2 - 0.15f),
             glm::vec3(bodyWidth + 0.05f, 0.25f, 0.3f), carColor * 0.95f, 2, 0.15f, 0.8f, 0.85f, 48.0f);
    
    // Rear bumper
    drawCube(shader, cubeVAO, glm::vec3(position.x, lowerBodyY - 0.1f, position.z - bodyLength/2 + 0.15f),
             glm::vec3(bodyWidth + 0.05f, 0.25f, 0.3f), carColor * 0.95f, 2, 0.15f, 0.8f, 0.85f, 48.0f);
    
    // === WHEEL ARCHES (fenders) ===
    glm::vec3 archColor = carColor * 0.92f;
    // Front left
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x - bodyWidth/2 + 0.08f, wheelY + 0.2f, frontAxleZ),
             glm::vec3(0.15f, 0.4f, 0.7f), glm::vec3(0, 0, 0), archColor, 2, 0.15f, 0.75f, 0.8f, 48.0f);
    // Front right
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + bodyWidth/2 - 0.08f, wheelY + 0.2f, frontAxleZ),
             glm::vec3(0.15f, 0.4f, 0.7f), glm::vec3(0, 0, 0), archColor, 2, 0.15f, 0.75f, 0.8f, 48.0f);
    // Rear left
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x - bodyWidth/2 + 0.08f, wheelY + 0.2f, rearAxleZ),
             glm::vec3(0.15f, 0.4f, 0.7f), glm::vec3(0, 0, 0), archColor, 2, 0.15f, 0.75f, 0.8f, 48.0f);
    // Rear right
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + bodyWidth/2 - 0.08f, wheelY + 0.2f, rearAxleZ),
             glm::vec3(0.15f, 0.4f, 0.7f), glm::vec3(0, 0, 0), archColor, 2, 0.15f, 0.75f, 0.8f, 48.0f);
    
    // === HOOD (sloped front) ===
    float hoodY = lowerBodyY + lowerBodyHeight/2 + 0.1f;
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, hoodY, position.z + 1.2f),
             glm::vec3(bodyWidth - 0.1f, 0.15f, 1.3f), glm::vec3(-8, 0, 0), carColor, 2, 0.15f, 0.85f, 0.95f, 80.0f);
    
    // === CABIN (greenhouse) ===
    float cabinWidth = bodyWidth - 0.25f;
    float cabinHeight = 0.55f;
    float cabinLength = 1.8f;
    float cabinY = hoodY + cabinHeight/2 + 0.05f;
    float cabinZ = position.z - 0.2f;
    
    // A-pillar slope (windshield frame)
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, cabinY, cabinZ + cabinLength/2 - 0.1f),
             glm::vec3(cabinWidth, cabinHeight, 0.1f), glm::vec3(-25, 0, 0), carColor * 0.85f, 2, 0.12f, 0.7f, 0.8f, 48.0f);
    
    // Main cabin
    drawCube(shader, cubeVAO, glm::vec3(position.x, cabinY, cabinZ),
             glm::vec3(cabinWidth, cabinHeight, cabinLength - 0.3f), carColor * 0.9f, 2, 0.12f, 0.75f, 0.85f, 56.0f);
    
    // C-pillar (rear)
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, cabinY, cabinZ - cabinLength/2 + 0.1f),
             glm::vec3(cabinWidth, cabinHeight, 0.1f), glm::vec3(20, 0, 0), carColor * 0.85f, 2, 0.12f, 0.7f, 0.8f, 48.0f);
    
    // === WINDOWS (glass) ===
    glm::vec3 glassColor(0.08f, 0.12f, 0.18f);
    // Windshield
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, cabinY + 0.05f, cabinZ + cabinLength/2 - 0.2f),
             glm::vec3(cabinWidth - 0.15f, cabinHeight - 0.15f, 0.05f), glm::vec3(-30, 0, 0), 
             glassColor, 4, 0.05f, 0.2f, 1.0f, 128.0f);
    // Rear window
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, cabinY + 0.05f, cabinZ - cabinLength/2 + 0.25f),
             glm::vec3(cabinWidth - 0.15f, cabinHeight - 0.15f, 0.05f), glm::vec3(25, 0, 0),
             glassColor, 4, 0.05f, 0.2f, 1.0f, 128.0f);
    // Side windows
    drawCube(shader, cubeVAO, glm::vec3(position.x - cabinWidth/2 + 0.02f, cabinY + 0.08f, cabinZ),
             glm::vec3(0.03f, cabinHeight - 0.2f, cabinLength - 0.5f), glassColor, 4, 0.05f, 0.2f, 1.0f, 128.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + cabinWidth/2 - 0.02f, cabinY + 0.08f, cabinZ),
             glm::vec3(0.03f, cabinHeight - 0.2f, cabinLength - 0.5f), glassColor, 4, 0.05f, 0.2f, 1.0f, 128.0f);
    
    // === TRUNK (rear deck) ===
    float trunkY = lowerBodyY + lowerBodyHeight/2 + 0.08f;
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, trunkY, position.z - 1.5f),
             glm::vec3(bodyWidth - 0.1f, 0.12f, 1.0f), glm::vec3(5, 0, 0), carColor, 2, 0.15f, 0.85f, 0.95f, 80.0f);
    
    // === HEADLIGHTS ===
    glm::vec3 headlightColor(0.95f, 0.95f, 0.85f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - 0.55f, lowerBodyY + 0.1f, position.z + bodyLength/2 - 0.02f),
             glm::vec3(0.35f, 0.18f, 0.08f), headlightColor, 4, 0.7f, 0.4f, 1.0f, 96.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.55f, lowerBodyY + 0.1f, position.z + bodyLength/2 - 0.02f),
             glm::vec3(0.35f, 0.18f, 0.08f), headlightColor, 4, 0.7f, 0.4f, 1.0f, 96.0f);
    
    // === TAILLIGHTS ===
    glm::vec3 taillightColor(0.85f, 0.1f, 0.1f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - 0.6f, lowerBodyY + 0.1f, position.z - bodyLength/2 + 0.02f),
             glm::vec3(0.3f, 0.12f, 0.06f), taillightColor, 4, 0.5f, 0.5f, 0.8f, 48.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.6f, lowerBodyY + 0.1f, position.z - bodyLength/2 + 0.02f),
             glm::vec3(0.3f, 0.12f, 0.06f), taillightColor, 4, 0.5f, 0.5f, 0.8f, 48.0f);
    
    // === GRILLE ===
    drawCube(shader, cubeVAO, glm::vec3(position.x, lowerBodyY, position.z + bodyLength/2 - 0.02f),
             glm::vec3(0.6f, 0.2f, 0.05f), glm::vec3(0.12f, 0.12f, 0.14f), 3, 0.1f, 0.4f, 0.6f, 32.0f);
    
    // === SIDE MIRRORS ===
    drawCube(shader, cubeVAO, glm::vec3(position.x - bodyWidth/2 - 0.08f, cabinY - 0.1f, cabinZ + 0.6f),
             glm::vec3(0.08f, 0.06f, 0.12f), carColor * 0.9f, 2, 0.15f, 0.7f, 0.8f, 48.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + bodyWidth/2 + 0.08f, cabinY - 0.1f, cabinZ + 0.6f),
             glm::vec3(0.08f, 0.06f, 0.12f), carColor * 0.9f, 2, 0.15f, 0.7f, 0.8f, 48.0f);
    
    // === DOOR HANDLES ===
    drawCube(shader, cubeVAO, glm::vec3(position.x - bodyWidth/2 + 0.02f, lowerBodyY + 0.15f, cabinZ + 0.3f),
             glm::vec3(0.02f, 0.03f, 0.12f), glm::vec3(0.7f, 0.7f, 0.72f), 3, 0.2f, 0.6f, 0.9f, 64.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + bodyWidth/2 - 0.02f, lowerBodyY + 0.15f, cabinZ + 0.3f),
             glm::vec3(0.02f, 0.03f, 0.12f), glm::vec3(0.7f, 0.7f, 0.72f), 3, 0.2f, 0.6f, 0.9f, 64.0f);
}


void drawParkingLot(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO, unsigned int wedgeVAO)
{
    glm::vec3 concreteColor(0.45f, 0.43f, 0.40f);
    glm::vec3 ceilingColor(0.35f, 0.33f, 0.30f);
    glm::vec3 lineColor(0.95f, 0.95f, 0.90f);
    glm::vec3 yellowColor(0.95f, 0.85f, 0.2f);
    
    // === FLOOR ===
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(LOT_WIDTH/2, 0.0f, LOT_DEPTH/2));
    floorModel = glm::scale(floorModel, glm::vec3(LOT_WIDTH, 1.0f, LOT_DEPTH));
    drawQuad(shader, quadVAO, floorModel, concreteColor, 0, 0.15f, 0.7f, 0.2f, 16.0f);
    
    // === CEILING ===
    glm::mat4 ceilingModel = glm::mat4(1.0f);
    ceilingModel = glm::translate(ceilingModel, glm::vec3(LOT_WIDTH/2, CEILING_HEIGHT, LOT_DEPTH/2));
    ceilingModel = glm::rotate(ceilingModel, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ceilingModel = glm::scale(ceilingModel, glm::vec3(LOT_WIDTH, 1.0f, LOT_DEPTH));
    drawQuad(shader, quadVAO, ceilingModel, ceilingColor, 0, 0.1f, 0.5f, 0.1f, 8.0f);
    
    // === WALLS ===
    // Back wall (Z = 0)
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, CEILING_HEIGHT/2, -WALL_THICKNESS/2),
             glm::vec3(LOT_WIDTH, CEILING_HEIGHT, WALL_THICKNESS), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    
    // Front wall with entrance opening
    float entranceWidth = 4.0f;
    float entranceStart = LOT_WIDTH/2 - entranceWidth/2;
    // Left section
    drawCube(shader, cubeVAO, glm::vec3(entranceStart/2, CEILING_HEIGHT/2, LOT_DEPTH + WALL_THICKNESS/2),
             glm::vec3(entranceStart, CEILING_HEIGHT, WALL_THICKNESS), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    // Right section
    float rightStart = LOT_WIDTH/2 + entranceWidth/2;
    float rightWidth = LOT_WIDTH - rightStart;
    drawCube(shader, cubeVAO, glm::vec3(rightStart + rightWidth/2, CEILING_HEIGHT/2, LOT_DEPTH + WALL_THICKNESS/2),
             glm::vec3(rightWidth, CEILING_HEIGHT, WALL_THICKNESS), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    // Top section above entrance
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, CEILING_HEIGHT - 0.5f, LOT_DEPTH + WALL_THICKNESS/2),
             glm::vec3(entranceWidth + 0.5f, 1.0f, WALL_THICKNESS), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    
    // Left wall with windows
    float windowHeight = 1.5f;
    float windowWidth = 3.0f;
    float windowSpacing = 8.0f;
    // Lower section
    drawCube(shader, cubeVAO, glm::vec3(-WALL_THICKNESS/2, 1.0f, LOT_DEPTH/2),
             glm::vec3(WALL_THICKNESS, 2.0f, LOT_DEPTH), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    // Upper section
    drawCube(shader, cubeVAO, glm::vec3(-WALL_THICKNESS/2, CEILING_HEIGHT - 0.3f, LOT_DEPTH/2),
             glm::vec3(WALL_THICKNESS, 0.6f, LOT_DEPTH), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    // Wall pillars between windows
    for (float z = 5.0f; z < LOT_DEPTH; z += windowSpacing) {
        drawCube(shader, cubeVAO, glm::vec3(-WALL_THICKNESS/2, 2.5f, z),
                 glm::vec3(WALL_THICKNESS + 0.1f, 1.6f, 1.0f), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    }
    
    // Right wall (solid)
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH + WALL_THICKNESS/2, CEILING_HEIGHT/2, LOT_DEPTH/2),
             glm::vec3(WALL_THICKNESS, CEILING_HEIGHT, LOT_DEPTH), concreteColor, 0, 0.12f, 0.6f, 0.15f, 12.0f);
    
    // === SUPPORT PILLARS ===
    float pillarSpacingX = 12.0f;
    float pillarSpacingZ = 10.0f;
    for (float x = pillarSpacingX; x < LOT_WIDTH - 1.0f; x += pillarSpacingX) {
        for (float z = pillarSpacingZ; z < LOT_DEPTH - 1.0f; z += pillarSpacingZ) {
            drawCube(shader, cubeVAO, glm::vec3(x, CEILING_HEIGHT/2, z),
                     glm::vec3(PILLAR_SIZE, CEILING_HEIGHT, PILLAR_SIZE), 
                     concreteColor * 0.9f, 0, 0.1f, 0.65f, 0.2f, 16.0f);
        }
    }
    
    // === PARKING LINES ===
    float lineThickness = 0.1f;
    float lineHeight = 0.01f;
    // Left row of spots
    float spotStartX = 2.0f;
    for (int i = 0; i < 8; i++) {
        float z = 4.0f + i * (SPOT_DEPTH + 0.5f);
        // Perpendicular lines
        drawCube(shader, cubeVAO, glm::vec3(spotStartX, lineHeight, z),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(spotStartX, lineHeight, z + SPOT_DEPTH),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        // Side lines
        drawCube(shader, cubeVAO, glm::vec3(spotStartX - SPOT_WIDTH/2, lineHeight, z + SPOT_DEPTH/2),
                 glm::vec3(0.15f, lineThickness, SPOT_DEPTH), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(spotStartX + SPOT_WIDTH/2, lineHeight, z + SPOT_DEPTH/2),
                 glm::vec3(0.15f, lineThickness, SPOT_DEPTH), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
    }
    
    // Right row of spots
    float spotStartX2 = LOT_WIDTH - 2.0f;
    for (int i = 0; i < 8; i++) {
        float z = 4.0f + i * (SPOT_DEPTH + 0.5f);
        drawCube(shader, cubeVAO, glm::vec3(spotStartX2, lineHeight, z),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(spotStartX2, lineHeight, z + SPOT_DEPTH),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(spotStartX2 - SPOT_WIDTH/2, lineHeight, z + SPOT_DEPTH/2),
                 glm::vec3(0.15f, lineThickness, SPOT_DEPTH), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(spotStartX2 + SPOT_WIDTH/2, lineHeight, z + SPOT_DEPTH/2),
                 glm::vec3(0.15f, lineThickness, SPOT_DEPTH), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
    }
    
    // Middle double row
    float middleX = LOT_WIDTH/2;
    for (int i = 0; i < 6; i++) {
        float z = 6.0f + i * (SPOT_DEPTH + 0.5f);
        // Left side spots
        float leftX = middleX - SPOT_WIDTH/2 - 0.5f;
        drawCube(shader, cubeVAO, glm::vec3(leftX, lineHeight, z),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(leftX, lineHeight, z + SPOT_DEPTH),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        // Right side spots
        float rightX = middleX + SPOT_WIDTH/2 + 0.5f;
        drawCube(shader, cubeVAO, glm::vec3(rightX, lineHeight, z),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
        drawCube(shader, cubeVAO, glm::vec3(rightX, lineHeight, z + SPOT_DEPTH),
                 glm::vec3(SPOT_WIDTH, lineThickness, 0.15f), lineColor, 1, 0.3f, 0.8f, 0.3f, 16.0f);
    }
    
    // === DIRECTIONAL ARROWS ===
    // Arrow in main lane pointing forward
    float arrowZ = LOT_DEPTH - 8.0f;
    float arrowX = LOT_WIDTH/2;
    // Arrow body
    drawCube(shader, cubeVAO, glm::vec3(arrowX, lineHeight, arrowZ),
             glm::vec3(0.4f, lineThickness, 2.0f), yellowColor, 1, 0.4f, 0.9f, 0.5f, 32.0f);
    // Arrow head (triangular using cubes)
    drawCube(shader, cubeVAO, glm::vec3(arrowX, lineHeight, arrowZ - 1.3f),
             glm::vec3(1.0f, lineThickness, 0.6f), yellowColor, 1, 0.4f, 0.9f, 0.5f, 32.0f);
    drawCube(shader, cubeVAO, glm::vec3(arrowX, lineHeight, arrowZ - 1.7f),
             glm::vec3(0.6f, lineThickness, 0.4f), yellowColor, 1, 0.4f, 0.9f, 0.5f, 32.0f);
    
    // === BOOM BARRIER ===
    float barrierX = LOT_WIDTH/2;
    float barrierZ = LOT_DEPTH - 2.0f;
    // Post
    drawCube(shader, cubeVAO, glm::vec3(barrierX + 2.5f, 0.6f, barrierZ),
             glm::vec3(0.25f, 1.2f, 0.25f), glm::vec3(0.3f, 0.3f, 0.35f), 3, 0.15f, 0.7f, 0.6f, 32.0f);
    // Control box
    drawCube(shader, cubeVAO, glm::vec3(barrierX + 2.5f, 1.0f, barrierZ),
             glm::vec3(0.4f, 0.5f, 0.3f), glm::vec3(0.2f, 0.2f, 0.25f), 3, 0.15f, 0.6f, 0.4f, 24.0f);
    // Arm (raised)
    drawCube(shader, cubeVAO, glm::vec3(barrierX + 0.5f, 1.3f, barrierZ),
             glm::vec3(3.5f, 0.08f, 0.12f), glm::vec3(0.9f, 0.2f, 0.2f), 1, 0.3f, 0.8f, 0.4f, 16.0f);
    // Stripes on arm
    for (int s = 0; s < 4; s++) {
        drawCube(shader, cubeVAO, glm::vec3(barrierX - 0.5f + s * 0.8f, 1.31f, barrierZ),
                 glm::vec3(0.3f, 0.09f, 0.13f), lineColor, 1, 0.4f, 0.9f, 0.5f, 24.0f);
    }
    
    // === TICKET MACHINE ===
    float machineX = LOT_WIDTH/2 - 3.0f;
    float machineZ = LOT_DEPTH - 2.0f;
    drawCube(shader, cubeVAO, glm::vec3(machineX, 0.7f, machineZ),
             glm::vec3(0.6f, 1.4f, 0.4f), glm::vec3(0.25f, 0.25f, 0.3f), 3, 0.15f, 0.7f, 0.5f, 32.0f);
    // Screen
    drawCube(shader, cubeVAO, glm::vec3(machineX, 1.1f, machineZ + 0.21f),
             glm::vec3(0.35f, 0.25f, 0.02f), glm::vec3(0.1f, 0.4f, 0.5f), 4, 0.5f, 0.4f, 0.9f, 64.0f);
    
    // === CEILING LIGHT FIXTURES ===
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            float x = (LOT_WIDTH/4) * (i + 0.5f);
            float z = (LOT_DEPTH/3) * (j + 0.5f);
            // Light housing
            drawCube(shader, cubeVAO, glm::vec3(x, CEILING_HEIGHT - 0.15f, z),
                     glm::vec3(1.2f, 0.1f, 0.4f), glm::vec3(0.8f, 0.8f, 0.82f), 3, 0.2f, 0.6f, 0.7f, 48.0f);
            // Light panel (emissive look)
            drawCube(shader, cubeVAO, glm::vec3(x, CEILING_HEIGHT - 0.22f, z),
                     glm::vec3(1.0f, 0.05f, 0.3f), glm::vec3(1.0f, 0.98f, 0.9f), 4, 0.9f, 0.3f, 0.2f, 8.0f);
        }
    }
    
    // === PARKED CARS ===
    // Various colors for realistic variety
    glm::vec3 carColors[] = {
        glm::vec3(0.15f, 0.15f, 0.18f),  // Dark gray
        glm::vec3(0.8f, 0.1f, 0.1f),     // Red
        glm::vec3(0.9f, 0.9f, 0.92f),    // White/silver
        glm::vec3(0.1f, 0.2f, 0.5f),     // Blue
        glm::vec3(0.2f, 0.2f, 0.22f),    // Black
        glm::vec3(0.6f, 0.55f, 0.5f)     // Beige
    };
    
    // Cars in left row
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX, 0.0f, 6.5f), carColors[0], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX, 0.0f, 12.0f), carColors[1], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX, 0.0f, 23.0f), carColors[2], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX, 0.0f, 28.5f), carColors[3], 0);
    
    // Cars in right row  
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX2, 0.0f, 6.5f), carColors[4], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX2, 0.0f, 17.5f), carColors[5], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(spotStartX2, 0.0f, 28.5f), carColors[0], 0);
    
    // Cars in middle row
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(middleX - SPOT_WIDTH/2 - 0.5f, 0.0f, 8.5f), carColors[2], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(middleX + SPOT_WIDTH/2 + 0.5f, 0.0f, 14.0f), carColors[1], 0);
    drawRealisticCar(shader, cubeVAO, cylVAO, wedgeVAO, glm::vec3(middleX - SPOT_WIDTH/2 - 0.5f, 0.0f, 25.0f), carColors[4], 0);
    
    // === EXIT/ENTRANCE SIGNS ===
    // Exit sign above entrance
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 2.8f, LOT_DEPTH - 0.5f),
             glm::vec3(1.5f, 0.4f, 0.1f), glm::vec3(0.1f, 0.6f, 0.2f), 1, 0.6f, 0.5f, 0.3f, 16.0f);
}

// Draw a hyper-realistic tree with branching trunk and organic foliage clusters
void drawTree(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, glm::vec3 position, float height, float spread)
{
    // === BARK/TRUNK COLORS (realistic brown tones like reference) ===
    glm::vec3 barkDark(0.25f, 0.15f, 0.08f);   // Dark brown bark
    glm::vec3 barkMid(0.35f, 0.22f, 0.12f);    // Medium brown
    glm::vec3 barkLight(0.42f, 0.28f, 0.15f);  // Lighter brown for highlights
    
    // === FOLIAGE COLORS (vibrant greens like reference) ===
    glm::vec3 leafDark(0.12f, 0.35f, 0.08f);   // Dark inner foliage
    glm::vec3 leafMid(0.18f, 0.48f, 0.12f);    // Main foliage color
    glm::vec3 leafLight(0.28f, 0.58f, 0.18f);  // Sunlit foliage
    glm::vec3 leafBright(0.35f, 0.65f, 0.22f); // Brightest highlights
    
    float trunkHeight = height * 0.35f;
    float trunkRadius = height * 0.06f;
    
    // === MAIN TRUNK (with texture variation) ===
    // Slightly tapered - wider at base
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + trunkHeight * 0.25f, position.z),
             glm::vec3(trunkRadius * 2.2f, trunkHeight * 0.5f, trunkRadius * 2.2f), barkDark, 0, 0.1f, 0.55f, 0.1f, 6.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + trunkHeight * 0.7f, position.z),
             glm::vec3(trunkRadius * 1.8f, trunkHeight * 0.4f, trunkRadius * 1.8f), barkMid, 0, 0.1f, 0.55f, 0.1f, 6.0f);
    
    // Trunk texture/bark detail variation
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + trunkRadius * 0.3f, position.y + trunkHeight * 0.4f, position.z),
             glm::vec3(trunkRadius * 0.5f, trunkHeight * 0.3f, trunkRadius * 0.4f), glm::vec3(0, 15, 0),
             barkLight, 0, 0.1f, 0.5f, 0.1f, 6.0f);
    
    // === PRIMARY BRANCHES (split from trunk like in reference) ===
    float branchY = position.y + trunkHeight * 0.8f;
    float branchLen = height * 0.25f;
    float branchRad = trunkRadius * 0.6f;
    
    // Branch 1 - going up-right
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + branchLen * 0.3f, branchY + branchLen * 0.3f, position.z - spread * 0.1f),
             glm::vec3(branchLen, branchRad * 1.5f, branchRad * 1.5f), glm::vec3(0, 0, -40),
             barkMid, 0, 0.1f, 0.55f, 0.1f, 6.0f);
    
    // Branch 2 - going up-left
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x - branchLen * 0.25f, branchY + branchLen * 0.35f, position.z + spread * 0.15f),
             glm::vec3(branchLen * 0.9f, branchRad * 1.4f, branchRad * 1.4f), glm::vec3(0, 0, 35),
             barkDark, 0, 0.1f, 0.55f, 0.1f, 6.0f);
    
    // Branch 3 - going forward
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, branchY + branchLen * 0.2f, position.z + branchLen * 0.3f),
             glm::vec3(branchRad * 1.3f, branchRad * 1.3f, branchLen * 0.8f), glm::vec3(30, 0, 0),
             barkMid, 0, 0.1f, 0.55f, 0.1f, 6.0f);
    
    // Branch 4 - going backward
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + spread * 0.1f, branchY + branchLen * 0.15f, position.z - branchLen * 0.35f),
             glm::vec3(branchRad * 1.2f, branchRad * 1.2f, branchLen * 0.7f), glm::vec3(-25, 10, 0),
             barkLight, 0, 0.1f, 0.55f, 0.1f, 6.0f);
    
    // === SECONDARY BRANCHES (smaller, extending outward) ===
    float secBranchLen = branchLen * 0.5f;
    float secBranchRad = branchRad * 0.5f;
    
    // Multiple smaller branches
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + spread * 0.4f, branchY + height * 0.15f, position.z),
             glm::vec3(secBranchLen, secBranchRad, secBranchRad), glm::vec3(0, 0, -50),
             barkDark, 0, 0.1f, 0.5f, 0.1f, 6.0f);
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x - spread * 0.35f, branchY + height * 0.18f, position.z + spread * 0.2f),
             glm::vec3(secBranchLen * 0.9f, secBranchRad, secBranchRad), glm::vec3(0, 20, 45),
             barkMid, 0, 0.1f, 0.5f, 0.1f, 6.0f);
    
    // === FOLIAGE CLUSTERS (organic, irregular shapes like reference) ===
    float foliageY = branchY + branchLen * 0.2f;
    float clusterSize = spread * 0.45f;
    
    // Main central canopy cluster
    drawCube(shader, cubeVAO, glm::vec3(position.x, foliageY + height * 0.2f, position.z),
             glm::vec3(clusterSize * 1.4f, clusterSize * 0.9f, clusterSize * 1.4f), leafMid, 0, 0.18f, 0.7f, 0.08f, 4.0f);
    
    // Overlapping clusters for organic look
    drawCube(shader, cubeVAO, glm::vec3(position.x + spread * 0.35f, foliageY + height * 0.12f, position.z - spread * 0.1f),
             glm::vec3(clusterSize * 1.1f, clusterSize * 0.7f, clusterSize * 1.0f), leafLight, 0, 0.2f, 0.72f, 0.08f, 4.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - spread * 0.3f, foliageY + height * 0.15f, position.z + spread * 0.25f),
             glm::vec3(clusterSize * 1.2f, clusterSize * 0.75f, clusterSize * 1.1f), leafDark, 0, 0.15f, 0.65f, 0.08f, 4.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + spread * 0.1f, foliageY + height * 0.28f, position.z - spread * 0.2f),
             glm::vec3(clusterSize * 0.9f, clusterSize * 0.65f, clusterSize * 0.9f), leafBright, 0, 0.22f, 0.75f, 0.1f, 4.0f);
    
    // Side clusters (hanging foliage)
    drawCube(shader, cubeVAO, glm::vec3(position.x + spread * 0.5f, foliageY + height * 0.05f, position.z + spread * 0.15f),
             glm::vec3(clusterSize * 0.8f, clusterSize * 0.5f, clusterSize * 0.7f), leafMid, 0, 0.18f, 0.68f, 0.08f, 4.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - spread * 0.45f, foliageY + height * 0.08f, position.z - spread * 0.2f),
             glm::vec3(clusterSize * 0.75f, clusterSize * 0.55f, clusterSize * 0.8f), leafLight, 0, 0.2f, 0.7f, 0.08f, 4.0f);
    
    // Forward/backward clusters
    drawCube(shader, cubeVAO, glm::vec3(position.x + spread * 0.15f, foliageY + height * 0.1f, position.z + spread * 0.45f),
             glm::vec3(clusterSize * 0.9f, clusterSize * 0.6f, clusterSize * 0.85f), leafDark, 0, 0.16f, 0.66f, 0.08f, 4.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - spread * 0.1f, foliageY + height * 0.18f, position.z - spread * 0.4f),
             glm::vec3(clusterSize * 0.85f, clusterSize * 0.6f, clusterSize * 0.9f), leafBright, 0, 0.2f, 0.72f, 0.1f, 4.0f);
    
    // Top crown clusters
    drawCube(shader, cubeVAO, glm::vec3(position.x + spread * 0.08f, foliageY + height * 0.35f, position.z + spread * 0.05f),
             glm::vec3(clusterSize * 0.7f, clusterSize * 0.5f, clusterSize * 0.7f), leafLight, 0, 0.22f, 0.75f, 0.1f, 4.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - spread * 0.12f, foliageY + height * 0.32f, position.z - spread * 0.08f),
             glm::vec3(clusterSize * 0.6f, clusterSize * 0.45f, clusterSize * 0.65f), leafBright, 0, 0.24f, 0.78f, 0.1f, 4.0f);
    
    // Small detail clusters for organic texture
    drawCube(shader, cubeVAO, glm::vec3(position.x + spread * 0.55f, foliageY + height * 0.02f, position.z - spread * 0.05f),
             glm::vec3(clusterSize * 0.4f, clusterSize * 0.35f, clusterSize * 0.4f), leafMid, 0, 0.18f, 0.68f, 0.08f, 4.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - spread * 0.5f, foliageY + height * 0.05f, position.z + spread * 0.1f),
             glm::vec3(clusterSize * 0.45f, clusterSize * 0.35f, clusterSize * 0.45f), leafDark, 0, 0.15f, 0.62f, 0.08f, 4.0f);
}

// Draw the outdoor environment: road, ground, sky, trees
void drawOutdoorEnvironment(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO)
{
    // === GROUND OUTSIDE (grass) ===
    glm::vec3 grassColor(0.25f, 0.42f, 0.18f);
    glm::mat4 groundModel = glm::mat4(1.0f);
    groundModel = glm::translate(groundModel, glm::vec3(LOT_WIDTH/2, -0.05f, LOT_DEPTH + 50.0f));
    groundModel = glm::scale(groundModel, glm::vec3(150.0f, 1.0f, 120.0f));
    drawQuad(shader, quadVAO, groundModel, grassColor, 0, 0.2f, 0.7f, 0.1f, 4.0f);
    
    // === ROAD in front of parking lot entrance ===
    glm::vec3 asphaltColor(0.18f, 0.18f, 0.2f);
    glm::mat4 roadModel = glm::mat4(1.0f);
    roadModel = glm::translate(roadModel, glm::vec3(LOT_WIDTH/2, 0.01f, LOT_DEPTH + 8.0f));
    roadModel = glm::scale(roadModel, glm::vec3(30.0f, 1.0f, 12.0f));
    drawQuad(shader, quadVAO, roadModel, asphaltColor, 0, 0.12f, 0.5f, 0.2f, 16.0f);
    
    // Road markings (center line)
    glm::vec3 roadLineColor(0.95f, 0.9f, 0.7f);
    for (int i = 0; i < 6; i++) {
        float lineZ = LOT_DEPTH + 4.0f + i * 2.5f;
        drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 0.02f, lineZ),
                 glm::vec3(1.5f, 0.02f, 0.15f), roadLineColor, 1, 0.4f, 0.8f, 0.3f, 16.0f);
    }
    
    // Sidewalk/curb
    glm::vec3 sidewalkColor(0.55f, 0.53f, 0.5f);
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 0.08f, LOT_DEPTH + 15.0f),
             glm::vec3(35.0f, 0.15f, 3.0f), sidewalkColor, 0, 0.18f, 0.65f, 0.15f, 12.0f);
    
    // === TREES along the road ===
    // Variety of tree sizes for realism
    float treePositions[][4] = {
        // x, z, height, spread
        {5.0f, LOT_DEPTH + 20.0f, 6.0f, 3.5f},
        {15.0f, LOT_DEPTH + 22.0f, 8.0f, 4.5f},
        {25.0f, LOT_DEPTH + 19.0f, 5.5f, 3.0f},
        {38.0f, LOT_DEPTH + 21.0f, 7.5f, 4.0f},
        {48.0f, LOT_DEPTH + 23.0f, 9.0f, 5.0f},
        {55.0f, LOT_DEPTH + 18.0f, 6.5f, 3.8f},
        // Trees further back
        {8.0f, LOT_DEPTH + 35.0f, 10.0f, 5.5f},
        {22.0f, LOT_DEPTH + 40.0f, 8.5f, 4.8f},
        {40.0f, LOT_DEPTH + 38.0f, 11.0f, 6.0f},
        {52.0f, LOT_DEPTH + 42.0f, 7.0f, 4.2f},
    };
    
    for (int i = 0; i < 10; i++) {
        drawTree(shader, cubeVAO, cylVAO, 
                 glm::vec3(treePositions[i][0], 0.0f, treePositions[i][1]),
                 treePositions[i][2], treePositions[i][3]);
    }
    
    // === SKY BACKDROP (distant) ===
    glm::vec3 skyColor(0.55f, 0.75f, 0.95f);
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 30.0f, LOT_DEPTH + 80.0f),
             glm::vec3(200.0f, 60.0f, 1.0f), skyColor, 1, 0.9f, 0.3f, 0.0f, 1.0f);
    
    // === DISTANT BUILDINGS silhouette ===
    glm::vec3 buildingColor(0.4f, 0.42f, 0.45f);
    drawCube(shader, cubeVAO, glm::vec3(15.0f, 8.0f, LOT_DEPTH + 60.0f),
             glm::vec3(12.0f, 16.0f, 8.0f), buildingColor, 0, 0.15f, 0.5f, 0.3f, 24.0f);
    drawCube(shader, cubeVAO, glm::vec3(35.0f, 12.0f, LOT_DEPTH + 65.0f),
             glm::vec3(10.0f, 24.0f, 8.0f), buildingColor * 0.9f, 0, 0.15f, 0.5f, 0.3f, 24.0f);
    drawCube(shader, cubeVAO, glm::vec3(50.0f, 6.0f, LOT_DEPTH + 55.0f),
             glm::vec3(14.0f, 12.0f, 10.0f), buildingColor * 1.1f, 0, 0.15f, 0.5f, 0.3f, 24.0f);
}

// Draw volumetric light rays coming through windows
void drawLightRays(Shader& shader, unsigned int cubeVAO)
{
    // Light rays are semi-transparent stretched cubes angled from windows
    // They simulate god rays / volumetric light scattering
    
    glm::vec3 rayColor(1.0f, 0.95f, 0.8f);  // Warm sunlight color
    float rayAlpha = 0.08f;  // Very subtle
    
    // Window positions along left wall (X = 0)
    float windowSpacing = 8.0f;
    float windowY = 2.5f;  // Middle height of window
    
    for (float z = 5.0f; z < LOT_DEPTH - 5.0f; z += windowSpacing) {
        // Each window casts a light ray into the parking lot
        // Ray extends from window toward the interior at an angle
        
        float rayLength = 15.0f;
        float rayWidth = 2.5f;
        float rayHeight = 2.0f;
        
        // Position ray center (extending from window into parking lot)
        float rayX = rayLength / 2.5f;
        float rayZ = z;
        float rayY = windowY - 0.8f;  // Rays angle downward
        
        // Draw multiple overlapping rays for softer appearance
        for (int layer = 0; layer < 3; layer++) {
            float layerOffset = layer * 0.3f;
            float layerScale = 1.0f - layer * 0.15f;
            
            drawCubeRotated(shader, cubeVAO, 
                           glm::vec3(rayX + layerOffset, rayY - layer * 0.4f, rayZ),
                           glm::vec3(rayLength * layerScale, rayHeight * layerScale * 0.5f, rayWidth * layerScale),
                           glm::vec3(25.0f, 0.0f, 0.0f),  // Angled downward
                           rayColor * (0.15f - layer * 0.03f), 1, 0.9f, 0.1f, 0.0f, 1.0f);
        }
        
        // Ground light pool (where ray hits floor)
        float poolX = rayLength * 0.8f;
        float poolZ = z;
        drawCube(shader, cubeVAO, glm::vec3(poolX, 0.02f, poolZ),
                 glm::vec3(4.0f, 0.02f, 3.5f), rayColor * 0.2f, 1, 0.7f, 0.3f, 0.0f, 1.0f);
    }
}

// ============================================================
// PARKING LOT SIGNAGE AND SAFETY FEATURES
// ============================================================

// Draw a STOP sign with post
void drawStopSign(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float rotation)
{
    // Colors
    glm::vec3 postColor(0.45f, 0.45f, 0.48f);    // Metal gray post
    glm::vec3 signRed(0.85f, 0.1f, 0.1f);        // Bright red for STOP
    glm::vec3 signWhite(0.95f, 0.95f, 0.95f);    // White border and text
    glm::vec3 postBase(0.3f, 0.3f, 0.32f);       // Darker base
    
    float postHeight = 2.2f;
    float postRadius = 0.04f;
    float signSize = 0.5f;
    
    // Post (vertical pole)
    drawCubeRotated(shader, cubeVAO, 
                   glm::vec3(position.x, position.y + postHeight/2, position.z),
                   glm::vec3(postRadius * 2, postHeight, postRadius * 2),
                   glm::vec3(0, rotation, 0),
                   postColor, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    
    // Post base (wider at bottom)
    drawCubeRotated(shader, cubeVAO, 
                   glm::vec3(position.x, position.y + 0.1f, position.z),
                   glm::vec3(postRadius * 4, 0.2f, postRadius * 4),
                   glm::vec3(0, rotation, 0),
                   postBase, 3, 0.1f, 0.5f, 0.5f, 16.0f);
    
    // Sign plate (octagonal approximated as rotated squares layered)
    float signY = position.y + postHeight + signSize * 0.4f;
    
    // Red octagon background (main sign)
    drawCubeRotated(shader, cubeVAO, 
                   glm::vec3(position.x, signY, position.z),
                   glm::vec3(signSize * 0.08f, signSize, signSize),
                   glm::vec3(0, rotation, 0),
                   signRed, 1, 0.15f, 0.7f, 0.4f, 16.0f);
    
    // 45-degree rotated layer to make it more octagonal
    drawCubeRotated(shader, cubeVAO, 
                   glm::vec3(position.x, signY, position.z),
                   glm::vec3(signSize * 0.07f, signSize * 0.75f, signSize * 0.75f),
                   glm::vec3(0, rotation + 45, 0),
                   signRed, 1, 0.15f, 0.7f, 0.4f, 16.0f);
    
    // White border (slightly larger)
    drawCubeRotated(shader, cubeVAO, 
                   glm::vec3(position.x + (rotation == 0 ? 0.002f : 0), signY, position.z + (rotation == 90 ? 0.002f : 0)),
                   glm::vec3(signSize * 0.02f, signSize * 0.85f, signSize * 0.85f),
                   glm::vec3(0, rotation, 0),
                   signWhite, 1, 0.2f, 0.8f, 0.5f, 24.0f);
    
    // "STOP" text approximation (horizontal white bar)
    drawCubeRotated(shader, cubeVAO, 
                   glm::vec3(position.x + (rotation == 0 ? 0.003f : 0), signY, position.z + (rotation == 90 ? 0.003f : 0)),
                   glm::vec3(signSize * 0.015f, signSize * 0.2f, signSize * 0.6f),
                   glm::vec3(0, rotation, 0),
                   signWhite, 1, 0.2f, 0.8f, 0.5f, 24.0f);
}

// Draw a safety bollard (protective post)
void drawSafetyBollard(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, glm::vec3 position)
{
    // Colors - bright yellow for visibility
    glm::vec3 yellowPaint(0.95f, 0.8f, 0.1f);    // Safety yellow
    glm::vec3 blackStripe(0.1f, 0.1f, 0.1f);     // Black reflective stripe
    glm::vec3 concreteBase(0.5f, 0.5f, 0.52f);   // Concrete foundation
    
    float bollardHeight = 0.9f;
    float bollardRadius = 0.1f;
    
    // Concrete base
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + 0.05f, position.z),
             glm::vec3(bollardRadius * 3, 0.1f, bollardRadius * 3), concreteBase, 0, 0.12f, 0.6f, 0.15f, 8.0f);
    
    // Main bollard body (yellow)
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + bollardHeight/2, position.z),
             glm::vec3(bollardRadius * 2, bollardHeight, bollardRadius * 2), yellowPaint, 3, 0.15f, 0.7f, 0.5f, 24.0f);
    
    // Black reflective stripes (2 stripes)
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + bollardHeight * 0.35f, position.z),
             glm::vec3(bollardRadius * 2.05f, 0.06f, bollardRadius * 2.05f), blackStripe, 3, 0.1f, 0.5f, 0.8f, 48.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + bollardHeight * 0.65f, position.z),
             glm::vec3(bollardRadius * 2.05f, 0.06f, bollardRadius * 2.05f), blackStripe, 3, 0.1f, 0.5f, 0.8f, 48.0f);
    
    // Top cap (slightly domed)
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + bollardHeight + 0.03f, position.z),
             glm::vec3(bollardRadius * 2.1f, 0.06f, bollardRadius * 2.1f), yellowPaint, 3, 0.18f, 0.75f, 0.6f, 32.0f);
}

// Draw a light pole with lamp
void drawLightPole(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, glm::vec3 position, float height)
{
    // Colors
    glm::vec3 poleColor(0.2f, 0.2f, 0.22f);      // Dark metal pole
    glm::vec3 lampHousing(0.25f, 0.25f, 0.28f);  // Lamp housing
    glm::vec3 lampLight(1.0f, 0.95f, 0.85f);     // Warm white light
    glm::vec3 baseColor(0.3f, 0.3f, 0.32f);      // Base
    
    float poleRadius = 0.08f;
    
    // Pole base (wide bottom)
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + 0.15f, position.z),
             glm::vec3(poleRadius * 4, 0.3f, poleRadius * 4), baseColor, 3, 0.1f, 0.5f, 0.5f, 16.0f);
    
    // Main pole (tapered - wider at bottom)
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + height * 0.25f, position.z),
             glm::vec3(poleRadius * 2.5f, height * 0.5f, poleRadius * 2.5f), poleColor, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x, position.y + height * 0.7f, position.z),
             glm::vec3(poleRadius * 2, height * 0.4f, poleRadius * 2), poleColor, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    
    // Lamp arm (horizontal extension)
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.3f, position.y + height - 0.1f, position.z),
             glm::vec3(0.6f, poleRadius * 1.5f, poleRadius * 1.5f), poleColor, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    
    // Lamp housing (rectangular fixture)
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.5f, position.y + height - 0.25f, position.z),
             glm::vec3(0.4f, 0.15f, 0.25f), lampHousing, 3, 0.1f, 0.5f, 0.5f, 24.0f);
    
    // Light panel (bottom of housing - glowing)
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.5f, position.y + height - 0.35f, position.z),
             glm::vec3(0.35f, 0.03f, 0.2f), lampLight, 1, 0.95f, 0.2f, 0.1f, 4.0f);
    
    // Light glow on ground (simulated light pool)
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.5f, 0.02f, position.z),
             glm::vec3(3.0f, 0.02f, 3.0f), glm::vec3(1.0f, 0.95f, 0.8f) * 0.15f, 1, 0.6f, 0.4f, 0.1f, 4.0f);
}

// Draw STOP stencil painted on ground
void drawStopStencil(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float rotation)
{
    glm::vec3 paintWhite(0.95f, 0.95f, 0.95f);
    float letterHeight = 1.2f;
    float letterWidth = 0.4f;
    float letterThick = 0.12f;
    float spacing = 0.55f;
    
    // S
    float sx = position.x - spacing * 1.5f;
    drawCubeRotated(shader, cubeVAO, glm::vec3(sx, 0.015f, position.z), 
                   glm::vec3(letterWidth, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // top
    drawCubeRotated(shader, cubeVAO, glm::vec3(sx - letterWidth * 0.35f, 0.015f, position.z - letterHeight * 0.15f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.35f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // top-left
    drawCubeRotated(shader, cubeVAO, glm::vec3(sx, 0.015f, position.z - letterHeight * 0.35f), 
                   glm::vec3(letterWidth, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // middle
    drawCubeRotated(shader, cubeVAO, glm::vec3(sx + letterWidth * 0.35f, 0.015f, position.z - letterHeight * 0.55f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.35f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // bottom-right
    drawCubeRotated(shader, cubeVAO, glm::vec3(sx, 0.015f, position.z - letterHeight * 0.72f), 
                   glm::vec3(letterWidth, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // bottom
    
    // T
    float tx = position.x - spacing * 0.5f;
    drawCubeRotated(shader, cubeVAO, glm::vec3(tx, 0.015f, position.z), 
                   glm::vec3(letterWidth, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // top
    drawCubeRotated(shader, cubeVAO, glm::vec3(tx, 0.015f, position.z - letterHeight * 0.4f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.8f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // vertical
    
    // O
    float ox = position.x + spacing * 0.5f;
    drawCubeRotated(shader, cubeVAO, glm::vec3(ox, 0.015f, position.z), 
                   glm::vec3(letterWidth, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // top
    drawCubeRotated(shader, cubeVAO, glm::vec3(ox, 0.015f, position.z - letterHeight * 0.72f), 
                   glm::vec3(letterWidth, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // bottom
    drawCubeRotated(shader, cubeVAO, glm::vec3(ox - letterWidth * 0.35f, 0.015f, position.z - letterHeight * 0.36f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.7f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // left
    drawCubeRotated(shader, cubeVAO, glm::vec3(ox + letterWidth * 0.35f, 0.015f, position.z - letterHeight * 0.36f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.7f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // right
    
    // P
    float px = position.x + spacing * 1.5f;
    drawCubeRotated(shader, cubeVAO, glm::vec3(px - letterWidth * 0.35f, 0.015f, position.z - letterHeight * 0.36f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.8f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // vertical
    drawCubeRotated(shader, cubeVAO, glm::vec3(px, 0.015f, position.z), 
                   glm::vec3(letterWidth * 0.7f, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // top
    drawCubeRotated(shader, cubeVAO, glm::vec3(px + letterWidth * 0.25f, 0.015f, position.z - letterHeight * 0.15f), 
                   glm::vec3(letterThick, 0.02f, letterHeight * 0.3f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // right curve
    drawCubeRotated(shader, cubeVAO, glm::vec3(px, 0.015f, position.z - letterHeight * 0.32f), 
                   glm::vec3(letterWidth * 0.7f, 0.02f, letterThick), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);  // middle
}

// Draw directional arrow on ground
void drawDirectionalArrow(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float rotation)
{
    glm::vec3 paintWhite(0.95f, 0.95f, 0.95f);
    
    // Arrow shaft
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x, 0.015f, position.z), 
                   glm::vec3(0.25f, 0.02f, 1.8f), glm::vec3(0, rotation, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);
    
    // Arrow head (chevron)
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x + 0.3f, 0.015f, position.z + 0.7f), 
                   glm::vec3(0.2f, 0.02f, 0.6f), glm::vec3(0, rotation - 35, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);
    drawCubeRotated(shader, cubeVAO, glm::vec3(position.x - 0.3f, 0.015f, position.z + 0.7f), 
                   glm::vec3(0.2f, 0.02f, 0.6f), glm::vec3(0, rotation + 35, 0), paintWhite, 1, 0.5f, 0.8f, 0.3f, 16.0f);
}

// Draw speed bump
void drawSpeedBump(Shader& shader, unsigned int cubeVAO, glm::vec3 position, float width)
{
    glm::vec3 yellowPaint(0.95f, 0.8f, 0.1f);
    glm::vec3 blackPaint(0.15f, 0.15f, 0.15f);
    
    float bumpHeight = 0.08f;
    float bumpDepth = 0.4f;
    
    // Main bump body (yellow and black stripes)
    float stripeWidth = 0.3f;
    int numStripes = (int)(width / stripeWidth);
    
    for (int i = 0; i < numStripes; i++) {
        float stripeX = position.x - width/2 + i * stripeWidth + stripeWidth/2;
        glm::vec3 color = (i % 2 == 0) ? yellowPaint : blackPaint;
        
        drawCube(shader, cubeVAO, glm::vec3(stripeX, position.y + bumpHeight/2, position.z),
                 glm::vec3(stripeWidth, bumpHeight, bumpDepth), color, 1, 0.2f, 0.7f, 0.4f, 16.0f);
    }
}

// Draw all parking lot signage and safety features
void drawParkingSignage(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO)
{
    // === STOP SIGNS ===
    // Near entrance/exit
    drawStopSign(shader, cubeVAO, glm::vec3(LOT_WIDTH/2 + 5.0f, 0.0f, LOT_DEPTH - 1.5f), 0);
    drawStopSign(shader, cubeVAO, glm::vec3(LOT_WIDTH/2 - 5.0f, 0.0f, LOT_DEPTH - 1.5f), 180);
    
    // At lane intersections inside
    drawStopSign(shader, cubeVAO, glm::vec3(15.0f, 0.0f, LOT_DEPTH/2), 90);
    
    // === STOP STENCILS (painted on ground near stop signs) ===
    drawStopStencil(shader, cubeVAO, glm::vec3(LOT_WIDTH/2 + 5.0f, 0.0f, LOT_DEPTH - 4.0f), 0);
    drawStopStencil(shader, cubeVAO, glm::vec3(LOT_WIDTH/2 - 5.0f, 0.0f, LOT_DEPTH - 4.0f), 0);
    drawStopStencil(shader, cubeVAO, glm::vec3(12.0f, 0.0f, LOT_DEPTH/2), 90);
    
    // === DIRECTIONAL ARROWS ===
    // Guide traffic flow in lanes
    drawDirectionalArrow(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 0.0f, 10.0f), 0);
    drawDirectionalArrow(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 0.0f, 25.0f), 0);
    drawDirectionalArrow(shader, cubeVAO, glm::vec3(25.0f, 0.0f, LOT_DEPTH/2 + 5.0f), 90);
    drawDirectionalArrow(shader, cubeVAO, glm::vec3(45.0f, 0.0f, LOT_DEPTH/2 + 5.0f), 90);
    
    // === SAFETY BOLLARDS ===
    // In front of pillars to protect them
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(12.0f, 0.0f, 9.0f));
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(12.0f, 0.0f, 19.0f));
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(12.0f, 0.0f, 29.0f));
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(24.0f, 0.0f, 9.0f));
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(24.0f, 0.0f, 19.0f));
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(24.0f, 0.0f, 29.0f));
    
    // Near entrance to protect walls
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(LOT_WIDTH/2 - 8.0f, 0.0f, LOT_DEPTH - 2.0f));
    drawSafetyBollard(shader, cubeVAO, cylVAO, glm::vec3(LOT_WIDTH/2 + 8.0f, 0.0f, LOT_DEPTH - 2.0f));
    
    // === SPEED BUMPS ===
    drawSpeedBump(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 0.0f, LOT_DEPTH - 6.0f), 8.0f);
    drawSpeedBump(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 0.0f, 6.0f), 8.0f);
    
    // === EXTERIOR LIGHT POLES ===
    // Outside the parking lot near the road
    drawLightPole(shader, cubeVAO, cylVAO, glm::vec3(-5.0f, 0.0f, LOT_DEPTH + 5.0f), 5.0f);
    drawLightPole(shader, cubeVAO, cylVAO, glm::vec3(LOT_WIDTH + 5.0f, 0.0f, LOT_DEPTH + 5.0f), 5.0f);
    drawLightPole(shader, cubeVAO, cylVAO, glm::vec3(LOT_WIDTH/2, 0.0f, LOT_DEPTH + 12.0f), 5.0f);
}

