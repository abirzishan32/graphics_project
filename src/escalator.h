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
                 unsigned int texBillboardAd,
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

    // Detailed escalator proportions
    float landingLen = 2.3f;
    float inclineRun = std::max(2.5f, l.run - 2.0f * landingLen);
    float inclineRise = l.rise;
    float inclineLen = std::sqrt(inclineRun * inclineRun + inclineRise * inclineRise);
    float inclineAngle = glm::degrees(std::atan2(inclineRise, inclineRun));

    float xBottomFlatCenter = l.baseX - landingLen * 0.5f;
    float xBottomToIncline = l.baseX - landingLen;
    float xTopFromIncline = l.topX + landingLen;
    float xTopFlatCenter = l.topX + landingLen * 0.5f;

    auto drawHandrailLoop = [&](float xCenter, float yCenter, float zPos, float radius, bool topLoop) {
        const int segs = 14;
        for (int i = 0; i < segs; ++i) {
            float t0 = (float)i / (float)segs;
            float t1 = (float)(i + 1) / (float)segs;

            float a0 = topLoop ? glm::pi<float>() * t0 : glm::pi<float>() * (1.0f + t0);
            float a1 = topLoop ? glm::pi<float>() * t1 : glm::pi<float>() * (1.0f + t1);

            glm::vec3 p0(xCenter + std::cos(a0) * radius, yCenter + std::sin(a0) * radius, zPos);
            glm::vec3 p1(xCenter + std::cos(a1) * radius, yCenter + std::sin(a1) * radius, zPos);

            glm::vec3 mid = (p0 + p1) * 0.5f;
            float segLen = glm::length(p1 - p0);
            float segAngle = glm::degrees(std::atan2(p1.y - p0.y, p1.x - p0.x));

            drawCubeRotated(shader, cubeVAO, mid,
                            glm::vec3(segLen, 0.055f, 0.06f),
                            glm::vec3(0.0f, 0.0f, segAngle),
                            glm::vec3(0.08f, 0.08f, 0.09f), 3,
                            0.05f, 0.35f, 0.95f, 180.0f);
        }
    };

    auto drawEscalatorLane = [&](float laneZ, bool upDirection) {
        glm::vec3 metalSide(0.58f, 0.62f, 0.69f);
        glm::vec3 combColor(0.22f, 0.24f, 0.26f);
        float stepTravelY0 = 0.16f;

        // Geometric step constants (used for perfect no-gap stair tiling).
        const float stepDepth = 0.62f;      // horizontal tread depth (local z axis of escalator profile)
        const float stepRise = 0.18f;       // vertical riser height
        const float treadThickness = 0.08f;
        const float riserThickness = 0.05f;
        const float stepWidth = l.laneWidth - 0.34f;

        // Metallic truss chassis (no solid wedge body)
        glm::vec3 inclineCenter((xBottomToIncline + xTopFromIncline) * 0.5f,
                                inclineRise * 0.5f,
                                laneZ);

        drawCubeRotated(shader, cubeVAO, inclineCenter + glm::vec3(0.0f, -0.10f, -l.laneWidth * 0.48f),
                        glm::vec3(inclineLen, 0.16f, 0.10f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle), metalSide, 3,
                        0.06f, 0.45f, 0.95f, 180.0f);
        drawCubeRotated(shader, cubeVAO, inclineCenter + glm::vec3(0.0f, -0.10f, l.laneWidth * 0.48f),
                        glm::vec3(inclineLen, 0.16f, 0.10f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle), metalSide, 3,
                        0.06f, 0.45f, 0.95f, 180.0f);

        for (int b = 0; b < 10; ++b) {
            float u = (float)b / 9.0f;
            float x = xBottomToIncline + u * (xTopFromIncline - xBottomToIncline);
            float y = u * inclineRise - 0.10f;
            drawCubeRotated(shader, cubeVAO,
                            glm::vec3(x, y, laneZ),
                            glm::vec3(0.10f, 0.12f, l.laneWidth * 0.94f),
                            glm::vec3(0.0f, 0.0f, -inclineAngle),
                            glm::vec3(0.44f, 0.47f, 0.52f), 3,
                            0.06f, 0.40f, 0.88f, 160.0f);
        }

        // Flat landing comb plates (bottom + top)
        drawCube(shader, cubeVAO,
                 glm::vec3(xBottomFlatCenter, stepTravelY0, laneZ),
                 glm::vec3(landingLen, 0.06f, l.laneWidth - 0.25f),
                 combColor, 3, 0.08f, 0.42f, 0.85f, 160.0f);
        drawCube(shader, cubeVAO,
                 glm::vec3(xTopFlatCenter, inclineRise + stepTravelY0, laneZ),
                 glm::vec3(landingLen, 0.06f, l.laneWidth - 0.25f),
                 combColor, 3, 0.08f, 0.42f, 0.85f, 160.0f);

        // Newel (rounded end where handrail loops)
        float newelRadius = 0.58f;
        drawHandrailLoop(xBottomToIncline, 0.72f, laneZ - l.laneWidth * 0.5f, newelRadius, false);
        drawHandrailLoop(xBottomToIncline, 0.72f, laneZ + l.laneWidth * 0.5f, newelRadius, false);
        drawHandrailLoop(xTopFromIncline, inclineRise + 0.72f, laneZ - l.laneWidth * 0.5f, newelRadius, true);
        drawHandrailLoop(xTopFromIncline, inclineRise + 0.72f, laneZ + l.laneWidth * 0.5f, newelRadius, true);

        // Handrails (top run)
        drawCube(shader, cubeVAO,
                 glm::vec3(xBottomFlatCenter, 1.30f, laneZ - l.laneWidth * 0.5f),
                 glm::vec3(landingLen, 0.055f, 0.06f),
                 glm::vec3(0.08f, 0.08f, 0.09f), 3, 0.05f, 0.35f, 0.95f, 180.0f);
        drawCube(shader, cubeVAO,
                 glm::vec3(xBottomFlatCenter, 1.30f, laneZ + l.laneWidth * 0.5f),
                 glm::vec3(landingLen, 0.055f, 0.06f),
                 glm::vec3(0.08f, 0.08f, 0.09f), 3, 0.05f, 0.35f, 0.95f, 180.0f);

        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 1.30f - inclineRise * 0.5f, -l.laneWidth * 0.5f),
                        glm::vec3(inclineLen, 0.055f, 0.06f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle),
                        glm::vec3(0.08f, 0.08f, 0.09f), 3, 0.05f, 0.35f, 0.95f, 180.0f);
        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 1.30f - inclineRise * 0.5f, l.laneWidth * 0.5f),
                        glm::vec3(inclineLen, 0.055f, 0.06f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle),
                        glm::vec3(0.08f, 0.08f, 0.09f), 3, 0.05f, 0.35f, 0.95f, 180.0f);

        drawCube(shader, cubeVAO,
                 glm::vec3(xTopFlatCenter, inclineRise + 1.30f, laneZ - l.laneWidth * 0.5f),
                 glm::vec3(landingLen, 0.055f, 0.06f),
                 glm::vec3(0.08f, 0.08f, 0.09f), 3, 0.05f, 0.35f, 0.95f, 180.0f);
        drawCube(shader, cubeVAO,
                 glm::vec3(xTopFlatCenter, inclineRise + 1.30f, laneZ + l.laneWidth * 0.5f),
                 glm::vec3(landingLen, 0.055f, 0.06f),
                 glm::vec3(0.08f, 0.08f, 0.09f), 3, 0.05f, 0.35f, 0.95f, 180.0f);

        // Side balustrade base panels (opaque lower skirt, practical hand-rest support)
        drawCube(shader, cubeVAO,
             glm::vec3(xBottomFlatCenter, 0.42f, laneZ - l.laneWidth * 0.5f),
             glm::vec3(landingLen, 0.48f, 0.08f),
             glm::vec3(0.50f, 0.54f, 0.60f), 3, 0.08f, 0.42f, 0.80f, 120.0f);
        drawCube(shader, cubeVAO,
             glm::vec3(xBottomFlatCenter, 0.42f, laneZ + l.laneWidth * 0.5f),
             glm::vec3(landingLen, 0.48f, 0.08f),
             glm::vec3(0.50f, 0.54f, 0.60f), 3, 0.08f, 0.42f, 0.80f, 120.0f);

        drawCubeRotated(shader, cubeVAO,
                inclineCenter + glm::vec3(0.0f, 0.42f - inclineRise * 0.5f, -l.laneWidth * 0.5f),
                glm::vec3(inclineLen, 0.48f, 0.08f),
                glm::vec3(0.0f, 0.0f, -inclineAngle),
                glm::vec3(0.50f, 0.54f, 0.60f), 3, 0.08f, 0.42f, 0.80f, 120.0f);
        drawCubeRotated(shader, cubeVAO,
                inclineCenter + glm::vec3(0.0f, 0.42f - inclineRise * 0.5f, l.laneWidth * 0.5f),
                glm::vec3(inclineLen, 0.48f, 0.08f),
                glm::vec3(0.0f, 0.0f, -inclineAngle),
                glm::vec3(0.50f, 0.54f, 0.60f), 3, 0.08f, 0.42f, 0.80f, 120.0f);

        drawCube(shader, cubeVAO,
             glm::vec3(xTopFlatCenter, inclineRise + 0.42f, laneZ - l.laneWidth * 0.5f),
             glm::vec3(landingLen, 0.48f, 0.08f),
             glm::vec3(0.50f, 0.54f, 0.60f), 3, 0.08f, 0.42f, 0.80f, 120.0f);
        drawCube(shader, cubeVAO,
             glm::vec3(xTopFlatCenter, inclineRise + 0.42f, laneZ + l.laneWidth * 0.5f),
             glm::vec3(landingLen, 0.48f, 0.08f),
             glm::vec3(0.50f, 0.54f, 0.60f), 3, 0.08f, 0.42f, 0.80f, 120.0f);

        // Glass balustrades (outside + inside lane boundary)
        glm::vec3 glassColor(0.76f, 0.86f, 0.95f);
        float glassH = 1.0f;

        drawCubeAlpha(shader, cubeVAO,
                      glm::vec3(xBottomFlatCenter, 0.78f, laneZ - l.laneWidth * 0.48f),
                      glm::vec3(landingLen, glassH, 0.03f), glassColor,
                      4, 0.05f, 0.14f, 1.0f, 220.0f, 0.22f);
        drawCubeAlpha(shader, cubeVAO,
                      glm::vec3(xBottomFlatCenter, 0.78f, laneZ + l.laneWidth * 0.48f),
                      glm::vec3(landingLen, glassH, 0.03f), glassColor,
                      4, 0.05f, 0.14f, 1.0f, 220.0f, 0.22f);

        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 0.78f - inclineRise * 0.5f, -l.laneWidth * 0.48f),
                        glm::vec3(inclineLen, glassH, 0.03f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle), glassColor,
                        4, 0.05f, 0.14f, 1.0f, 220.0f);
        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 0.78f - inclineRise * 0.5f, l.laneWidth * 0.48f),
                        glm::vec3(inclineLen, glassH, 0.03f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle), glassColor,
                        4, 0.05f, 0.14f, 1.0f, 220.0f);

        drawCubeAlpha(shader, cubeVAO,
                      glm::vec3(xTopFlatCenter, inclineRise + 0.78f, laneZ - l.laneWidth * 0.48f),
                      glm::vec3(landingLen, glassH, 0.03f), glassColor,
                      4, 0.05f, 0.14f, 1.0f, 220.0f, 0.22f);
        drawCubeAlpha(shader, cubeVAO,
                      glm::vec3(xTopFlatCenter, inclineRise + 0.78f, laneZ + l.laneWidth * 0.48f),
                      glm::vec3(landingLen, glassH, 0.03f), glassColor,
                      4, 0.05f, 0.14f, 1.0f, 220.0f, 0.22f);

        // Handrail capping strips directly above glass for clear human hand-rest surfaces.
        drawCube(shader, cubeVAO,
                 glm::vec3(xBottomFlatCenter, 1.28f, laneZ - l.laneWidth * 0.48f),
                 glm::vec3(landingLen, 0.075f, 0.09f),
                 glm::vec3(0.07f, 0.07f, 0.08f), 3, 0.05f, 0.34f, 0.98f, 210.0f);
        drawCube(shader, cubeVAO,
                 glm::vec3(xBottomFlatCenter, 1.28f, laneZ + l.laneWidth * 0.48f),
                 glm::vec3(landingLen, 0.075f, 0.09f),
                 glm::vec3(0.07f, 0.07f, 0.08f), 3, 0.05f, 0.34f, 0.98f, 210.0f);

        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 1.28f - inclineRise * 0.5f, -l.laneWidth * 0.48f),
                        glm::vec3(inclineLen, 0.075f, 0.09f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle),
                        glm::vec3(0.07f, 0.07f, 0.08f), 3, 0.05f, 0.34f, 0.98f, 210.0f);
        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 1.28f - inclineRise * 0.5f, l.laneWidth * 0.48f),
                        glm::vec3(inclineLen, 0.075f, 0.09f),
                        glm::vec3(0.0f, 0.0f, -inclineAngle),
                        glm::vec3(0.07f, 0.07f, 0.08f), 3, 0.05f, 0.34f, 0.98f, 210.0f);

        drawCube(shader, cubeVAO,
                 glm::vec3(xTopFlatCenter, inclineRise + 1.28f, laneZ - l.laneWidth * 0.48f),
                 glm::vec3(landingLen, 0.075f, 0.09f),
                 glm::vec3(0.07f, 0.07f, 0.08f), 3, 0.05f, 0.34f, 0.98f, 210.0f);
        drawCube(shader, cubeVAO,
                 glm::vec3(xTopFlatCenter, inclineRise + 1.28f, laneZ + l.laneWidth * 0.48f),
                 glm::vec3(landingLen, 0.075f, 0.09f),
                 glm::vec3(0.07f, 0.07f, 0.08f), 3, 0.05f, 0.34f, 0.98f, 210.0f);

        // Opaque side skirt to block any view to ground between moving steps.
        drawCubeRotated(shader, cubeVAO,
                        inclineCenter + glm::vec3(0.0f, 0.02f, 0.0f),
                        glm::vec3(inclineLen, 0.22f, stepWidth),
                        glm::vec3(0.0f, 0.0f, -inclineAngle),
                        glm::vec3(0.18f, 0.19f, 0.21f), 3,
                        0.06f, 0.40f, 0.35f, 42.0f);

        // Animated right-angled steps with mathematically exact tread/riser pitch.
        // Local escalator profile coordinates (z forward, y up):
        //   y_offset = i * pitch * sin(theta)
        //   z_offset = i * pitch * cos(theta)
        // with pitch = stepDepth / cos(theta) and tan(theta) = stepRise / stepDepth.
        // This simplifies exactly to: y_offset = i * stepRise, z_offset = i * stepDepth.
        const int stepCount = 44;
        const float stepRate = 2.05f; // steps/second
        float phase = std::fmod((float)glfwGetTime() * stepRate, 1.0f);
        float inclineAngleRad = glm::radians(inclineAngle);
        float stepPitch = stepDepth / glm::cos(inclineAngleRad);
        float visibleLen = landingLen + inclineRun + landingLen;
        float sideDir = upDirection ? 1.0f : -1.0f;

        for (int i = 0; i < stepCount; ++i) {
            float idx = (float)i + phase;
            float y_offset = idx * stepPitch * glm::sin(inclineAngleRad); // == idx * stepRise
            float z_offset = idx * stepPitch * glm::cos(inclineAngleRad); // == idx * stepDepth

            // Escalator forward is toward -X in world for up lane. Keep naming z_offset
            // because this is local profile forward distance.
            float d = std::fmod(z_offset, visibleLen);

            float x = xBottomFlatCenter;
            float y = stepTravelY0;

            if (d < landingLen) {
                float t = d / landingLen;
                x = upDirection ? (l.baseX - t * landingLen) : (l.topX + t * landingLen);
                y = stepTravelY0;
            } else if (d < landingLen + inclineRun) {
                float t = (d - landingLen) / inclineRun;
                if (upDirection) {
                    x = xBottomToIncline - t * inclineRun;
                    y = stepTravelY0 + t * inclineRise;
                } else {
                    x = xTopFromIncline + t * inclineRun;
                    y = stepTravelY0 + inclineRise - t * inclineRise;
                }
            } else if (d < visibleLen) {
                float t = (d - landingLen - inclineRun) / landingLen;
                x = upDirection ? (xTopFromIncline - t * landingLen) : (xBottomToIncline + t * landingLen);
                y = stepTravelY0 + inclineRise;
            } else {
                continue;
            }

            // Lane direction phase shift keeps both lanes moving opposite while sharing geometry.
            x += sideDir * 0.0f;

            // Tread (horizontal top surface)
            drawCube(shader, cubeVAO,
                     glm::vec3(x, y, laneZ),
                     glm::vec3(stepDepth, treadThickness, stepWidth),
                     glm::vec3(0.14f, 0.14f, 0.15f), 7,
                     0.08f, 0.52f, 0.35f, 48.0f);

            // Riser (vertical face at tread back edge, tightly matches next-step rise)
            drawCube(shader, cubeVAO,
                     glm::vec3(x + (upDirection ? (stepDepth * 0.5f - riserThickness * 0.5f)
                                                : -(stepDepth * 0.5f - riserThickness * 0.5f)),
                               y - (stepRise * 0.5f + treadThickness * 0.5f),
                               laneZ),
                     glm::vec3(riserThickness, stepRise + treadThickness, stepWidth),
                     glm::vec3(0.12f, 0.12f, 0.13f), 7,
                     0.08f, 0.45f, 0.25f, 36.0f);
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
            float postHalfDepth = 0.06f; // half of support depth (0.12)
            float backOffsetSign = (side == 0) ? -1.0f : 1.0f;
            float postZ = posterZs[side] + backOffsetSign * postHalfDepth;

            glm::mat4 panelModel = glm::mat4(1.0f);
            panelModel = glm::translate(panelModel, glm::vec3(x, 0.25f, posterZs[side]));
            panelModel = glm::rotate(panelModel,
                                     glm::radians(side == 0 ? 180.0f : 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
            panelModel = glm::scale(panelModel, glm::vec3(2.0f, 2.8f, 1.0f));
            shader.setMat4("model", panelModel);

            // Apply real advertisement texture to one billboard (aarong.jpeg)
            if (side == 0 && i == 0 && texBillboardAd != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texBillboardAd);
                shader.setInt("texture1", 0);
                shader.setInt("useTexture", 1);
                shader.setInt("textureType", 0);
                shader.setVec3("objectColor", glm::vec3(1.0f));
            } else {
                shader.setInt("useTexture", 0);
                shader.setInt("textureType", 3);
                shader.setVec3("objectColor", glm::vec3(0.92f, 0.92f, 0.94f));
            }

            glBindVertexArray(billboardVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            drawCube(shader, cubeVAO,
                     glm::vec3(x, 1.1f, postZ),
                     glm::vec3(0.12f, 2.2f, 0.12f),
                     glm::vec3(0.42f, 0.42f, 0.45f), 3, 0.1f, 0.5f, 0.35f, 32.0f);
        }
    }

    shader.setInt("useTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

} // namespace EscalatorSystem

#endif
