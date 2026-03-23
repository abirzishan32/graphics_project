#ifndef ESCALATOR_H
#define ESCALATOR_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#include "shader.h"
#include "basic_camera.h"

// Draw helpers implemented in main.cpp
void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawCubeAlpha(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
                   glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess, float alpha);
void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation,
                     glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawQuad(Shader& shader, unsigned int VAO, glm::mat4 model,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);

namespace EscalatorSystem {

struct Layout {
    float firstFloorY;
    float mallRightWallX;
    float connectZ;

    float topX;
    float baseX;
    float run;
    float rise;
    float angleDeg;
    float length;

    float laneOffsetZ;
    float laneWidth;

    float gateX;
    float pathStartX;
};

inline Layout computeLayout(float floorToFloorHeight, float lotWidth, float lotDepth, float wallThickness) {
    Layout l;
    l.firstFloorY = floorToFloorHeight;
    l.mallRightWallX = lotWidth + wallThickness * 0.5f;
    l.connectZ = lotDepth * 0.78f;

    l.topX = l.mallRightWallX + 6.4f;
    l.baseX = lotWidth + 30.0f;
    l.run = l.baseX - l.topX;
    l.rise = l.firstFloorY;
    l.angleDeg = glm::degrees(std::atan2(l.rise, l.run));
    l.length = std::sqrt(l.run * l.run + l.rise * l.rise);

    l.laneOffsetZ = 1.45f;
    l.laneWidth = 2.4f;

    l.gateX = l.mallRightWallX + 0.35f;
    l.pathStartX = l.baseX + 11.0f;
    return l;
}

inline unsigned int createBillboardVAO() {
    float vertices[] = {
        -0.5f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         0.5f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         0.5f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
    };

    unsigned int vbo, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return vao;
}

inline void updateRide(BasicCamera& camera, float deltaTime,
                       float floorToFloorHeight, float lotWidth, float lotDepth, float wallThickness) {
    Layout l = computeLayout(floorToFloorHeight, lotWidth, lotDepth, wallThickness);

    const float riderSpeed = 2.2f;
    const float slopeYPerX = l.rise / l.run;
    const float topReleaseX = l.topX + 0.95f;
    const float bottomReleaseX = l.baseX - 0.95f;

    auto tryRideLane = [&](float laneZ, bool upLane) {
        float progress = (l.baseX - camera.Position.x) / l.run;
        bool withinRampX = camera.Position.x >= (l.topX - 0.15f) && camera.Position.x <= (l.baseX + 0.15f);
        bool nearLane = std::abs(camera.Position.z - laneZ) <= (l.laneWidth * 0.55f);
        bool nearExpectedY = std::abs(camera.Position.y - (1.7f + glm::clamp(progress, 0.0f, 1.0f) * l.rise)) < 1.0f;

        if (!(withinRampX && nearLane && nearExpectedY)) {
            return;
        }

        // Release zones: once rider reaches the top/bottom handoff area,
        // stop forcing escalator motion so W can move freely into/out of mall.
        if (upLane && camera.Position.x <= topReleaseX) {
            return;
        }
        if (!upLane && camera.Position.x >= bottomReleaseX) {
            return;
        }

        if (upLane) {
            camera.Position.x -= riderSpeed * deltaTime;
            camera.Position.y += riderSpeed * slopeYPerX * deltaTime;
            if (camera.Position.x < topReleaseX) {
                camera.Position.x = topReleaseX;
                camera.Position.y = 1.7f + l.rise;
            }
        } else {
            camera.Position.x += riderSpeed * deltaTime;
            camera.Position.y -= riderSpeed * slopeYPerX * deltaTime;
            if (camera.Position.x > bottomReleaseX) {
                camera.Position.x = bottomReleaseX;
                camera.Position.y = 1.7f;
            }
        }

        camera.Position.z = glm::mix(camera.Position.z, laneZ, glm::clamp(deltaTime * 8.0f, 0.0f, 1.0f));
        camera.EyeHeight = camera.Position.y;
    };

    tryRideLane(l.connectZ - l.laneOffsetZ, true);
    tryRideLane(l.connectZ + l.laneOffsetZ, false);
}

inline void draw(Shader& shader, unsigned int cubeVAO, unsigned int quadVAO, unsigned int billboardVAO,
                 float floorToFloorHeight, float lotWidth, float lotDepth, float wallThickness) {
    Layout l = computeLayout(floorToFloorHeight, lotWidth, lotDepth, wallThickness);

    shader.setInt("useTexture", 0);

    // 1) Green terrain
    glm::mat4 terrainModel = glm::mat4(1.0f);
    terrainModel = glm::translate(terrainModel, glm::vec3(lotWidth + 20.0f, -0.06f, lotDepth * 0.84f));
    terrainModel = glm::scale(terrainModel, glm::vec3(56.0f, 1.0f, 50.0f));
    drawQuad(shader, quadVAO, terrainModel,
             glm::vec3(0.18f, 0.64f, 0.23f), 0, 0.25f, 0.75f, 0.08f, 6.0f);

    // 2) Pedestrian path in front of escalator
    float pathLen = l.pathStartX - l.baseX;
    glm::mat4 pathModel = glm::mat4(1.0f);
    pathModel = glm::translate(pathModel, glm::vec3(l.baseX + pathLen * 0.5f, 0.015f, l.connectZ));
    pathModel = glm::scale(pathModel, glm::vec3(pathLen, 1.0f, 6.5f));
    drawQuad(shader, quadVAO, pathModel,
             glm::vec3(0.20f, 0.20f, 0.22f), 0, 0.12f, 0.55f, 0.2f, 20.0f);

    // 3) Branch road from parking-lot front road
    float roadJunctionZ = lotDepth + 9.5f;
    float connectorX = l.baseX + 8.0f;
    float branchStartX = lotWidth * 0.62f;
    float branchWidth = connectorX - branchStartX;
    glm::mat4 branchH = glm::mat4(1.0f);
    branchH = glm::translate(branchH, glm::vec3(branchStartX + branchWidth * 0.5f, 0.013f, roadJunctionZ));
    branchH = glm::scale(branchH, glm::vec3(branchWidth, 1.0f, 4.0f));
    drawQuad(shader, quadVAO, branchH,
             glm::vec3(0.22f, 0.22f, 0.24f), 0, 0.12f, 0.56f, 0.2f, 20.0f);

    float branchZLen = l.connectZ - roadJunctionZ;
    glm::mat4 branchV = glm::mat4(1.0f);
    branchV = glm::translate(branchV, glm::vec3(connectorX, 0.013f, roadJunctionZ + branchZLen * 0.5f));
    branchV = glm::scale(branchV, glm::vec3(4.0f, 1.0f, branchZLen));
    drawQuad(shader, quadVAO, branchV,
             glm::vec3(0.22f, 0.22f, 0.24f), 0, 0.12f, 0.56f, 0.2f, 20.0f);

    auto drawEscalatorLane = [&](float laneZ, bool upDirection) {
        glm::vec3 rampPos(l.topX + l.run * 0.5f, l.rise * 0.5f, laneZ);

        // Support + ramp are intentionally same tilt so they are not reversed.
        drawCubeRotated(shader, cubeVAO, rampPos,
                        glm::vec3(l.length, 0.38f, l.laneWidth),
                        glm::vec3(0.0f, 0.0f, -l.angleDeg),
                        glm::vec3(0.36f, 0.37f, 0.40f), 3, 0.08f, 0.48f, 0.55f, 64.0f);

        drawCubeRotated(shader, cubeVAO, rampPos + glm::vec3(0.0f, 0.55f, -l.laneWidth * 0.5f),
                        glm::vec3(l.length, 1.1f, 0.07f),
                        glm::vec3(0.0f, 0.0f, -l.angleDeg),
                        glm::vec3(0.62f, 0.67f, 0.74f), 3, 0.08f, 0.42f, 0.75f, 96.0f);
        drawCubeRotated(shader, cubeVAO, rampPos + glm::vec3(0.0f, 0.55f, l.laneWidth * 0.5f),
                        glm::vec3(l.length, 1.1f, 0.07f),
                        glm::vec3(0.0f, 0.0f, -l.angleDeg),
                        glm::vec3(0.62f, 0.67f, 0.74f), 3, 0.08f, 0.42f, 0.75f, 96.0f);

        const int stepCount = 24;
        float motion = std::fmod((float)glfwGetTime() * 0.45f, 1.0f);
        glm::vec3 stepColor(0.55f, 0.58f, 0.62f);

        for (int i = 0; i < stepCount; ++i) {
            float u = (float)i / (float)stepCount;
            float p = std::fmod(u + motion, 1.0f);
            float s = upDirection ? p : (1.0f - p);

            float x = l.baseX - s * l.run;
            float y = s * l.rise + 0.20f;

            drawCubeRotated(shader, cubeVAO,
                            glm::vec3(x, y, laneZ),
                            glm::vec3(0.58f, 0.12f, l.laneWidth - 0.30f),
                            glm::vec3(0.0f, 0.0f, -l.angleDeg),
                            stepColor, 6, 0.1f, 0.56f, 0.8f, 120.0f);
        }
    };

    // Left lane up, right lane down (as seen from outside looking to mall)
    drawEscalatorLane(l.connectZ - l.laneOffsetZ, true);
    drawEscalatorLane(l.connectZ + l.laneOffsetZ, false);

    // 4) Buffer/open space on first floor after escalator exit
    float exitPadX = l.topX - 0.9f;
    drawCube(shader, cubeVAO,
             glm::vec3(exitPadX, l.firstFloorY + 0.08f, l.connectZ),
             glm::vec3(2.2f, 0.16f, 6.0f),
             glm::vec3(0.28f, 0.29f, 0.31f), 3, 0.1f, 0.5f, 0.35f, 40.0f);

    float openSpaceCenterX = (l.gateX + 1.2f + l.topX - 2.0f) * 0.5f;
    float openSpaceWidth = std::max(2.0f, (l.topX - 2.0f) - (l.gateX + 1.2f));
    drawCube(shader, cubeVAO,
             glm::vec3(openSpaceCenterX, l.firstFloorY + 0.08f, l.connectZ),
             glm::vec3(openSpaceWidth, 0.16f, 6.0f),
             glm::vec3(0.30f, 0.31f, 0.33f), 3, 0.1f, 0.5f, 0.35f, 40.0f);

    // 5) Main gate: glass-like door at first floor wall connection
    drawCube(shader, cubeVAO,
             glm::vec3(l.gateX + 0.18f, l.firstFloorY + 1.35f, l.connectZ),
             glm::vec3(0.20f, 2.6f, 4.2f),
             glm::vec3(0.80f, 0.82f, 0.86f), 3, 0.10f, 0.45f, 0.35f, 48.0f);

    glDepthMask(GL_FALSE);
    drawCubeAlpha(shader, cubeVAO,
                  glm::vec3(l.gateX + 0.28f, l.firstFloorY + 1.35f, l.connectZ - 1.0f),
                  glm::vec3(0.035f, 2.2f, 1.9f), glm::vec3(0.74f, 0.87f, 0.97f),
                  4, 0.05f, 0.15f, 1.0f, 220.0f, 0.32f);
    drawCubeAlpha(shader, cubeVAO,
                  glm::vec3(l.gateX + 0.28f, l.firstFloorY + 1.35f, l.connectZ + 1.0f),
                  glm::vec3(0.035f, 2.2f, 1.9f), glm::vec3(0.74f, 0.87f, 0.97f),
                  4, 0.05f, 0.15f, 1.0f, 220.0f, 0.32f);
    glDepthMask(GL_TRUE);

    // 6) Billboard rows (UV-ready)
    shader.setInt("useTexture", 0);
    shader.setInt("textureType", 3);
    shader.setVec3("objectColor", glm::vec3(0.92f, 0.92f, 0.94f));
    shader.setFloat("ambientStrength", 0.16f);
    shader.setFloat("diffuseStrength", 0.68f);
    shader.setFloat("specularStrength", 0.25f);
    shader.setFloat("shininess", 22.0f);

    float posterZs[2] = { l.connectZ - 4.6f, l.connectZ + 4.6f };
    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < 3; ++i) {
            float x = l.pathStartX - 2.6f - i * 3.2f;

            glm::mat4 panelModel = glm::mat4(1.0f);
            panelModel = glm::translate(panelModel, glm::vec3(x, 0.25f, posterZs[side]));
            panelModel = glm::rotate(panelModel,
                                     glm::radians(side == 0 ? 180.0f : 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
            panelModel = glm::scale(panelModel, glm::vec3(2.0f, 2.8f, 1.0f));
            shader.setMat4("model", panelModel);
            glBindVertexArray(billboardVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            drawCube(shader, cubeVAO,
                     glm::vec3(x, 1.1f, posterZs[side]),
                     glm::vec3(0.12f, 2.2f, 0.12f),
                     glm::vec3(0.42f, 0.42f, 0.45f), 3, 0.1f, 0.5f, 0.35f, 32.0f);
        }
    }

    glBindVertexArray(0);
}

} // namespace EscalatorSystem

#endif
