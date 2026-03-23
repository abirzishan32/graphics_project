#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "basic_camera.h"
#include "car_generator.h"
#include "human_generator.h"

#include <iostream>
#include <vector>
#include <cmath>

// stb_image: single-header image loader for textures (GL_REPEAT, mipmaps)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
void drawCubeAlpha(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess, float alpha);
void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawCylinder(Shader& shader, unsigned int VAO, int segments, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawQuad(Shader& shader, unsigned int VAO, glm::mat4 model, 
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void setupLighting(Shader& shader);
void drawParkingLot(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO, unsigned int wedgeVAO);
void drawRealisticCar(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, unsigned int wedgeVAO, glm::vec3 position, glm::vec3 carColor, float rotation);
void drawAttendantBooth(Shader& shader, unsigned int cubeVAO, glm::vec3 position);
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
void drawScene(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO, unsigned int wedgeVAO);
void drawStackedEmptyFloors(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO);
void drawElevatorSystem(Shader& shader, unsigned int cubeVAO);
unsigned int createCurvedBarrierVAO(int& outVertexCount);
void drawCurvedBarrier(Shader& shader, unsigned int VAO, int vertexCount);
unsigned int createSphereVAO(int& outVertexCount);
unsigned int createConeVAO(int& outVertexCount);
unsigned int loadTexture(const char* path);
void drawTexturedObjects(Shader& shader, unsigned int cubeVAO,
    unsigned int sphereVAO, int sphereCount,
    unsigned int coneVAO, int coneCount,
    unsigned int texBrick, unsigned int texContainer);

// Settings (Window size in points, may differ from pixels on Retina)
unsigned int SCR_WIDTH = 1400;
unsigned int SCR_HEIGHT = 900;

// Parking lot dimensions (meters)
const float LOT_WIDTH = 140.0f;
const float LOT_DEPTH = 100.0f;
const float CEILING_HEIGHT = 3.5f;
const float WALL_THICKNESS = 0.3f;
const float PILLAR_SIZE = 0.5f;
const float SPOT_WIDTH = 2.5f;
const float SPOT_DEPTH = 5.0f;
const float LANE_WIDTH = 6.0f;
const int EXTRA_FLOORS = 5;
const int TOP_FLOOR_INDEX = 5;
const float FLOOR_TO_FLOOR_HEIGHT = 4.0f;

// Camera - starts inside the parking lot at pedestrian height
BasicCamera camera(5.0f, 1.7f, LOT_DEPTH/2.0f);
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Lights toggle
bool ceilingLightsOn = true;
bool entranceLightsOn = true;
bool oneKeyPressed = false;
bool twoKeyPressed = false;
bool lKeyPressed = false;

// Texture mode toggle
int  useTexture = 0;
bool threeKeyPressed = false;

// Viewport toggle
bool showFourViewports = true;
bool vKeyPressed = false;
bool eKeyPressed = false;

unsigned int texElevatorPanelBase = 0;     // wood-like base texture
unsigned int texElevatorButtons = 0;       // button overlay texture
unsigned int texSevenSegment[10] = {0};    // 0..9 LED digit textures

struct Elevator {
    enum class State {
        IDLE,
        DOORS_OPENING,
        DOORS_OPEN,
        DOORS_CLOSING,
        MOVING
    };

    State state = State::DOORS_OPEN;
    glm::vec3 shaftCenter = glm::vec3(LOT_WIDTH - 4.0f, 0.0f, 4.0f);
    float shaftWidth = 3.2f;
    float shaftDepth = 3.2f;
    float cabinWidth = 2.2f;
    float cabinDepth = 2.0f;
    float cabinHeight = 2.6f;

    float doorOpenAmount = 1.0f;
    float doorSpeed = 1.4f;
    float moveSpeed = 2.8f;
    float doorsOpenHoldTime = 1.4f;
    float holdTimer = 0.0f;

    int currentFloor = 0;
    int targetFloor = 0;
    bool riderInside = false;
    float cabinY = cabinHeight * 0.5f;

    float floorY(int floorIndex) const {
        return floorIndex * FLOOR_TO_FLOOR_HEIGHT;
    }

    int floorFromY(float y) const {
        int floor = static_cast<int>(std::round(y / FLOOR_TO_FLOOR_HEIGHT));
        if (floor < 0) floor = 0;
        if (floor > TOP_FLOOR_INDEX) floor = TOP_FLOOR_INDEX;
        return floor;
    }

    float doorTravel() const {
        return doorOpenAmount * (cabinWidth * 0.26f);
    }

    bool isCameraInsideCabin(const BasicCamera& cam) const {
        float minY = cabinY - cabinHeight * 0.5f + 0.05f;
        float maxY = cabinY + cabinHeight * 0.5f - 0.05f;
        float halfX = cabinWidth * 0.38f;
        float halfZ = cabinDepth * 0.38f;
        return std::abs(cam.Position.x - shaftCenter.x) <= halfX &&
               std::abs(cam.Position.z - shaftCenter.z) <= halfZ &&
               cam.Position.y >= minY &&
               cam.Position.y <= maxY;
    }

    bool canAcceptFloorInput(const BasicCamera& cam) const {
        return isCameraInsideCabin(cam) &&
               (state == State::IDLE || state == State::DOORS_OPEN || state == State::DOORS_OPENING);
    }

    bool isCameraInFrontOfDoor(const BasicCamera& cam) const {
        int landingFloor = floorFromY(cam.Position.y);
        float doorPlaneZ = shaftCenter.z + shaftDepth * 0.5f + 0.55f;
        float halfX = shaftWidth * 0.5f + 0.9f;
        float minY = floorY(landingFloor) - 0.2f;
        float maxY = floorY(landingFloor) + cabinHeight + 0.3f;

        return std::abs(cam.Position.x - shaftCenter.x) <= halfX &&
               std::abs(cam.Position.z - doorPlaneZ) <= 1.15f &&
               cam.Position.y >= minY &&
               cam.Position.y <= maxY;
    }

    void requestFloor(int floorIndex) {
        if (floorIndex < 0 || floorIndex > TOP_FLOOR_INDEX) return;
        targetFloor = floorIndex;

        if (targetFloor == currentFloor) {
            if (state == State::IDLE || state == State::DOORS_CLOSING) {
                state = State::DOORS_OPENING;
            }
            return;
        }

        if (state == State::DOORS_OPEN || state == State::DOORS_OPENING) {
            state = State::DOORS_CLOSING;
            return;
        }
        if (state == State::IDLE) {
            state = State::MOVING;
        }
    }

    void requestFromOutside(int floorIndex) {
        if (floorIndex < 0 || floorIndex > TOP_FLOOR_INDEX) return;

        targetFloor = floorIndex;
        if (targetFloor == currentFloor) {
            if (state == State::IDLE || state == State::DOORS_CLOSING) {
                state = State::DOORS_OPENING;
            }
            return;
        }

        if (state == State::DOORS_OPEN || state == State::DOORS_OPENING) {
            state = State::DOORS_CLOSING;
        } else if (state == State::IDLE) {
            state = State::MOVING;
        }
    }

    void update(float dt, BasicCamera& cam) {
        float previousCabinY = cabinY;

        // Hall-call convenience: if user stands in front of the door on current landing,
        // open the door without needing to be inside the cabin.
        if (isCameraInFrontOfDoor(cam)) {
            int landingFloor = floorFromY(cam.Position.y);
            if (landingFloor == currentFloor && (state == State::IDLE || state == State::DOORS_CLOSING)) {
                targetFloor = currentFloor;
                state = State::DOORS_OPENING;
            }
        }

        if (!riderInside) {
            riderInside = isCameraInsideCabin(cam);
        }

        switch (state) {
            case State::IDLE:
                if (targetFloor != currentFloor) {
                    state = State::MOVING;
                }
                break;

            case State::DOORS_OPENING:
                doorOpenAmount += doorSpeed * dt;
                if (doorOpenAmount >= 1.0f) {
                    doorOpenAmount = 1.0f;
                    holdTimer = 0.0f;
                    state = State::DOORS_OPEN;
                }
                break;

            case State::DOORS_OPEN:
                holdTimer += dt;
                if (targetFloor != currentFloor) {
                    state = State::DOORS_CLOSING;
                } else if (holdTimer >= doorsOpenHoldTime) {
                    state = State::DOORS_CLOSING;
                }
                break;

            case State::DOORS_CLOSING:
                doorOpenAmount -= doorSpeed * dt;
                if (doorOpenAmount <= 0.0f) {
                    doorOpenAmount = 0.0f;
                    if (targetFloor != currentFloor) {
                        state = State::MOVING;
                    } else {
                        state = State::IDLE;
                    }
                }
                break;

            case State::MOVING: {
                float targetY = floorY(targetFloor) + cabinHeight * 0.5f;
                float direction = (targetY > cabinY) ? 1.0f : -1.0f;
                float step = moveSpeed * dt;

                if (std::abs(targetY - cabinY) <= step) {
                    cabinY = targetY;
                    currentFloor = targetFloor;
                    holdTimer = 0.0f;
                    state = State::DOORS_OPENING;
                } else {
                    cabinY += direction * step;
                }
                break;
            }
        }

        float deltaY = cabinY - previousCabinY;
        if (riderInside && std::abs(deltaY) > 0.0f) {
            cam.Position.y += deltaY;
            cam.EyeHeight += deltaY;
        }

        if (state == State::DOORS_OPENING || state == State::DOORS_OPEN) {
            riderInside = isCameraInsideCabin(cam);
        }
    }
};

Elevator elevator;
bool elevatorFloorKeyPressed[6] = {false, false, false, false, false, false};

// Manually define missing GLAD function pointers to fix linker errors
PFNGLACTIVETEXTUREPROC glad_glActiveTexture = nullptr;
PFNGLGENERATEMIPMAPPROC glad_glGenerateMipmap = nullptr;

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


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
    // Manually load function pointers that are missing from the project's glad.c
    // These are OpenGL 1.3+ core/extension functions needed for multi-texturing and mipmaps.
    glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC)glfwGetProcAddress("glActiveTexture");
    glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)glfwGetProcAddress("glGenerateMipmap");
    
    if (!glad_glActiveTexture || !glad_glGenerateMipmap) {
        std::cout << "Warning: Failed to load glActiveTexture or glGenerateMipmap" << std::endl;
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
    int curvedBarrierCount = 0;
    unsigned int curvedBarrierVAO = createCurvedBarrierVAO(curvedBarrierCount);
    // Curved objects for texture demonstration
    int sphereCount = 0;
    unsigned int sphereVAO = createSphereVAO(sphereCount);
    int coneCount = 0;
    unsigned int coneVAO = createConeVAO(coneCount);
    // Load image textures using stb_image (GL_REPEAT + trilinear mipmapping)
    unsigned int texBrick     = loadTexture("brick-wall.jpg");
    unsigned int texContainer = loadTexture("container.jpg");
    texElevatorPanelBase = loadTexture("container.jpg");
    texElevatorButtons   = loadTexture("elevator-buttons.png");
    for (int i = 0; i < 10; ++i) {
        std::string path = "src/seven-segments/" + std::to_string(i) + ".png";
        texSevenSegment[i] = loadTexture(path.c_str());
    }

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        elevator.update(deltaTime, camera);

        // Get actual framebuffer size for High-DPI displays (Mac)
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        float width = (float)display_w / 2.0f;
        float height = (float)display_h / 2.0f;

        // Clear screen once
        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        setupLighting(shader);

        if (showFourViewports) {
            // --- 1. Top-Left: Inside Back-Left Corner ---
            glViewport(0, (int)height, (int)width, (int)height);
            glm::mat4 projection = glm::perspective(glm::radians(60.0f), width / height, 0.1f, 200.0f);
            glm::vec3 camPos1 = glm::vec3(2.0f, 3.0f, 2.0f); 
            glm::mat4 view1 = glm::lookAt(camPos1, glm::vec3(40.0f, 1.0f, 30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shader.setMat4("projection", projection);
            shader.setMat4("view", view1);
            shader.setVec3("viewPos", camPos1);
            drawScene(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);
            drawCurvedBarrier(shader, curvedBarrierVAO, curvedBarrierCount);
            drawTexturedObjects(shader, cubeVAO, sphereVAO, sphereCount, coneVAO, coneCount, texBrick, texContainer);

            // --- 2. Top-Right: Inside Back-Right Corner ---
            glViewport((int)width, (int)height, (int)width, (int)height);
            glm::vec3 camPos2 = glm::vec3(LOT_WIDTH - 2.0f, 3.0f, 2.0f);
            glm::mat4 view2 = glm::lookAt(camPos2, glm::vec3(40.0f, 1.0f, 30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shader.setMat4("projection", projection);
            shader.setMat4("view", view2);
            shader.setVec3("viewPos", camPos2);
            drawScene(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);
            drawCurvedBarrier(shader, curvedBarrierVAO, curvedBarrierCount);
            drawTexturedObjects(shader, cubeVAO, sphereVAO, sphereCount, coneVAO, coneCount, texBrick, texContainer);

            // --- 3. Bottom-Left: Inside Front-Left Corner ---
            glViewport(0, 0, (int)width, (int)height);
            glm::vec3 camPos3 = glm::vec3(2.0f, 3.0f, LOT_DEPTH - 2.0f);
            glm::mat4 view3 = glm::lookAt(camPos3, glm::vec3(40.0f, 1.0f, 30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shader.setMat4("projection", projection);
            shader.setMat4("view", view3);
            shader.setVec3("viewPos", camPos3);
            drawScene(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);
            drawCurvedBarrier(shader, curvedBarrierVAO, curvedBarrierCount);
            drawTexturedObjects(shader, cubeVAO, sphereVAO, sphereCount, coneVAO, coneCount, texBrick, texContainer);

            // --- 4. Bottom-Right: Interactive View ---
            glViewport((int)width, 0, (int)width, (int)height);
            glm::mat4 projectionInteractive = glm::perspective(glm::radians(camera.Zoom), width / height, 0.1f, 200.0f);
            glm::mat4 viewInteractive = camera.GetViewMatrix();
            shader.setMat4("projection", projectionInteractive);
            shader.setMat4("view", viewInteractive);
            shader.setVec3("viewPos", camera.Position);
            drawScene(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);
            drawCurvedBarrier(shader, curvedBarrierVAO, curvedBarrierCount);
            drawTexturedObjects(shader, cubeVAO, sphereVAO, sphereCount, coneVAO, coneCount, texBrick, texContainer);
        } else {
            // --- Single Viewport: Interactive View ---
            glViewport(0, 0, display_w, display_h);
            float aspectRatio = (float)display_w / (float)display_h;
            glm::mat4 projectionFullscreen = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 200.0f);
            glm::mat4 viewFullscreen = camera.GetViewMatrix();
            shader.setMat4("projection", projectionFullscreen);
            shader.setMat4("view", viewFullscreen);
            shader.setVec3("viewPos", camera.Position);
            drawScene(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);
            drawCurvedBarrier(shader, curvedBarrierVAO, curvedBarrierCount);
            drawTexturedObjects(shader, cubeVAO, sphereVAO, sphereCount, coneVAO, coneCount, texBrick, texContainer);
        }

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

    bool insideElevatorForInput = elevator.canAcceptFloorInput(camera);
    bool outsideElevatorForInput = elevator.isCameraInFrontOfDoor(camera);

    // E key: open/call elevator when standing in front of the elevator door.
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!eKeyPressed && outsideElevatorForInput) {
            int landingFloor = elevator.floorFromY(camera.Position.y);
            elevator.requestFromOutside(landingFloor);
            eKeyPressed = true;
        }
    } else {
        eKeyPressed = false;
    }

    // Elevator floor buttons: 0=G, 1..5 = upper floors
    for (int floor = 0; floor <= TOP_FLOOR_INDEX; ++floor) {
        int key = (floor == 0) ? GLFW_KEY_0 : (GLFW_KEY_1 + floor - 1);
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            if (!elevatorFloorKeyPressed[floor]) {
                if (insideElevatorForInput) {
                    elevator.requestFloor(floor);
                } else if (outsideElevatorForInput) {
                    elevator.requestFromOutside(floor);
                } else {
                    if (floor == 1) {
                        entranceLightsOn = !entranceLightsOn;
                    } else if (floor == 2) {
                        ceilingLightsOn = !ceilingLightsOn;
                    } else if (floor == 3) {
                        useTexture = (useTexture + 1) % 3;
                    }
                }
                elevatorFloorKeyPressed[floor] = true;
            }
        } else {
            elevatorFloorKeyPressed[floor] = false;
        }
    }

    // L key for master toggle (optional convenience)
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!lKeyPressed) {
            bool newState = !(ceilingLightsOn || entranceLightsOn);
            ceilingLightsOn = newState;
            entranceLightsOn = newState;
            lKeyPressed = true;
        }
    } else {
        lKeyPressed = false;
    }

    // Key V: Toggle between 4 viewports and single viewport
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
        if (!vKeyPressed) {
            showFourViewports = !showFourViewports;
            vKeyPressed = true;
        }
    } else {
        vKeyPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // glViewport is handled in the render loop for multi-viewport setup
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

// ============================================================
// Curved Concrete Barrier: Tessellated Half-Cylinder Arc
// Demonstrates Full Phong Illumination (Ambient + Diffuse + Specular)
// with Multiple Light Sources (Ceiling Lights + Entrance Bar Lights)
// Each vertex has an analytically correct smooth normal for accurate
// per-fragment Phong shading across the curved surface.
// ============================================================
unsigned int createCurvedBarrierVAO(int& outVertexCount) {
    const int slices = 40;  // angular resolution (smoothness)
    const int stacks = 20;  // length segments along the Z axis
    const float radius = 0.9f;
    const float length = 12.0f;
    const float PI = 3.14159265358979f;
    
    // Vertex layout: pos(3) + normal(3) + texcoord(2) = 8 floats
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int j = 0; j <= stacks; j++) {
        float z = (float)j / (float)stacks * length;
        for (int i = 0; i <= slices; i++) {
            // Angle sweeps 0 -> PI (upper half-circle only)
            float angle = (float)i / (float)slices * PI;
            float x = radius * cos(angle);
            float y = radius * sin(angle);
            
            // Analytical normal: radial direction on a cylinder
            // Normal = normalize(cos(angle), sin(angle), 0)
            // which is already a unit vector by construction
            float nx = cos(angle);
            float ny = sin(angle);
            float nz = 0.0f;
            
            float u = (float)i / (float)slices;
            float v = (float)j / (float)stacks;
            
            vertices.insert(vertices.end(), {x, y, z, nx, ny, nz, u, v});
        }
    }
    
    // Two triangles (CCW winding) per quad for correct face culling
    for (int j = 0; j < stacks; j++) {
        for (int i = 0; i < slices; i++) {
            unsigned int tl = j * (slices + 1) + i;
            unsigned int tr = tl + 1;
            unsigned int bl = (j + 1) * (slices + 1) + i;
            unsigned int br = bl + 1;
            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }
    
    outVertexCount = (int)indices.size();
    
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute (location = 1) - smooth analytical normals for Phong shading
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord attribute (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return VAO;
}

// Draw the curved concrete barrier demonstrating Phong + Multiple Lights.
// The curved surface is placed:
//  - Below ceiling point lights (gets white diffuse + specular from above)
//  - Near entrance bar lights (gets warm golden specular from the side)
void drawCurvedBarrier(Shader& shader, unsigned int VAO, int vertexCount) {
    // Keep curved arc procedural-only so image textures (panel/display) never map onto it.
    shader.setInt("useTexture", 0);
    // --- Barrier 1: Driver-side center divider, near entrance ---
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(LOT_WIDTH/2.0f - 7.0f, 0.0f, LOT_DEPTH * 0.25f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    // Polished concrete: moderate specular so Phong highlights are visible
    shader.setVec3("objectColor",     glm::vec3(0.80f, 0.76f, 0.72f));
    shader.setInt("textureType",      0);     // concrete
    shader.setFloat("ambientStrength",  0.15f);
    shader.setFloat("diffuseStrength",  0.85f);
    shader.setFloat("specularStrength", 0.60f);
    shader.setFloat("shininess",        64.0f);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // --- Barrier 2: Passenger-side, symmetric mirror ---
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(LOT_WIDTH/2.0f + 7.0f, 0.0f, LOT_DEPTH * 0.25f));
    shader.setMat4("model", model);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ============================================================
// Texture Loading
// Applies GL_REPEAT wrapping and GL_LINEAR_MIPMAP_LINEAR filtering.
// ============================================================
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nChannels, 0);
    if (data) {
        GLenum format = (nChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        // GL_REPEAT: texture tiles when UVs exceed [0,1]
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Trilinear filtering: smooth between mipmap levels and within each level
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << nChannels << " ch)\n";
    } else {
        std::cerr << "Failed to load texture: " << path << "\n";
    }
    stbi_image_free(data);
    return textureID;
}

// ============================================================
// Sphere VAO (UV sphere)
// ============================================================
unsigned int createSphereVAO(int& outVertexCount) {
    const int stacks = 32;
    const int slices = 32;
    const float radius = 1.0f;
    const float PI = 3.14159265358979f;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; i++) {
        float phi   = PI * (float)i / stacks;          // 0 .. PI
        float sinP  = sin(phi);
        float cosP  = cos(phi);
        for (int j = 0; j <= slices; j++) {
            float theta = 2.0f * PI * (float)j / slices; // 0 .. 2PI
            float sinT  = sin(theta);
            float cosT  = cos(theta);
            // Vertex position on the unit sphere
            float x = sinP * cosT;
            float y = cosP;
            float z = sinP * sinT;
            // Normal = position (unit sphere)
            float u = (float)j / slices;
            float v = (float)i / stacks;
            vertices.insert(vertices.end(), {x * radius, y * radius, z * radius,
                                             x, y, z,  // analytical unit normal
                                             u, v});
        }
    }
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            unsigned int tl = i * (slices + 1) + j;
            unsigned int bl = (i + 1) * (slices + 1) + j;
            indices.push_back(tl);     indices.push_back(bl);     indices.push_back(tl + 1);
            indices.push_back(tl + 1); indices.push_back(bl);     indices.push_back(bl + 1);
        }
    }
    outVertexCount = (int)indices.size();

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return VAO;
}

// ============================================================
// Cone VAO (side surface only, open base)
// Positions + smooth side normals + UV coordinates
// Used for: textured cone showing Phong gradient on curved side
// ============================================================
unsigned int createConeVAO(int& outVertexCount) {
    const int slices = 36;
    const float radius = 0.8f;
    const float height = 2.0f;
    const float PI = 3.14159265358979f;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Side normals for a cone: outward slant (normal has a Y component = r/h)
    float slopeLen = sqrt(radius * radius + height * height);
    float nY = radius / slopeLen;   // upward tilt of the normal
    float nR = height / slopeLen;   // outward strength of the normal

    for (int i = 0; i <= slices; i++) {
        float angle = 2.0f * PI * (float)i / slices;
        float cosA  = cos(angle);
        float sinA  = sin(angle);
        // Base ring vertex
        vertices.insert(vertices.end(), {
            radius * cosA, 0.0f, radius * sinA,   // position (base)
            nR * cosA, nY, nR * sinA,              // smooth normal
            (float)i / slices, 0.0f                // uv
        });
        // Apex vertex (same u, v=1)
        vertices.insert(vertices.end(), {
            0.0f, height, 0.0f,                    // apex position
            nR * cosA, nY, nR * sinA,              // same outward normal
            (float)i / slices, 1.0f                // uv
        });
    }
    // Triangles: every quad on the side uses base[i], apex[i], base[i+1]
    for (int i = 0; i < slices; i++) {
        unsigned int base0 = i * 2;
        unsigned int apex0 = i * 2 + 1;
        unsigned int base1 = (i + 1) * 2;
        indices.push_back(base0); indices.push_back(base1); indices.push_back(apex0);
    }
    outVertexCount = (int)indices.size();

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return VAO;
}

// ============================================================
// Draw all textured demonstration objects
// Showcases all three texture modes + both curved objects
// ============================================================
void drawTexturedObjects(Shader& shader, unsigned int cubeVAO,
    unsigned int sphereVAO, int sphereCount,
    unsigned int coneVAO,   int coneCount,
    unsigned int texBrick, unsigned int texContainer)
{
    // Bind both texture units once
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texBrick);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texContainer);
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);

    // Share the current global texture mode
    shader.setInt("useTexture", useTexture);

    // ------------------------------------------------------------------
    // (a) SIMPLE TEXTURE – brick wall panel (mode 1 when active)
    // The image texture is used *directly* as the base color (no mixing).
    // ------------------------------------------------------------------
    {
        glm::mat4 model = glm::mat4(1.0f);
        // Stand a brick display panel against the right wall, near the barrier
        model = glm::translate(model, glm::vec3(LOT_WIDTH - 0.2f, 1.5f, LOT_DEPTH * 0.3f));
        model = glm::scale(model, glm::vec3(0.2f, 3.0f, 6.0f));
        shader.setMat4("model", model);
        shader.setVec3("objectColor", glm::vec3(0.9f, 0.88f, 0.86f));
        shader.setInt("textureType", 0);
        shader.setFloat("ambientStrength",  0.12f);
        shader.setFloat("diffuseStrength",  0.9f);
        shader.setFloat("specularStrength", 0.15f);
        shader.setFloat("shininess", 16.0f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    // ------------------------------------------------------------------
    // (b) BLENDED CUBE – storage container box
    // Initial color is RED. When Key 3 is pressed, the texture is blended 
    // with this red surface color in the fragment shader (using textureType 4).
    // ------------------------------------------------------------------
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(LOT_WIDTH/2.0f, 0.6f, LOT_DEPTH * 0.5f));
        model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.8f));
        shader.setMat4("model", model);
        shader.setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f)); // Red starting color
        shader.setInt("textureType", 4);     // Blended mode when texture is active
        shader.setFloat("ambientStrength",  0.12f);
        shader.setFloat("diffuseStrength",  0.85f);
        shader.setFloat("specularStrength", 0.30f);
        shader.setFloat("shininess", 32.0f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    // ------------------------------------------------------------------
    // (c) FRAGMENT-BLENDED TEXTURE – accent ramp/plinth near entrance (mode 3)
    // Blend factor is computed per-fragment from world Y height.
    // ------------------------------------------------------------------
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(LOT_WIDTH/2.0f + 2.0f, 0.0f, LOT_DEPTH - 4.0f));
        model = glm::scale(model, glm::vec3(3.5f, 3.5f, 0.3f));
        shader.setMat4("model", model);
        shader.setVec3("objectColor", glm::vec3(0.7f, 0.65f, 0.6f));
        shader.setInt("textureType", 0);
        shader.setFloat("ambientStrength",  0.12f);
        shader.setFloat("diffuseStrength",  0.85f);
        shader.setFloat("specularStrength", 0.40f);
        shader.setFloat("shininess", 48.0f);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    // ------------------------------------------------------------------
    // (d) TEXTURED SPHERE (curved object #1)
    // UV sphere with smooth radial normals. Brick texture.
    // Shows Phong specular highlight shifting as the viewer moves.
    // ------------------------------------------------------------------
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(LOT_WIDTH/2.0f - 3.0f, 1.0f, LOT_DEPTH * 0.4f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        shader.setMat4("model", model);
        shader.setVec3("objectColor", glm::vec3(0.9f, 0.88f, 0.85f));
        shader.setInt("textureType", 0);
        shader.setFloat("ambientStrength",  0.10f);
        shader.setFloat("diffuseStrength",  0.85f);
        shader.setFloat("specularStrength", 0.70f);
        shader.setFloat("shininess", 64.0f);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // ------------------------------------------------------------------
    // (e) TEXTURED CONE (curved object #2)
    // Cone with smooth side normals. Container texture.
    // Diffuse gradient visible from base (bright) to apex (dim).
    // ------------------------------------------------------------------
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(LOT_WIDTH/2.0f + 4.0f, 0.0f, LOT_DEPTH * 0.4f));
        shader.setMat4("model", model);
        shader.setVec3("objectColor", glm::vec3(0.85f, 0.8f, 0.75f));
        shader.setInt("textureType", 0);
        shader.setFloat("ambientStrength",  0.10f);
        shader.setFloat("diffuseStrength",  0.85f);
        shader.setFloat("specularStrength", 0.50f);
        shader.setFloat("shininess", 48.0f);
        glBindVertexArray(coneVAO);
        glDrawElements(GL_TRIANGLES, coneCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void drawStackedEmptyFloors(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO)
{
    glm::vec3 concreteColor(0.42f, 0.41f, 0.39f);
    glm::vec3 wallColor(0.38f, 0.37f, 0.36f);

    shader.setInt("useTexture", 0);

    for (int floor = 1; floor <= EXTRA_FLOORS; ++floor) {
        float yBase = floor * FLOOR_TO_FLOOR_HEIGHT;

        glm::mat4 floorModel = glm::mat4(1.0f);
        floorModel = glm::translate(floorModel, glm::vec3(LOT_WIDTH / 2.0f, yBase, LOT_DEPTH / 2.0f));
        floorModel = glm::scale(floorModel, glm::vec3(LOT_WIDTH, 1.0f, LOT_DEPTH));
        drawQuad(shader, quadVAO, floorModel, concreteColor, 0, 0.12f, 0.65f, 0.2f, 20.0f);

        float wallY = yBase + CEILING_HEIGHT * 0.5f;
        drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH / 2.0f, wallY, -WALL_THICKNESS / 2.0f),
                 glm::vec3(LOT_WIDTH, CEILING_HEIGHT, WALL_THICKNESS), wallColor, 0, 0.1f, 0.55f, 0.18f, 14.0f);
        drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH / 2.0f, wallY, LOT_DEPTH + WALL_THICKNESS / 2.0f),
                 glm::vec3(LOT_WIDTH, CEILING_HEIGHT, WALL_THICKNESS), wallColor, 0, 0.1f, 0.55f, 0.18f, 14.0f);
        drawCube(shader, cubeVAO, glm::vec3(-WALL_THICKNESS / 2.0f, wallY, LOT_DEPTH / 2.0f),
                 glm::vec3(WALL_THICKNESS, CEILING_HEIGHT, LOT_DEPTH), wallColor, 0, 0.1f, 0.55f, 0.18f, 14.0f);
        drawCube(shader, cubeVAO, glm::vec3(LOT_WIDTH + WALL_THICKNESS / 2.0f, wallY, LOT_DEPTH / 2.0f),
                 glm::vec3(WALL_THICKNESS, CEILING_HEIGHT, LOT_DEPTH), wallColor, 0, 0.1f, 0.55f, 0.18f, 14.0f);
    }
}

void drawElevatorSystem(Shader& shader, unsigned int cubeVAO)
{
    shader.setInt("useTexture", 0);

    float shaftHeight = EXTRA_FLOORS * FLOOR_TO_FLOOR_HEIGHT + CEILING_HEIGHT;
    float shaftWallY = shaftHeight * 0.5f;
    float shaftWallThickness = 0.12f;
    glm::vec3 shaftColor(0.30f, 0.31f, 0.33f);

    // Shaft outer shell (open at front)
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x, shaftWallY, elevator.shaftCenter.z - elevator.shaftDepth * 0.5f),
             glm::vec3(elevator.shaftWidth, shaftHeight, shaftWallThickness), shaftColor,
             3, 0.08f, 0.45f, 0.35f, 36.0f);
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x - elevator.shaftWidth * 0.5f, shaftWallY, elevator.shaftCenter.z),
             glm::vec3(shaftWallThickness, shaftHeight, elevator.shaftDepth), shaftColor,
             3, 0.08f, 0.45f, 0.35f, 36.0f);
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x + elevator.shaftWidth * 0.5f, shaftWallY, elevator.shaftCenter.z),
             glm::vec3(shaftWallThickness, shaftHeight, elevator.shaftDepth), shaftColor,
             3, 0.08f, 0.45f, 0.35f, 36.0f);

    // Cabin shell
    float cabinY = elevator.cabinY;
    float wallThickness = 0.06f;
    glm::vec3 cabinFrameColor(0.22f, 0.23f, 0.25f);
    glm::vec3 mirrorColor(0.78f, 0.80f, 0.85f);

    // Cabin floor/roof
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x, cabinY - elevator.cabinHeight * 0.5f + 0.03f, elevator.shaftCenter.z),
             glm::vec3(elevator.cabinWidth, 0.06f, elevator.cabinDepth),
             glm::vec3(0.35f, 0.35f, 0.37f), 3, 0.1f, 0.5f, 0.3f, 20.0f);
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x, cabinY + elevator.cabinHeight * 0.5f - 0.03f, elevator.shaftCenter.z),
             glm::vec3(elevator.cabinWidth, 0.06f, elevator.cabinDepth),
             cabinFrameColor, 3, 0.08f, 0.45f, 0.4f, 32.0f);

    // Mirror-finish interior surfaces (high specular + high shininess)
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x, cabinY, elevator.shaftCenter.z - elevator.cabinDepth * 0.5f + wallThickness * 0.5f),
             glm::vec3(elevator.cabinWidth - 0.05f, elevator.cabinHeight - 0.12f, wallThickness),
             mirrorColor, 5, 0.02f, 0.08f, 1.0f, 256.0f);
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x - elevator.cabinWidth * 0.5f + wallThickness * 0.5f, cabinY, elevator.shaftCenter.z),
             glm::vec3(wallThickness, elevator.cabinHeight - 0.12f, elevator.cabinDepth - 0.05f),
             mirrorColor, 5, 0.02f, 0.08f, 1.0f, 256.0f);
    drawCube(shader, cubeVAO,
             glm::vec3(elevator.shaftCenter.x + elevator.cabinWidth * 0.5f - wallThickness * 0.5f, cabinY, elevator.shaftCenter.z),
             glm::vec3(wallThickness, elevator.cabinHeight - 0.12f, elevator.cabinDepth - 0.05f),
             mirrorColor, 5, 0.02f, 0.08f, 1.0f, 256.0f);

    // Door geometry transform values (used later for transparent glass draw)
    float panelWidth = elevator.cabinWidth * 0.50f;
    float panelHeight = elevator.cabinHeight - 0.15f;
    float travel = elevator.doorTravel();
    float leftPanelX = elevator.shaftCenter.x - elevator.cabinWidth * 0.25f - travel;
    float rightPanelX = elevator.shaftCenter.x + elevator.cabinWidth * 0.25f + travel;
    float innerDoorZ = elevator.shaftCenter.z + elevator.cabinDepth * 0.5f - wallThickness;
    float outerDoorZ = elevator.shaftCenter.z + elevator.shaftDepth * 0.5f + wallThickness * 0.8f;

    // Button panel with texture mapping: wood base + elevator button overlay
    if (glad_glActiveTexture) {
        glad_glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texElevatorPanelBase);
        glad_glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texElevatorButtons);
    }
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);
    shader.setInt("useTexture", 4);

    glm::vec3 panelPos(elevator.shaftCenter.x + elevator.cabinWidth * 0.5f - 0.09f,
                       cabinY,
                       elevator.shaftCenter.z - 0.25f);
    drawCube(shader, cubeVAO, panelPos,
             glm::vec3(0.035f, 1.35f, 0.48f),
             glm::vec3(1.0f, 1.0f, 1.0f), 4, 0.12f, 0.72f, 0.3f, 24.0f);

    // LED 7-segment display: texture mapped from src/seven-segments/<digit>.png
    int displayDigit = elevator.targetFloor;
    if (displayDigit < 0) displayDigit = 0;
    if (displayDigit > 9) displayDigit = 9;

    if (glad_glActiveTexture) {
        glad_glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texSevenSegment[displayDigit]);
    }
    shader.setInt("texture1", 0);
    shader.setInt("useTexture", 5);

    // Place display inside the elevator above the inner front door so it stays in clear view.
    glm::vec3 ledPos(elevator.shaftCenter.x,
                     cabinY + elevator.cabinHeight * 0.33f,
                     elevator.shaftCenter.z + elevator.cabinDepth * 0.5f - 0.05f);
    drawCube(shader, cubeVAO, ledPos,
             glm::vec3(0.95f, 0.34f, 0.015f),
             glm::vec3(1.0f, 1.0f, 1.0f), 6, 0.8f, 0.2f, 0.1f, 8.0f);

    // Transparent front gate/door (glass-like), drawn last for correct blending.
    shader.setInt("useTexture", 0);
    glm::vec3 doorGlassColor(0.75f, 0.86f, 0.95f);

    glDepthMask(GL_FALSE);
    drawCubeAlpha(shader, cubeVAO, glm::vec3(leftPanelX, cabinY, innerDoorZ),
             glm::vec3(panelWidth, panelHeight, wallThickness), doorGlassColor,
             4, 0.05f, 0.15f, 1.0f, 220.0f, 0.30f);
    drawCubeAlpha(shader, cubeVAO, glm::vec3(rightPanelX, cabinY, innerDoorZ),
             glm::vec3(panelWidth, panelHeight, wallThickness), doorGlassColor,
             4, 0.05f, 0.15f, 1.0f, 220.0f, 0.30f);

    drawCubeAlpha(shader, cubeVAO, glm::vec3(leftPanelX, cabinY, outerDoorZ),
             glm::vec3(panelWidth, panelHeight, wallThickness), doorGlassColor,
             4, 0.05f, 0.15f, 1.0f, 220.0f, 0.30f);
    drawCubeAlpha(shader, cubeVAO, glm::vec3(rightPanelX, cabinY, outerDoorZ),
             glm::vec3(panelWidth, panelHeight, wallThickness), doorGlassColor,
             4, 0.05f, 0.15f, 1.0f, 220.0f, 0.30f);
    glDepthMask(GL_TRUE);

    shader.setFloat("objectAlpha", 1.0f);
    shader.setInt("useTexture", 0);
    if (glad_glActiveTexture) {
        glad_glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glad_glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glad_glActiveTexture(GL_TEXTURE0);
    }
}

void drawCubeAlpha(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess, float alpha)
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
    shader.setFloat("objectAlpha", alpha);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    shader.setFloat("objectAlpha", 1.0f);
}

void setupLighting(Shader& shader)
{
    shader.setFloat("objectAlpha", 1.0f);
    // Pass general light status to shader for ambient/emissive components
    shader.setBool("lightsOn", ceilingLightsOn || entranceLightsOn);

    // Window positions for volumetric scattering (4 windows)
    shader.setInt("numWindows", 4);
    float windowSpacing = 8.0f;
    for (int i = 0; i < 4; i++) {
        float z = windowSpacing/2 + i * windowSpacing;
        std::string base = "windowPositions[" + std::to_string(i) + "]";
        shader.setVec3(base, glm::vec3(0.0f, 2.5f, z));
    }

    // Unified Sun Direction (from window towards parking lot)
    glm::vec3 sunDir = glm::normalize(glm::vec3(1.0f, -0.6f, 0.4f)); 
    shader.setVec3("sunDirection", sunDir);
    
    // Directional light (sunlight through windows)
    shader.setBool("useDirLight", true);
    shader.setVec3("dirLight.direction", sunDir);
    
    // Base ambient when everything is off
    if (!ceilingLightsOn && !entranceLightsOn) {
        shader.setVec3("dirLight.ambient", glm::vec3(0.03f, 0.035f, 0.04f));
        shader.setVec3("dirLight.diffuse", glm::vec3(0.15f, 0.14f, 0.12f));
        shader.setVec3("dirLight.specular", glm::vec3(0.1f, 0.1f, 0.08f));
    } else {
        shader.setVec3("dirLight.ambient", glm::vec3(0.25f, 0.22f, 0.18f));
        shader.setVec3("dirLight.diffuse", glm::vec3(1.2f, 1.1f, 0.9f));
        shader.setVec3("dirLight.specular", glm::vec3(1.2f, 1.15f, 1.0f));
    }

    int totalPointLights = 0;

    // --- Ceiling Lights (Indices 0-11) ---
    if (ceilingLightsOn) {
        float lightSpacingX = LOT_WIDTH / 4.0f;
        float lightSpacingZ = LOT_DEPTH / 3.0f;
        int lightIdx = 0;
        for (int i = 0; i < 4 && lightIdx < 12; i++) {
            for (int j = 0; j < 3 && lightIdx < 12; j++) {
                float x = lightSpacingX * (i + 0.5f);
                float z = lightSpacingZ * (j + 0.5f);
                std::string base = "pointLights[" + std::to_string(lightIdx) + "]";
                shader.setVec3(base + ".position", glm::vec3(x, CEILING_HEIGHT - 0.3f, z));
                shader.setVec3(base + ".ambient", glm::vec3(0.18f, 0.16f, 0.12f));
                shader.setVec3(base + ".diffuse", glm::vec3(1.4f, 1.3f, 1.1f));
                shader.setVec3(base + ".specular", glm::vec3(1.3f, 1.2f, 1.0f));
                shader.setFloat(base + ".constant", 1.0f);
                shader.setFloat(base + ".linear", 0.09f);
                shader.setFloat(base + ".quadratic", 0.032f);
                lightIdx++;
            }
        }
        totalPointLights = 12;
    }

    // --- Entrance Rod Lights (Indices 12-13) ---
    if (entranceLightsOn) {
        // Left rod light
        std::string rodLeft = "pointLights[12]";
        shader.setVec3(rodLeft + ".position", glm::vec3(LOT_WIDTH/2 - 6.0f, 1.5f, LOT_DEPTH - 1.0f));
        shader.setVec3(rodLeft + ".ambient", glm::vec3(0.15f, 0.12f, 0.08f));
        shader.setVec3(rodLeft + ".diffuse", glm::vec3(1.3f, 1.0f, 0.7f));
        shader.setVec3(rodLeft + ".specular", glm::vec3(1.2f, 1.0f, 0.8f));
        shader.setFloat(rodLeft + ".constant", 1.0f);
        shader.setFloat(rodLeft + ".linear", 0.09f);
        shader.setFloat(rodLeft + ".quadratic", 0.032f);
        
        // Right rod light
        std::string rodRight = "pointLights[13]";
        shader.setVec3(rodRight + ".position", glm::vec3(LOT_WIDTH/2 + 6.0f, 1.5f, LOT_DEPTH - 1.0f));
        shader.setVec3(rodRight + ".ambient", glm::vec3(0.15f, 0.12f, 0.08f));
        shader.setVec3(rodRight + ".diffuse", glm::vec3(1.3f, 1.0f, 0.7f));
        shader.setVec3(rodRight + ".specular", glm::vec3(1.2f, 1.0f, 0.8f));
        shader.setFloat(rodRight + ".constant", 1.0f);
        shader.setFloat(rodRight + ".linear", 0.09f);
        shader.setFloat(rodRight + ".quadratic", 0.032f);
        
        if (totalPointLights < 14) totalPointLights = 14; 
    }

    // If one system is off but the other is on, we still need to make sure the "off" lights aren't contributing
    // Actually, shader expects numPointLights. We should probably only pass the lights that are ON, 
    // BUT the indices are fixed in my logic (12-13 for entrance).
    // Let's adjust to pass all 14 and zero out the ones that are off if necessary, 
    // OR just set numPointLights to 14 and ensure the "off" ones have 0 intensity.

    if (!ceilingLightsOn) {
        for (int i = 0; i < 12; i++) {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            shader.setVec3(base + ".diffuse", glm::vec3(0.0f));
            shader.setVec3(base + ".specular", glm::vec3(0.0f));
            shader.setVec3(base + ".ambient", glm::vec3(0.0f));
        }
    }
    if (!entranceLightsOn) {
        for (int i = 12; i < 14; i++) {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            shader.setVec3(base + ".diffuse", glm::vec3(0.0f));
            shader.setVec3(base + ".specular", glm::vec3(0.0f));
            shader.setVec3(base + ".ambient", glm::vec3(0.0f));
        }
    }

    shader.setInt("numPointLights", (ceilingLightsOn || entranceLightsOn) ? 14 : 0);
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
    shader.setFloat("objectAlpha", 1.0f);
    
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
    shader.setFloat("objectAlpha", 1.0f);
    
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
    shader.setFloat("objectAlpha", 1.0f);
    
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
    shader.setFloat("objectAlpha", 1.0f);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawParkingLot(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO, unsigned int wedgeVAO)
{
    glm::vec3 concreteColor(0.45f, 0.43f, 0.40f);
    glm::vec3 ceilingColor(0.35f, 0.33f, 0.30f);
    glm::vec3 lineColor(0.95f, 0.95f, 0.90f);
    glm::vec3 yellowColor(0.95f, 0.85f, 0.2f);
    
    // === FLOOR ===
    shader.setInt("useTexture", useTexture); // Allow floor texture to change
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(LOT_WIDTH/2, 0.0f, LOT_DEPTH/2));
    floorModel = glm::scale(floorModel, glm::vec3(LOT_WIDTH, 1.0f, LOT_DEPTH));
    drawQuad(shader, quadVAO, floorModel, concreteColor, 0, 0.15f, 0.7f, 0.2f, 16.0f);
    
    // === CEILING ===
    shader.setInt("useTexture", 0); // Ceiling remains starting state
    glm::mat4 ceilingModel = glm::mat4(1.0f);
    ceilingModel = glm::translate(ceilingModel, glm::vec3(LOT_WIDTH/2, CEILING_HEIGHT, LOT_DEPTH/2));
    ceilingModel = glm::rotate(ceilingModel, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ceilingModel = glm::scale(ceilingModel, glm::vec3(LOT_WIDTH, 1.0f, LOT_DEPTH));
    drawQuad(shader, quadVAO, ceilingModel, ceilingColor, 0, 0.1f, 0.5f, 0.1f, 8.0f);
    
    // === WALLS ===
    shader.setInt("useTexture", useTexture); // Allow walls texture to change
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
    shader.setInt("useTexture", 0); // Pillars remain starting state
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
    for (int i = 0; i < 10; i++) {
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
    for (int i = 0; i < 10; i++) {
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
    for (int i = 0; i < 8; i++) {
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
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 4; j++) {
            float x = (LOT_WIDTH/5) * (i + 0.5f);
            float z = (LOT_DEPTH/4) * (j + 0.5f);
            // Light housing
            drawCube(shader, cubeVAO, glm::vec3(x, CEILING_HEIGHT - 0.15f, z),
                     glm::vec3(1.2f, 0.1f, 0.4f), glm::vec3(0.8f, 0.8f, 0.82f), 3, 0.2f, 0.6f, 0.7f, 48.0f);
            // Light panel (emissive look)
            glm::vec3 panelColor = ceilingLightsOn ? glm::vec3(1.0f, 0.98f, 0.9f) : glm::vec3(0.2f, 0.18f, 0.15f);
            drawCube(shader, cubeVAO, glm::vec3(x, CEILING_HEIGHT - 0.22f, z),
                     glm::vec3(1.0f, 0.05f, 0.3f), panelColor, 4, 
                     ceilingLightsOn ? 0.9f : 0.1f, 0.3f, 0.2f, 8.0f);
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
             
    // Attendant Booth (New Feature)
    drawAttendantBooth(shader, cubeVAO, glm::vec3(LOT_WIDTH/2 - 5.0f, 0.0f, LOT_DEPTH - 3.0f));
}

// Draw a realistic car using the procedural generator
void drawRealisticCar(Shader& shader, unsigned int cubeVAO, unsigned int cylVAO, unsigned int wedgeVAO, glm::vec3 position, glm::vec3 carColor, float rotation)
{
    // Initialize the procedural car generator lazily (once)
    // This ensures VBOs are created after OpenGL context is set up
    static ProceduralSedan sedan;
    
    // Render the high-fidelity car
    sedan.render(shader, position, rotation, carColor);
}

// Draw the parking lot attendant booth with realistic human inside
void drawAttendantBooth(Shader& shader, unsigned int cubeVAO, glm::vec3 position) {
    // === BOOTH STRUCTURE ===
    float boothWidth = 2.0f;
    float boothHeight = 2.5f;
    float boothDepth = 2.0f;
    glm::vec3 wallColor(0.8f, 0.85f, 0.9f); // Light blueish white
    glm::vec3 glassColor(0.1f, 0.2f, 0.3f);
    glm::vec3 metalColor(0.3f, 0.3f, 0.35f);
    
    // Floor
    drawCube(shader, cubeVAO, position + glm::vec3(0, 0.1f, 0), glm::vec3(boothWidth, 0.2f, boothDepth), 
             glm::vec3(0.4f, 0.4f, 0.45f), 1, 0.3f, 0.6f, 0.4f, 16.0f);
             
    // Roof
    drawCube(shader, cubeVAO, position + glm::vec3(0, boothHeight, 0), glm::vec3(boothWidth+0.4f, 0.2f, boothDepth+0.4f), 
             glm::vec3(0.2f, 0.25f, 0.3f), 1, 0.3f, 0.6f, 0.4f, 16.0f);
             
    // Pillars (Corners)
    float pSize = 0.2f;
    drawCube(shader, cubeVAO, position + glm::vec3(-boothWidth/2+pSize/2, boothHeight/2, -boothDepth/2+pSize/2), 
             glm::vec3(pSize, boothHeight, pSize), metalColor, 3, 0.2f, 0.6f, 0.5f, 32.0f);
    drawCube(shader, cubeVAO, position + glm::vec3(boothWidth/2-pSize/2, boothHeight/2, -boothDepth/2+pSize/2), 
             glm::vec3(pSize, boothHeight, pSize), metalColor, 3, 0.2f, 0.6f, 0.5f, 32.0f);
    drawCube(shader, cubeVAO, position + glm::vec3(-boothWidth/2+pSize/2, boothHeight/2, boothDepth/2-pSize/2), 
             glm::vec3(pSize, boothHeight, pSize), metalColor, 3, 0.2f, 0.6f, 0.5f, 32.0f);
    drawCube(shader, cubeVAO, position + glm::vec3(boothWidth/2-pSize/2, boothHeight/2, boothDepth/2-pSize/2), 
             glm::vec3(pSize, boothHeight, pSize), metalColor, 3, 0.2f, 0.6f, 0.5f, 32.0f);
             
    // Walls (Lower half opaque)
    float lowWallH = 1.0f;
    drawCube(shader, cubeVAO, position + glm::vec3(0, lowWallH/2, -boothDepth/2+0.05f), 
             glm::vec3(boothWidth-0.1f, lowWallH, 0.1f), wallColor, 2, 0.3f, 0.7f, 0.2f, 8.0f); // Back
    drawCube(shader, cubeVAO, position + glm::vec3(-boothWidth/2+0.05f, lowWallH/2, 0), 
             glm::vec3(0.1f, lowWallH, boothDepth-0.1f), wallColor, 2, 0.3f, 0.7f, 0.2f, 8.0f); // Left
    drawCube(shader, cubeVAO, position + glm::vec3(boothWidth/2-0.05f, lowWallH/2, 0), 
             glm::vec3(0.1f, lowWallH, boothDepth-0.1f), wallColor, 2, 0.3f, 0.7f, 0.2f, 8.0f); // Right
    drawCube(shader, cubeVAO, position + glm::vec3(0, lowWallH/2, boothDepth/2-0.05f), 
             glm::vec3(boothWidth-0.1f, lowWallH, 0.1f), wallColor, 2, 0.3f, 0.7f, 0.2f, 8.0f); // Front
             
    // Windows (Upper half glass)
    float winH = boothHeight - lowWallH - 0.2f;
    float winY = lowWallH + winH/2;
    // Front Window
    drawCube(shader, cubeVAO, position + glm::vec3(0, winY, boothDepth/2-0.05f), 
             glm::vec3(boothWidth-0.4f, winH, 0.05f), glassColor, 4, 0.1f, 0.3f, 0.9f, 64.0f);
    // Side Windows
    drawCube(shader, cubeVAO, position + glm::vec3(-boothWidth/2+0.05f, winY, 0), 
             glm::vec3(0.05f, winH, boothDepth-0.4f), glassColor, 4, 0.1f, 0.3f, 0.9f, 64.0f);
    drawCube(shader, cubeVAO, position + glm::vec3(boothWidth/2-0.05f, winY, 0), 
             glm::vec3(0.05f, winH, boothDepth-0.4f), glassColor, 4, 0.1f, 0.3f, 0.9f, 64.0f);
             
    // Desk inside
    drawCube(shader, cubeVAO, position + glm::vec3(0, 0.9f, 0.4f), 
             glm::vec3(1.8f, 0.05f, 0.6f), glm::vec3(0.4f, 0.25f, 0.15f), 1, 0.2f, 0.6f, 0.3f, 16.0f);
             
    // === HUMAN ATTENDANT ===
    static ProceduralHuman attendant;
    // Sitting position: behind desk, slightly elevated chair
    glm::vec3 humanPos = position + glm::vec3(0, 1.4f, -0.2f);
    attendant.render(shader, humanPos, 180.0f); // Facing front (toward +Z presumably if entrance is there? No, usually looks nicely)
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
    
    // === ENTRANCE ROD LIGHTS ===
    // Vertical rod lights on both sides of the entrance
    // These create warm illumination with proper Phong shading
    
    glm::vec3 rodHousing(0.2f, 0.2f, 0.22f);      // Dark metal housing
    glm::vec3 rodLightOn(1.0f, 0.85f, 0.55f);     // Warm golden glow when ON
    glm::vec3 rodLightOff(0.15f, 0.12f, 0.1f);    // Dim when OFF
    glm::vec3 rodLight = entranceLightsOn ? rodLightOn : rodLightOff;
    
    float rodX1 = LOT_WIDTH/2 - 6.0f;
    float rodX2 = LOT_WIDTH/2 + 6.0f;
    float rodZ = LOT_DEPTH - 1.0f;
    float rodHeight = 2.5f;
    float rodRadius = 0.06f;
    
    // Left rod light fixture
    // Base
    drawCube(shader, cubeVAO, glm::vec3(rodX1, 0.1f, rodZ),
             glm::vec3(0.2f, 0.2f, 0.2f), rodHousing, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    // Vertical pole
    drawCube(shader, cubeVAO, glm::vec3(rodX1, rodHeight/2, rodZ),
             glm::vec3(rodRadius * 2, rodHeight, rodRadius * 2), rodHousing, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    // Light tube (glowing) - facing INWARD toward parking lot
    drawCube(shader, cubeVAO, glm::vec3(rodX1, rodHeight/2, rodZ - rodRadius * 3),
             glm::vec3(rodRadius * 1.5f, rodHeight * 0.7f, rodRadius * 1.5f), rodLight, 1, 
             entranceLightsOn ? 0.9f : 0.15f, entranceLightsOn ? 0.3f : 0.5f, entranceLightsOn ? 0.1f : 0.3f, 8.0f);
    // Top cap
    drawCube(shader, cubeVAO, glm::vec3(rodX1, rodHeight + 0.05f, rodZ),
             glm::vec3(0.15f, 0.1f, 0.15f), rodHousing, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    
    // Right rod light fixture
    // Base
    drawCube(shader, cubeVAO, glm::vec3(rodX2, 0.1f, rodZ),
             glm::vec3(0.2f, 0.2f, 0.2f), rodHousing, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    // Vertical pole
    drawCube(shader, cubeVAO, glm::vec3(rodX2, rodHeight/2, rodZ),
             glm::vec3(rodRadius * 2, rodHeight, rodRadius * 2), rodHousing, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    // Light tube (glowing) - facing INWARD toward parking lot
    drawCube(shader, cubeVAO, glm::vec3(rodX2, rodHeight/2, rodZ - rodRadius * 3),
             glm::vec3(rodRadius * 1.5f, rodHeight * 0.7f, rodRadius * 1.5f), rodLight, 1, 
             entranceLightsOn ? 0.9f : 0.15f, entranceLightsOn ? 0.3f : 0.5f, entranceLightsOn ? 0.1f : 0.3f, 8.0f);
    // Top cap
    drawCube(shader, cubeVAO, glm::vec3(rodX2, rodHeight + 0.05f, rodZ),
             glm::vec3(0.15f, 0.1f, 0.15f), rodHousing, 3, 0.1f, 0.5f, 0.6f, 32.0f);
    
    // Ground light pools from rod lights (when ON)
    if (entranceLightsOn) {
        glm::vec3 poolColor(1.0f, 0.85f, 0.55f);
        drawCube(shader, cubeVAO, glm::vec3(rodX1, 0.015f, rodZ + 0.5f),
                 glm::vec3(2.0f, 0.02f, 2.5f), poolColor * 0.2f, 1, 0.6f, 0.4f, 0.1f, 4.0f);
        drawCube(shader, cubeVAO, glm::vec3(rodX2, 0.015f, rodZ + 0.5f),
                 glm::vec3(2.0f, 0.02f, 2.5f), poolColor * 0.2f, 1, 0.6f, 0.4f, 0.1f, 4.0f);
    }
}

// Helper function to draw the entire scene
void drawScene(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int cylVAO, unsigned int wedgeVAO) {
    // Reset texture mode to 0 (default starting state) for scene objects that shouldn't change
    shader.setInt("useTexture", 0);

    // Draw outdoor environment first
    drawOutdoorEnvironment(shader, cubeVAO, quadVAO, cylVAO);

    // Draw additional empty floors stacked above ground floor
    drawStackedEmptyFloors(shader, cubeVAO, quadVAO);

    // Draw the parking lot
    drawParkingLot(shader, cubeVAO, quadVAO, cylVAO, wedgeVAO);

    // Draw interactive elevator system at corner
    drawElevatorSystem(shader, cubeVAO);
    
    // Draw parking lot signage and safety features
    drawParkingSignage(shader, cubeVAO, cylVAO);
    // Note: drawCurvedBarrier is called separately from main() because it needs curvedBarrierVAO
}
