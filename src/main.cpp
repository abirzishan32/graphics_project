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
void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawQuad(Shader& shader, unsigned int VAO, glm::mat4 model, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void setupLighting(Shader& shader);
void drawParkingLot(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO);
void drawCar(Shader& shader, unsigned int cubeVAO, glm::vec3 position, glm::vec3 carColor);

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

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Build and compile shader program
    Shader shader("pbrVertex.vs", "pbrFragment.fs");

    // Create geometry
    unsigned int cubeVAO = createCubeVAO();
    unsigned int quadVAO = createQuadVAO();

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Clear screen with dark gray (underground parking atmosphere)
        glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Set view/projection matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camera.Position);

        // Setup lighting
        setupLighting(shader);

        // Draw the parking lot
        drawParkingLot(shader, cubeVAO, quadVAO);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &quadVAO);
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

void setupLighting(Shader& shader)
{
    // Directional light (sunlight through windows) - coming from right side
    shader.setBool("useDirLight", true);
    shader.setVec3("dirLight.direction", glm::vec3(-0.5f, -0.7f, -0.3f));
    shader.setVec3("dirLight.ambient", glm::vec3(0.15f, 0.14f, 0.12f));
    shader.setVec3("dirLight.diffuse", glm::vec3(0.8f, 0.75f, 0.6f));
    shader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.45f, 0.4f));

    // Point lights (ceiling-mounted fixtures)
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
            shader.setVec3(base + ".ambient", glm::vec3(0.08f, 0.07f, 0.06f));
            shader.setVec3(base + ".diffuse", glm::vec3(0.9f, 0.85f, 0.7f));
            shader.setVec3(base + ".specular", glm::vec3(0.6f, 0.55f, 0.5f));
            shader.setFloat(base + ".constant", 1.0f);
            shader.setFloat(base + ".linear", 0.09f);
            shader.setFloat(base + ".quadratic", 0.032f);
            lightIdx++;
        }
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

void drawCar(Shader& shader, unsigned int cubeVAO, glm::vec3 position, glm::vec3 carColor)
{
    // Car body (main)
    drawCube(shader, cubeVAO, 
             glm::vec3(position.x, position.y + 0.5f, position.z),
             glm::vec3(1.8f, 0.6f, 4.2f), carColor, 2, 0.15f, 0.8f, 0.9f, 64.0f);
    
    // Car top (cabin)
    drawCube(shader, cubeVAO,
             glm::vec3(position.x, position.y + 1.0f, position.z - 0.3f),
             glm::vec3(1.6f, 0.5f, 2.2f), carColor * 0.9f, 2, 0.15f, 0.8f, 0.9f, 64.0f);
    
    // Windows (glass)
    drawCube(shader, cubeVAO,
             glm::vec3(position.x, position.y + 1.0f, position.z - 0.3f),
             glm::vec3(1.65f, 0.4f, 2.0f), glm::vec3(0.1f, 0.15f, 0.2f), 4, 0.1f, 0.3f, 1.0f, 128.0f);
    
    // Wheels (4)
    glm::vec3 wheelColor(0.1f, 0.1f, 0.1f);
    float wheelY = position.y + 0.25f;
    drawCube(shader, cubeVAO, glm::vec3(position.x - 0.7f, wheelY, position.z + 1.2f),
             glm::vec3(0.3f, 0.5f, 0.5f), wheelColor, 3, 0.1f, 0.6f, 0.3f, 16.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.7f, wheelY, position.z + 1.2f),
             glm::vec3(0.3f, 0.5f, 0.5f), wheelColor, 3, 0.1f, 0.6f, 0.3f, 16.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x - 0.7f, wheelY, position.z - 1.2f),
             glm::vec3(0.3f, 0.5f, 0.5f), wheelColor, 3, 0.1f, 0.6f, 0.3f, 16.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.7f, wheelY, position.z - 1.2f),
             glm::vec3(0.3f, 0.5f, 0.5f), wheelColor, 3, 0.1f, 0.6f, 0.3f, 16.0f);
    
    // Headlights
    drawCube(shader, cubeVAO, glm::vec3(position.x - 0.6f, position.y + 0.5f, position.z + 2.1f),
             glm::vec3(0.3f, 0.2f, 0.05f), glm::vec3(1.0f, 0.95f, 0.8f), 4, 0.5f, 0.5f, 1.0f, 64.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.6f, position.y + 0.5f, position.z + 2.1f),
             glm::vec3(0.3f, 0.2f, 0.05f), glm::vec3(1.0f, 0.95f, 0.8f), 4, 0.5f, 0.5f, 1.0f, 64.0f);
    
    // Taillights
    drawCube(shader, cubeVAO, glm::vec3(position.x - 0.6f, position.y + 0.5f, position.z - 2.1f),
             glm::vec3(0.25f, 0.15f, 0.05f), glm::vec3(0.8f, 0.1f, 0.1f), 4, 0.3f, 0.6f, 0.8f, 32.0f);
    drawCube(shader, cubeVAO, glm::vec3(position.x + 0.6f, position.y + 0.5f, position.z - 2.1f),
             glm::vec3(0.25f, 0.15f, 0.05f), glm::vec3(0.8f, 0.1f, 0.1f), 4, 0.3f, 0.6f, 0.8f, 32.0f);
}

void drawParkingLot(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO)
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
    drawCar(shader, cubeVAO, glm::vec3(spotStartX, 0.0f, 6.5f), carColors[0]);
    drawCar(shader, cubeVAO, glm::vec3(spotStartX, 0.0f, 12.0f), carColors[1]);
    drawCar(shader, cubeVAO, glm::vec3(spotStartX, 0.0f, 23.0f), carColors[2]);
    drawCar(shader, cubeVAO, glm::vec3(spotStartX, 0.0f, 28.5f), carColors[3]);
    
    // Cars in right row  
    drawCar(shader, cubeVAO, glm::vec3(spotStartX2, 0.0f, 6.5f), carColors[4]);
    drawCar(shader, cubeVAO, glm::vec3(spotStartX2, 0.0f, 17.5f), carColors[5]);
    drawCar(shader, cubeVAO, glm::vec3(spotStartX2, 0.0f, 28.5f), carColors[0]);
    
    // Cars in middle row
    drawCar(shader, cubeVAO, glm::vec3(middleX - SPOT_WIDTH/2 - 0.5f, 0.0f, 8.5f), carColors[2]);
    drawCar(shader, cubeVAO, glm::vec3(middleX + SPOT_WIDTH/2 + 0.5f, 0.0f, 14.0f), carColors[1]);
    drawCar(shader, cubeVAO, glm::vec3(middleX - SPOT_WIDTH/2 - 0.5f, 0.0f, 25.0f), carColors[4]);
    
    // === EXIT/ENTRANCE SIGNS ===
    // Exit sign above entrance
    drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH/2, 2.8f, LOT_DEPTH - 0.5f),
             glm::vec3(1.5f, 0.4f, 0.1f), glm::vec3(0.1f, 0.6f, 0.2f), 1, 0.6f, 0.5f, 0.3f, 16.0f);
}
