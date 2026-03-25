#ifndef SECOND_FLOOR_LAYOUT_H
#define SECOND_FLOOR_LAYOUT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>

#include "shader.h"

// External helper functions assumed to be defined elsewhere (e.g. main.cpp or a utils file)
extern void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
                     glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
extern void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation,
                            glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);

namespace SecondFloorDesign {

struct RenderContext {
    Shader* shader = nullptr;
    unsigned int cubeVAO = 0;
    unsigned int cylVAO = 0;
    unsigned int sphereVAO = 0;
    int sphereCount = 0;
    unsigned int texSofa = 0;
    unsigned int texBillboardClockwork = 0;
    unsigned int texBillboardInterstellar = 0;
};

inline RenderContext& ctx() {
    static RenderContext context;
    return context;
}

inline void setRenderContext(Shader& shader,
                             unsigned int cubeVAO,
                             unsigned int cylVAO,
                             unsigned int sphereVAO,
                             int sphereCount,
                             unsigned int texSofa,
                             unsigned int texBillboardClockwork,
                             unsigned int texBillboardInterstellar) {
    RenderContext& c = ctx();
    c.shader = &shader;
    c.cubeVAO = cubeVAO;
    c.cylVAO = cylVAO;
    c.sphereVAO = sphereVAO;
    c.sphereCount = sphereCount;
    c.texSofa = texSofa;
    c.texBillboardClockwork = texBillboardClockwork;
    c.texBillboardInterstellar = texBillboardInterstellar;
}

// ─────────────────────────────────────────────────────────────────────────────
// 3D PROCEDURAL MESHES
// ─────────────────────────────────────────────────────────────────────────────

inline unsigned int createCurvedScreenVAO(int& outVertexCount) {
    const int segmentsX = 64;
    const int segmentsY = 16;
    const float width = 10.66f; // Reduced from 54 to fit 2nd floor
    const float height = 6.0f;  // Reduced from 30 to 6
    const float arcRadius = 15.0f; 
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    float thetaTotal = width / arcRadius; 
    
    for (int y = 0; y <= segmentsY; ++y) {
        float v = (float)y / segmentsY;
        float posY = v * height;
        
        for (int x = 0; x <= segmentsX; ++x) {
            float u = (float)x / segmentsX;
            float angle = -thetaTotal * 0.5f + u * thetaTotal;
            
            float posX = std::sin(angle) * arcRadius;
            float posZ = arcRadius - std::cos(angle) * arcRadius;
            
            float nx = -std::sin(angle);
            float nz = -std::cos(angle);
            
            vertices.insert(vertices.end(), {
                posX, posY, posZ,
                nx, 0.0f, nz,
                u, v
            });
        }
    }
    
    for (int y = 0; y < segmentsY; ++y) {
        for (int x = 0; x < segmentsX; ++x) {
            unsigned int bl = y * (segmentsX + 1) + x;
            unsigned int br = bl + 1;
            unsigned int tl = (y + 1) * (segmentsX + 1) + x;
            unsigned int tr = tl + 1;
            
            indices.push_back(bl);
            indices.push_back(br);
            indices.push_back(tr);
            
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tl);
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

inline unsigned int createPleatedCurtainVAO(int& outVertexCount) {
    const int segmentsX = 100;
    const int segmentsY = 10;
    const float width = 3.5f;   // Scaled down to match new screen
    const float height = 6.2f;
    
    const float pleatFreq = 10.0f;
    const float pleatDepth = 0.15f; 
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int y = 0; y <= segmentsY; ++y) {
        float v = (float)y / segmentsY;
        float posY = v * height;
        
        for (int x = 0; x <= segmentsX; ++x) {
            float u = (float)x / segmentsX;
            float posX = u * width;
            
            float localDepth = pleatDepth * (0.8f + 0.2f * (1.0f - v));
            float posZ = std::sin(u * glm::two_pi<float>() * pleatFreq) * localDepth;
            
            float dzdx = std::cos(u * glm::two_pi<float>() * pleatFreq) * glm::two_pi<float>() * pleatFreq / width * localDepth;
            
            glm::vec3 tangent(1.0f, 0.0f, dzdx);
            glm::vec3 bitangent(0.0f, 1.0f, 0.0f);
            glm::vec3 normal = glm::normalize(glm::cross(bitangent, tangent));
            
            // Generate some rough UVs for curtain just in case (though it'll be solid velvet)
            vertices.insert(vertices.end(), {
                posX, posY, posZ,
                normal.x, normal.y, normal.z,
                u, v
            });
        }
    }
    
    for (int y = 0; y < segmentsY; ++y) {
        for (int x = 0; x < segmentsX; ++x) {
            unsigned int bl = y * (segmentsX + 1) + x;
            unsigned int br = bl + 1;
            unsigned int tl = (y + 1) * (segmentsX + 1) + x;
            unsigned int tr = tl + 1;
            
            indices.push_back(bl);
            indices.push_back(br);
            indices.push_back(tr);
            
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tl);
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

inline unsigned int createPopcornBoxVAO(int& outVertexCount) {
    float b = 0.4f; // bottom half-width
    float t = 0.55f; // top half-width
    float h = 1.2f; // total height
    
    // Front Face (Z = front)
    glm::vec3 nF(0.0f, h, t - b);
    nF = glm::normalize(nF);
    
    // Back Face (Z = back)
    glm::vec3 nB(0.0f, h, -(t - b));
    nB = glm::normalize(nB);
    
    // Right Face (X = right)
    glm::vec3 nR(h, -(t - b), 0.0f);
    nR = glm::normalize(nR);
    
    // Left Face (X = left)
    glm::vec3 nL(-h, -(t - b), 0.0f);
    nL = glm::normalize(nL);
    
    float verts[] = {
        // Front face
        -b, 0.0f,  b, nF.x, nF.y, nF.z, 0.0f, 0.0f,
         b, 0.0f,  b, nF.x, nF.y, nF.z, 1.0f, 0.0f,
         t,  h,   t, nF.x, nF.y, nF.z, 1.0f, 1.0f,
         t,  h,   t, nF.x, nF.y, nF.z, 1.0f, 1.0f,
        -t,  h,   t, nF.x, nF.y, nF.z, 0.0f, 1.0f,
        -b, 0.0f,  b, nF.x, nF.y, nF.z, 0.0f, 0.0f,
        // Back face
        -b, 0.0f, -b, nB.x, nB.y, nB.z, 1.0f, 0.0f,
        -t,  h,  -t, nB.x, nB.y, nB.z, 1.0f, 1.0f,
         t,  h,  -t, nB.x, nB.y, nB.z, 0.0f, 1.0f,
         t,  h,  -t, nB.x, nB.y, nB.z, 0.0f, 1.0f,
         b, 0.0f, -b, nB.x, nB.y, nB.z, 0.0f, 0.0f,
        -b, 0.0f, -b, nB.x, nB.y, nB.z, 1.0f, 0.0f,
        // Left face
        -b, 0.0f, -b, nL.x, nL.y, nL.z, 0.0f, 0.0f,
        -b, 0.0f,  b, nL.x, nL.y, nL.z, 1.0f, 0.0f,
        -t,  h,   t, nL.x, nL.y, nL.z, 1.0f, 1.0f,
        -t,  h,   t, nL.x, nL.y, nL.z, 1.0f, 1.0f,
        -t,  h,  -t, nL.x, nL.y, nL.z, 0.0f, 1.0f,
        -b, 0.0f, -b, nL.x, nL.y, nL.z, 0.0f, 0.0f,
        // Right face
         b, 0.0f, -b, nR.x, nR.y, nR.z, 1.0f, 0.0f,
         t,  h,  -t, nR.x, nR.y, nR.z, 1.0f, 1.0f,
         t,  h,   t, nR.x, nR.y, nR.z, 0.0f, 1.0f,
         t,  h,   t, nR.x, nR.y, nR.z, 0.0f, 1.0f,
         b, 0.0f,  b, nR.x, nR.y, nR.z, 0.0f, 0.0f,
         b, 0.0f, -b, nR.x, nR.y, nR.z, 1.0f, 0.0f,
        // Bottom face
        -b, 0.0f, -b, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         b, 0.0f, -b, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
         b, 0.0f,  b, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         b, 0.0f,  b, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -b, 0.0f,  b, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -b, 0.0f, -b, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
        // Top face (open box) - inner floor to drop popcorn inside
        -t, h - 0.2f, -t, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -t, h - 0.2f,  t, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
         t, h - 0.2f,  t, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         t, h - 0.2f,  t, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
         t, h - 0.2f, -t, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        -t, h - 0.2f, -t, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
    };
    
    outVertexCount = 36;
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return VAO;
}

inline void drawPopcornBoxHelper(glm::vec3 pos) {
    RenderContext& c = ctx();
    if (!c.shader || c.sphereVAO == 0) return;
    Shader& shader = *c.shader;
    
    static unsigned int pboxVAO = 0;
    static int pboxVerts = 0;
    if (pboxVAO == 0) pboxVAO = createPopcornBoxVAO(pboxVerts);
    
    glm::vec3 stripeColor(0.8f, 0.1f, 0.1f);
    
    // Draw rigid box
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, pos);
    m = glm::scale(m, glm::vec3(0.3f)); // scale down
    shader.setMat4("model", m);
    shader.setVec3("objectColor", stripeColor);
    shader.setInt("textureType", 0);
    shader.setFloat("ambientStrength", 0.3f);
    shader.setFloat("diffuseStrength", 0.6f);
    shader.setFloat("specularStrength", 0.1f);
    shader.setFloat("shininess", 2.0f);
    glBindVertexArray(pboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, pboxVerts);
    
    // Draw overlapping popcorn spheres on top rim
    glm::vec3 pcColor(0.95f, 0.85f, 0.3f);
    float boxH = 0.3f * 1.2f;
    for (int i = 0; i < 12; ++i) {
        float ox = ((rand() % 100) / 100.0f - 0.5f) * 0.18f;
        float oz = ((rand() % 100) / 100.0f - 0.5f) * 0.18f;
        float oy = ((rand() % 100) / 100.0f) * 0.05f;
        
        glm::mat4 sm = glm::mat4(1.0f);
        sm = glm::translate(sm, pos + glm::vec3(ox, boxH - 0.05f + oy, oz));
        sm = glm::scale(sm, glm::vec3(0.04f));
        shader.setMat4("model", sm);
        shader.setVec3("objectColor", pcColor);
        glBindVertexArray(c.sphereVAO);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// PROCEDURAL VIP RECLINER CHAIR
// ─────────────────────────────────────────────────────────────────────────────

inline void drawCinemaChair(glm::vec3 position, float rotationDeg) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;
    
    glm::vec3 leather(0.3f, 0.3f, 0.3f);   // Brighten base color since texture will tint it
    glm::vec3 metal(0.2f, 0.2f, 0.2f);
    glm::vec3 cupHolderGlow(0.0f, 0.5f, 0.8f);
    
    float amb = 0.20f;
    float diff = 0.50f;
    float spec = 0.15f; // reduced spec for diffuse fabric/leather
    float shin = 8.0f;
    
    // Draw Base (Metal, no texture)
    shader.setInt("useTexture", 0);
    drawCubeRotated(shader, c.cubeVAO, position + glm::vec3(0.0f, 0.15f, 0.0f), 
                    glm::vec3(0.6f, 0.3f, 0.6f), glm::vec3(0, rotationDeg, 0),
                    metal, 0, amb, 0.3f, 0.8f, 64.0f);
                    
    // Use sofa texture for plush elements
    if (c.texSofa != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, c.texSofa);
        shader.setInt("texture1", 0);
        shader.setInt("useTexture", 1);
    }
                    
    // Draw Plush Seat Cushion
    drawCubeRotated(shader, c.cubeVAO, position + glm::vec3(0.0f, 0.4f, 0.1f), 
                    glm::vec3(0.8f, 0.2f, 0.7f), glm::vec3(0, rotationDeg, 0),
                    leather, 0, amb, diff, spec, shin);
                    
    // Draw Angled High Backrest
    float rRad = glm::radians(rotationDeg);
    float backTilt = glm::radians(15.0f);
    glm::mat4 backModel = glm::mat4(1.0f);
    backModel = glm::translate(backModel, position + glm::vec3(-std::sin(rRad)*0.4f, 0.9f, -std::cos(rRad)*0.4f)); 
    backModel = glm::rotate(backModel, rRad, glm::vec3(0.0f, 1.0f, 0.0f));
    backModel = glm::rotate(backModel, -backTilt, glm::vec3(1.0f, 0.0f, 0.0f));
    backModel = glm::scale(backModel, glm::vec3(0.8f, 1.1f, 0.2f));
    
    shader.setMat4("model", backModel);
    shader.setVec3("objectColor", leather);
    shader.setFloat("ambientStrength", amb);
    shader.setFloat("diffuseStrength", diff);
    shader.setFloat("specularStrength", spec);
    shader.setFloat("shininess", shin);
    shader.setFloat("objectAlpha", 1.0f);
    glBindVertexArray(c.cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Draw Armrests
    auto rotY = [&](float x, float z) {
        return glm::vec3(std::cos(rRad)*x + std::sin(rRad)*z, 0.0f, -std::sin(rRad)*x + std::cos(rRad)*z);
    };
    
    glm::vec3 armrestOffset(0.5f, 0.65f, 0.1f);
    glm::vec3 armL = position + glm::vec3(0.0f, armrestOffset.y, 0.0f) + rotY(0.48f, 0.05f);
    glm::vec3 armR = position + glm::vec3(0.0f, armrestOffset.y, 0.0f) + rotY(-0.48f, 0.05f);
    
    drawCubeRotated(shader, c.cubeVAO, armL, glm::vec3(0.2f, 0.15f, 0.7f), glm::vec3(0, rotationDeg, 0), leather, 0, amb, diff, spec, shin);
    drawCubeRotated(shader, c.cubeVAO, armR, glm::vec3(0.2f, 0.15f, 0.7f), glm::vec3(0, rotationDeg, 0), leather, 0, amb, diff, spec, shin);
    
    // Armrest vertical supports
    drawCubeRotated(shader, c.cubeVAO, armL - glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(0.15f, 0.4f, 0.6f), glm::vec3(0, rotationDeg, 0), leather, 0, amb, diff, spec, shin);
    drawCubeRotated(shader, c.cubeVAO, armR - glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(0.15f, 0.4f, 0.6f), glm::vec3(0, rotationDeg, 0), leather, 0, amb, diff, spec, shin);

    // Revert texture mode
    shader.setInt("useTexture", 0);

    // Cup holders (Glowing rings on armrests)
    glm::vec3 cupL = armL + rotY(0.0f, 0.25f) + glm::vec3(0.0f, 0.08f, 0.0f);
    glm::vec3 cupR = armR + rotY(0.0f, 0.25f) + glm::vec3(0.0f, 0.08f, 0.0f);
    drawCubeRotated(shader, c.cubeVAO, cupL, glm::vec3(0.08f, 0.015f, 0.08f), glm::vec3(0, rotationDeg, 0), cupHolderGlow, 1, 0.9f, 0.2f, 0.0f, 1.0f);
    drawCubeRotated(shader, c.cubeVAO, cupR, glm::vec3(0.08f, 0.015f, 0.08f), glm::vec3(0, rotationDeg, 0), cupHolderGlow, 1, 0.9f, 0.2f, 0.0f, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// PROCEDURAL NPC SYSTEM (HIERARCHICAL)
// ─────────────────────────────────────────────────────────────────────────────

enum class NPCState { WANDERING, IN_LINE, TAKING_PICTURE, SELLING };

enum Gender { MALE, FEMALE };

struct NPC {
    glm::vec3 position;
    glm::vec3 targetPosition;
    float rotationY;
    float speed;
    float walkCycleTime;
    NPCState state;
    Gender gender;
    glm::vec3 clothingColor;
    float flashTimer;
    float waitTimer;
};

static std::vector<NPC> npcs;

inline void initNPCs(float floorY) {
    if (!npcs.empty()) return;

    const int wanderingCount = 28;
    const int lineCount = 6;
    const int photographerCount = 2;
    const int salesmanCount = 1;
    const int totalCrowd = wanderingCount + lineCount + photographerCount + salesmanCount;

    std::vector<Gender> genderPool;
    genderPool.reserve(totalCrowd);
    for (int i = 0; i < totalCrowd; ++i) {
        genderPool.push_back(i < totalCrowd / 2 ? MALE : FEMALE);
    }
    for (int i = totalCrowd - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        Gender tmp = genderPool[i];
        genderPool[i] = genderPool[j];
        genderPool[j] = tmp;
    }
    int genderIndex = 0;

    auto randomBrightColor = []() {
        float r = 0.25f + ((rand() % 100) / 100.0f) * 0.75f;
        float g = 0.25f + ((rand() % 100) / 100.0f) * 0.75f;
        float b = 0.25f + ((rand() % 100) / 100.0f) * 0.75f;
        float maxC = std::max(r, std::max(g, b));
        if (maxC < 0.65f) {
            float boost = 0.65f - maxC;
            r = std::min(1.0f, r + boost);
            g = std::min(1.0f, g + boost);
            b = std::min(1.0f, b + boost);
        }
        return glm::vec3(r, g, b);
    };

    std::vector<glm::vec2> usedPositions;
    usedPositions.reserve(totalCrowd);
    auto randomLobbyPosition2D = [&](float minDistance) {
        const float minX = 18.0f;
        const float maxX = 122.0f;
        const float minZ = 73.0f;
        const float maxZ = 94.0f;

        glm::vec2 candidate((minX + maxX) * 0.5f, (minZ + maxZ) * 0.5f);
        bool found = false;
        for (int attempt = 0; attempt < 60 && !found; ++attempt) {
            float x = minX + ((rand() % 10000) / 10000.0f) * (maxX - minX);
            float z = minZ + ((rand() % 10000) / 10000.0f) * (maxZ - minZ);
            candidate = glm::vec2(x, z);

            found = true;
            for (const glm::vec2& p : usedPositions) {
                if (glm::distance(p, candidate) < minDistance) {
                    found = false;
                    break;
                }
            }
        }
        usedPositions.push_back(candidate);
        return candidate;
    };
    
    // Main wandering crowd across lobby floor
    for (int i = 0; i < wanderingCount; ++i) {
        glm::vec2 pos2 = randomLobbyPosition2D(2.2f);
        glm::vec2 target2 = randomLobbyPosition2D(2.0f);
        NPC n;
        n.position = glm::vec3(pos2.x, floorY, pos2.y);
        n.targetPosition = glm::vec3(target2.x, floorY, target2.y);
        glm::vec3 facingDir = n.targetPosition - n.position;
        n.rotationY = glm::degrees(atan2(facingDir.x, facingDir.z));
        n.speed = 1.0f + ((rand() % 100) / 200.0f); // 1.0 to 1.5
        n.walkCycleTime = (rand() % 100) / 10.0f;
        n.state = NPCState::WANDERING;
        n.gender = genderPool[genderIndex++];
        n.clothingColor = randomBrightColor();
        n.flashTimer = 0.0f;
        n.waitTimer = 0.0f;
        npcs.push_back(n);
    }
    
    // Queue near concession with slight random spread
    for (int i = 0; i < lineCount; ++i) {
        glm::vec2 queuePos = randomLobbyPosition2D(1.4f);
        queuePos.x = 26.0f + ((rand() % 1000) / 1000.0f) * 8.0f;
        queuePos.y = 83.0f + ((rand() % 1000) / 1000.0f) * 10.0f;
        NPC n;
        n.position = glm::vec3(queuePos.x, floorY, queuePos.y);
        n.targetPosition = n.position;
        n.rotationY = 180.0f; // Face -Z (towards counter)
        n.speed = 0.0f;
        n.walkCycleTime = 0.0f;
        n.state = NPCState::IN_LINE;
        n.gender = genderPool[genderIndex++];
        n.clothingColor = randomBrightColor();
        n.flashTimer = 0.0f;
        n.waitTimer = 0.0f;
        npcs.push_back(n);
    }
    
    // Photographers near billboards
    for (int i = 0; i < photographerCount; ++i) {
        NPC n;
        n.position = glm::vec3(45.0f + i * 50.0f + ((rand() % 1000) / 1000.0f - 0.5f) * 3.0f,
                               floorY,
                               78.0f + ((rand() % 1000) / 1000.0f - 0.5f) * 2.5f);
        n.targetPosition = n.position;
        n.rotationY = 180.0f; // Look back at billboard
        n.speed = 0.0f;
        n.walkCycleTime = 0.0f;
        n.state = NPCState::TAKING_PICTURE;
        n.gender = genderPool[genderIndex++];
        n.clothingColor = randomBrightColor();
        n.flashTimer = (rand() % 500) / 100.0f;
        n.waitTimer = 0.0f;
        npcs.push_back(n);
    }
    
    // Concession sales staff
    for (int i = 0; i < salesmanCount; ++i) {
        NPC salesman;
        salesman.position = glm::vec3(30.0f + ((rand() % 1000) / 1000.0f - 0.5f) * 1.5f, floorY, 78.5f);
        salesman.targetPosition = salesman.position;
        salesman.rotationY = 0.0f; // Face +Z (towards customers)
        salesman.speed = 0.0f;
        salesman.walkCycleTime = 0.0f;
        salesman.state = NPCState::SELLING;
        salesman.gender = genderPool[genderIndex++];
        salesman.clothingColor = randomBrightColor();
        salesman.flashTimer = 0.0f;
        salesman.waitTimer = 0.0f;
        npcs.push_back(salesman);
    }
}

inline void updateNPCs(float deltaTime, float floorY) {
    if (npcs.empty()) {
        initNPCs(floorY);
    }
    
    for (auto& n : npcs) {
        if (n.state == NPCState::WANDERING) {
            if (n.waitTimer > 0.0f) {
                n.waitTimer -= deltaTime;
                n.walkCycleTime = 0.0f;
            } else {
                glm::vec3 dir = n.targetPosition - n.position;
                dir.y = 0.0f;
                float dist = glm::length(dir);
                if (dist < 0.2f) {
                    n.waitTimer = 1.0f + (rand() % 300) / 100.0f;
                    n.targetPosition = glm::vec3(20.0f + (rand() % 100), floorY, 73.0f + (rand() % 20));
                } else {
                    dir = glm::normalize(dir);
                    n.position += dir * n.speed * deltaTime;
                    n.rotationY = glm::degrees(atan2(dir.x, dir.z));
                    n.walkCycleTime += deltaTime * n.speed * 4.0f;
                }
            }
        } else if (n.state == NPCState::TAKING_PICTURE) {
            n.flashTimer -= deltaTime;
            if (n.flashTimer <= 0.0f) {
                n.flashTimer = 3.0f + (rand() % 400) / 100.0f; // Flash every 3-7 seconds
            }
        } else if (n.state == NPCState::SELLING) {
            // Keep the salesman's sine-wave running to animate scooping arms
            n.walkCycleTime += deltaTime * 5.0f; 
        }
    }
}

inline void drawProceduralNPC(NPC& npc, float walkCycleTime) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0 || c.cylVAO == 0 || c.sphereVAO == 0) return;
    Shader& shader = *c.shader;

    glm::vec3 position = npc.position;
    float rotationY = npc.rotationY;
    float cycle = walkCycleTime;
    float flashPhase = npc.flashTimer;
    NPCState state = npc.state;
    bool isFemale = (npc.gender == FEMALE);
    
    glm::vec3 skinColor(0.85f, 0.65f, 0.5f);
    glm::vec3 shirtColor = npc.clothingColor;
    glm::vec3 pantsColor(0.1f, 0.1f, 0.15f);
    glm::vec3 shoeColor(0.05f, 0.05f, 0.05f);
    
    // Adjust colors for salesman
    if (state == NPCState::SELLING) {
        shirtColor = glm::mix(npc.clothingColor, glm::vec3(0.85f, 0.15f, 0.15f), 0.5f);
        pantsColor = glm::vec3(0.9f, 0.9f, 0.9f);   // White pants
    }
    
    // Root transformation (Position and Yaw)
    glm::mat4 root = glm::mat4(1.0f);
    root = glm::translate(root, position);
    root = glm::rotate(root, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Mathematical swing logic
    float swing = std::sin(cycle); 
    float swingCos = std::cos(cycle); // For bouncy walk vertically
    
    float hipL = swing * 30.0f;
    float hipR = -swing * 30.0f;
    float kneeL = std::max(0.0f, -swing) * 40.0f;
    float kneeR = std::max(0.0f, swing) * 40.0f;
    
    float shoulderL = -swing * 25.0f;
    float shoulderR = swing * 25.0f;
    float elbowL = std::max(0.0f, -swing) * 20.0f + 5.0f;
    float elbowR = std::max(0.0f, swing) * 20.0f + 5.0f;
    
    float pelvicBounce = (state == NPCState::WANDERING) ? std::abs(swingCos) * 0.05f : 0.0f;

    // Special animation overrides for the Salesman (Scooping)
    if (state == NPCState::SELLING) {
        hipL = 0; hipR = 0; kneeL = 0; kneeR = 0; pelvicBounce = 0;
        shoulderL = 10.0f + std::sin(cycle * 0.5f) * 15.0f; // Continuous sweeping
        shoulderR = 25.0f + std::cos(cycle) * 20.0f;        // Sharp scooping
        elbowL = 40.0f + std::sin(cycle * 0.5f) * 5.0f;
        elbowR = 30.0f + std::cos(cycle) * 10.0f;
    }

    // --- 1. PELVIS (Anchor point for legs and spine) ---
    float pelvisY = 0.9f + pelvicBounce;
    glm::mat4 pelvisM = glm::translate(root, glm::vec3(0.0f, pelvisY, 0.0f));
    
    // --- 2. LOWER LIMBS (Thigh -> Calf -> Shoe) ---
    auto drawLeg = [&](int side, float hipFlex, float kneeFlex) {
        float dirMultiplier = (side == 0) ? -1.0f : 1.0f; // Left=0, Right=1
        
        // Thigh
        glm::mat4 thighM = glm::translate(pelvisM, glm::vec3(0.12f * dirMultiplier, -0.05f, 0.0f)); 
        thighM = glm::rotate(thighM, glm::radians(hipFlex), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 drawThigh = glm::translate(thighM, glm::vec3(0.0f, -0.2f, 0.0f)); // Draw relative to pivot
        drawThigh = glm::scale(drawThigh, glm::vec3(0.16f, 0.4f, 0.16f));
        shader.setMat4("model", drawThigh);
        shader.setVec3("objectColor", pantsColor);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, 16 * 12);
        
        // Calf
        glm::mat4 calfM = glm::translate(thighM, glm::vec3(0.0f, -0.4f, 0.0f));
        calfM = glm::rotate(calfM, glm::radians(kneeFlex), glm::vec3(1.0f, 0.0f, 0.0f)); // Knee rotates backwards (+X)
        glm::mat4 drawCalf = glm::translate(calfM, glm::vec3(0.0f, -0.2f, 0.0f));
        drawCalf = glm::scale(drawCalf, glm::vec3(0.12f, 0.4f, 0.12f));
        shader.setMat4("model", drawCalf);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, 16 * 12);
        
        // Shoe
        glm::mat4 shoeM = glm::translate(calfM, glm::vec3(0.0f, -0.42f, 0.06f));
        shoeM = glm::scale(shoeM, glm::vec3(0.14f, 0.08f, 0.25f));
        shader.setMat4("model", shoeM);
        shader.setVec3("objectColor", shoeColor);
        glBindVertexArray(c.cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    };
    if (!isFemale) {
        drawLeg(0, hipL, kneeL); // Left leg
        drawLeg(1, hipR, kneeR); // Right leg
    } else {
        drawLeg(0, hipL * 0.8f, kneeL * 0.6f);
        drawLeg(1, hipR * 0.8f, kneeR * 0.6f);
    }
    
    // --- 3. UPPER TORSO (Spine -> Chest) ---
    glm::mat4 torsoM = glm::translate(pelvisM, glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 drawTorso = glm::translate(torsoM, glm::vec3(0.0f, 0.35f, 0.0f));
    drawTorso = glm::scale(drawTorso, isFemale ? glm::vec3(0.30f, 0.65f, 0.19f) : glm::vec3(0.35f, 0.7f, 0.2f));
    shader.setMat4("model", drawTorso);
    shader.setVec3("objectColor", shirtColor);
    glBindVertexArray(c.cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    if (isFemale) {
        // Flared lower dress built from stacked cylinders for a tapered silhouette
        for (int seg = 0; seg < 3; ++seg) {
            float t = seg / 2.0f;
            float segY = 0.20f - t * 0.28f;
            float radius = 0.20f + t * 0.12f;
            glm::mat4 dressM = glm::translate(torsoM, glm::vec3(0.0f, segY, 0.0f));
            dressM = glm::scale(dressM, glm::vec3(radius, 0.24f, radius));
            shader.setMat4("model", dressM);
            shader.setVec3("objectColor", glm::mix(shirtColor, glm::vec3(0.95f), 0.08f));
            glBindVertexArray(c.cylVAO);
            glDrawArrays(GL_TRIANGLES, 0, 16 * 12);
        }
    }
    
    // --- 4. UPPER LIMBS (Shoulder -> Upper Arm -> Lower Arm -> Hand) ---
    auto drawArm = [&](int side, float shoulderFlex, float elbowFlex) {
        float dirMultiplier = (side == 0) ? -1.0f : 1.0f; 
        float shoulderWidth = isFemale ? 0.20f : 0.24f;
        
        // Upper Arm
        glm::mat4 uArmM = glm::translate(torsoM, glm::vec3(shoulderWidth * dirMultiplier, 0.58f, 0.0f)); // Shoulder pivot
        uArmM = glm::rotate(uArmM, glm::radians(shoulderFlex), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 drawUArm = glm::translate(uArmM, glm::vec3(0.0f, -0.15f, 0.0f)); 
        drawUArm = glm::scale(drawUArm, isFemale ? glm::vec3(0.10f, 0.28f, 0.10f) : glm::vec3(0.12f, 0.3f, 0.12f));
        shader.setMat4("model", drawUArm);
        shader.setVec3("objectColor", shirtColor); // Sleeve
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, 16 * 12);
        
        // Lower Arm
        glm::mat4 lArmM = glm::translate(uArmM, glm::vec3(0.0f, -0.3f, 0.0f)); // Elbow pivot
        lArmM = glm::rotate(lArmM, glm::radians(-elbowFlex), glm::vec3(1.0f, 0.0f, 0.0f)); // Elbow rotates forward (-X)
        glm::mat4 drawLArm = glm::translate(lArmM, glm::vec3(0.0f, -0.15f, 0.0f));
        drawLArm = glm::scale(drawLArm, glm::vec3(0.1f, 0.3f, 0.1f));
        shader.setMat4("model", drawLArm);
        shader.setVec3("objectColor", skinColor);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, 16 * 12);
        
        // Hand
        glm::mat4 handM = glm::translate(lArmM, glm::vec3(0.0f, -0.35f, 0.0f));
        handM = glm::scale(handM, glm::vec3(0.08f, 0.12f, 0.12f));
        shader.setMat4("model", handM);
        shader.setVec3("objectColor", skinColor);
        glBindVertexArray(c.sphereVAO);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
    };
    drawArm(0, shoulderL, elbowL); // Left arm
    drawArm(1, shoulderR, elbowR); // Right arm
    
    // --- 5. HEAD and NECK ---
    // Neck
    glm::mat4 neckM = glm::translate(torsoM, glm::vec3(0.0f, 0.75f, 0.0f));
    glm::mat4 drawNeck = glm::scale(neckM, glm::vec3(0.1f, 0.1f, 0.1f));
    shader.setMat4("model", drawNeck);
    shader.setVec3("objectColor", skinColor);
    glBindVertexArray(c.cylVAO);
    glDrawArrays(GL_TRIANGLES, 0, 16 * 12);
    
    // Head using cubeVAO (so the Front Face gets the UV properly)
    glm::mat4 headM = glm::translate(neckM, glm::vec3(0.0f, 0.15f, 0.0f));
    headM = glm::scale(headM, glm::vec3(0.22f, 0.25f, 0.22f));
    shader.setMat4("model", headM);
    // Explicitly enforce useTexture = 1 if user bound a literal face texture
    // For now we allow procedural skin if no texture is bound
    shader.setVec3("objectColor", skinColor);
    glBindVertexArray(c.cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    if (isFemale) {
        // Long hair volume at back of head (elongated sphere/ellipsoid)
        glm::mat4 hairM = glm::translate(neckM, glm::vec3(0.0f, 0.12f, -0.15f));
        hairM = glm::scale(hairM, glm::vec3(0.24f, 0.36f, 0.18f));
        shader.setMat4("model", hairM);
        shader.setVec3("objectColor", glm::vec3(0.16f, 0.10f, 0.05f));
        glBindVertexArray(c.sphereVAO);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
    }
    
    // --- 6. CAMERA FLASH (Photographers only) ---
    if (state == NPCState::TAKING_PICTURE && flashPhase > 2.85f) { 
        shader.setVec3("pointLights[31].position", position + glm::vec3(0.0f, 1.5f, 0.0f));
        shader.setVec3("pointLights[31].ambient", glm::vec3(0.5f));
        shader.setVec3("pointLights[31].diffuse", glm::vec3(5.0f, 5.0f, 6.0f)); // Bright blue-white flash
        shader.setVec3("pointLights[31].specular", glm::vec3(4.0f));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SECOND FLOOR MAIN LAYOUT (MOVIE THEATER)
// ─────────────────────────────────────────────────────────────────────────────

inline void drawSecondFloorLayout(float floorY, float ceilingY) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;
    
    const float roomWidth = 140.0f; // LOT_WIDTH
    const float roomDepth = 100.0f; // LOT_DEPTH
    const float roomHeight = ceilingY - floorY;
    
    // --- 1. ACOUSTIC WALLS ---
    glm::vec3 panelColor(0.2f, 0.05f, 0.05f); // Deep burgundy acoustic panels
    float padding = 0.1f;
    int numPanels = 24;
    float pSpacing = roomDepth / numPanels;
    
    for(int s = 0; s < 2; ++s) {
        float wx = (s == 0) ? padding : roomWidth - padding;
        for (int i = 1; i < numPanels; ++i) {
            float wz = i * pSpacing;
            drawCube(shader, c.cubeVAO, glm::vec3(wx, floorY + roomHeight / 2.0f, wz),
                     glm::vec3(0.4f, roomHeight * 0.8f, pSpacing * 0.8f), panelColor, 0, 0.1f, 0.4f, 0.05f, 4.0f);
        }
    }
    
    // --- 2. THE BIG SCREEN & 3D PLEATED CURTAINS ---
    static unsigned int screenVAO = 0;
    static int screenVerts = 0;
    static unsigned int curtainVAO = 0;
    static int curtainVerts = 0;
    
    if (screenVAO == 0) screenVAO = createCurvedScreenVAO(screenVerts);
    if (curtainVAO == 0) curtainVAO = createPleatedCurtainVAO(curtainVerts);
    
    // Draw Screen
    glm::mat4 screenMat = glm::mat4(1.0f);
    screenMat = glm::translate(screenMat, glm::vec3(roomWidth / 2.0f, floorY + 1.0f, 10.0f));
    screenMat = glm::rotate(screenMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    shader.setMat4("model", screenMat);
    shader.setVec3("objectColor", glm::vec3(0.9f, 0.95f, 1.0f)); 
    shader.setInt("textureType", 0);
    shader.setFloat("ambientStrength", 3.0f); // Massive glow
    shader.setFloat("diffuseStrength", 0.1f);
    shader.setFloat("specularStrength", 0.0f);
    shader.setFloat("shininess", 1.0f);
    shader.setFloat("objectAlpha", 1.0f);
    glBindVertexArray(screenVAO);
    glDrawElements(GL_TRIANGLES, screenVerts, GL_UNSIGNED_INT, 0);
    
    // Draw Curtains (Left and Right)
    glm::vec3 velvetRed(0.45f, 0.02f, 0.05f);
    float curtAmb = 0.05f, curtDiff = 0.35f, curtSpec = 0.05f, curtShin = 4.0f;
    
    // Left Curtain
    glm::mat4 curtLeft = glm::mat4(1.0f);
    curtLeft = glm::translate(curtLeft, glm::vec3(roomWidth / 2.0f - 5.3f, floorY + 0.9f, 9.5f)); // 5.3 = half of 10.6
    shader.setMat4("model", curtLeft);
    shader.setVec3("objectColor", velvetRed);
    shader.setFloat("ambientStrength", curtAmb);
    shader.setFloat("diffuseStrength", curtDiff);
    shader.setFloat("specularStrength", curtSpec);
    shader.setFloat("shininess", curtShin);
    glBindVertexArray(curtainVAO);
    glDrawElements(GL_TRIANGLES, curtainVerts, GL_UNSIGNED_INT, 0);

    // Right Curtain
    glm::mat4 curtRight = glm::mat4(1.0f);
    curtRight = glm::translate(curtRight, glm::vec3(roomWidth / 2.0f + 5.3f, floorY + 0.9f, 9.5f));
    curtRight = glm::scale(curtRight, glm::vec3(-1.0f, 1.0f, 1.0f));
    shader.setMat4("model", curtRight);
    glDrawElements(GL_TRIANGLES, curtainVerts, GL_UNSIGNED_INT, 0);
    
    // Top Valance
    glm::mat4 curtTop = glm::mat4(1.0f);
    curtTop = glm::translate(curtTop, glm::vec3(roomWidth / 2.0f - 5.4f, floorY + 7.0f, 9.6f));
    curtTop = glm::scale(curtTop, glm::vec3(10.8f / 3.5f, 0.15f, 1.0f));
    shader.setMat4("model", curtTop);
    glDrawElements(GL_TRIANGLES, curtainVerts, GL_UNSIGNED_INT, 0);
    
    // --- 3. LOBBY DIVIDER WALL (Z = 70.0f) ---
    glm::vec3 wallColor(0.2f, 0.18f, 0.15f);
    float wallZ = 70.0f;
    float archWidth = 14.0f;
    
    // Left Wall
    float leftW = (roomWidth / 2.0f) - (archWidth / 2.0f);
    drawCube(shader, c.cubeVAO, glm::vec3(leftW / 2.0f, floorY + roomHeight / 2.0f, wallZ),
             glm::vec3(leftW, roomHeight, 0.5f), wallColor, 0, 0.2f, 0.5f, 0.1f, 4.0f);
             
    // Right Wall
    float rightW = leftW;
    drawCube(shader, c.cubeVAO, glm::vec3(roomWidth - rightW / 2.0f, floorY + roomHeight / 2.0f, wallZ),
             glm::vec3(rightW, roomHeight, 0.5f), wallColor, 0, 0.2f, 0.5f, 0.1f, 4.0f);
             
    // Top Arch lintel
    float archH = roomHeight - 5.0f;
    drawCube(shader, c.cubeVAO, glm::vec3(roomWidth / 2.0f, floorY + 5.0f + archH / 2.0f, wallZ),
             glm::vec3(archWidth, archH, 0.5f), wallColor, 0, 0.2f, 0.5f, 0.1f, 4.0f);
             
    // --- 4. CONCESSION STAND (Lobby at Z = 80.0f, X = 30.0f) ---
    glm::vec3 counterColor(0.4f, 0.1f, 0.1f);  // Glossy red counter
    glm::vec3 topColor(0.9f, 0.9f, 0.9f);      // Marble top
    glm::vec3 popcornGlass(0.7f, 0.8f, 0.9f);  // Glass box
    glm::vec3 popcorn(0.9f, 0.8f, 0.2f);       // Yellow kernels
    
    float counterX = 30.0f;
    float counterZ = 80.0f;
    
    // Base counter body
    drawCube(shader, c.cubeVAO, glm::vec3(counterX, floorY + 0.6f, counterZ),
             glm::vec3(8.0f, 1.2f, 2.5f), counterColor, 0, 0.2f, 0.6f, 0.4f, 16.0f);
    // Marble top
    drawCube(shader, c.cubeVAO, glm::vec3(counterX, floorY + 1.25f, counterZ),
             glm::vec3(8.4f, 0.1f, 2.8f), topColor, 0, 0.2f, 0.6f, 0.8f, 64.0f);
             
    // VINTAGE POPCORN MACHINE
    float pmX = counterX - 2.0f;
    float pmY = floorY + 1.3f;
    
    // Red metallic stand base
    drawCube(shader, c.cubeVAO, glm::vec3(pmX, pmY + 0.2f, counterZ),
             glm::vec3(1.4f, 0.4f, 1.4f), glm::vec3(0.6f, 0.1f, 0.1f), 0, 0.3f, 0.6f, 0.8f, 32.0f);
             
    // Thick glass walls (Alpha = 0.3)
    shader.setFloat("objectAlpha", 0.3f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawCube(shader, c.cubeVAO, glm::vec3(pmX, pmY + 1.1f, counterZ),
             glm::vec3(1.3f, 1.4f, 1.3f), popcornGlass, 0, 0.8f, 0.2f, 0.9f, 128.0f);
    glDisable(GL_BLEND);
    shader.setFloat("objectAlpha", 1.0f); // Restore
    
    // Internal Heat Lamp Glow (Emissive)
    drawCube(shader, c.cubeVAO, glm::vec3(pmX, pmY + 1.7f, counterZ),
             glm::vec3(0.2f, 0.1f, 0.2f), glm::vec3(2.0f, 1.0f, 0.1f), 0, 3.0f, 0.0f, 0.0f, 1.0f);
             
    // Pile of popcorn kernels inside
    for(int k=0; k<25; ++k) {
        float kox = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;
        float koz = ((rand() % 100) / 100.0f - 0.5f) * 1.0f;
        float koy = pmY + 0.45f + ((rand() % 100) / 100.0f) * 0.4f;
        drawCube(shader, c.cubeVAO, glm::vec3(pmX + kox, koy, counterZ + koz),
                 glm::vec3(0.12f), popcorn, 0, 0.3f, 0.8f, 0.1f, 2.0f);
    }
    
    // Striped Awning/Canopy on top
    for (int s = 0; s < 7; ++s) {
        float stripW = 1.5f / 7.0f;
        float sx = (pmX - 0.75f) + stripW * s + (stripW / 2.0f);
        glm::vec3 col = (s % 2 == 0) ? glm::vec3(0.8f, 0.1f, 0.1f) : glm::vec3(0.9f, 0.9f, 0.9f);
        
        glm::mat4 canMat = glm::mat4(1.0f);
        canMat = glm::translate(canMat, glm::vec3(sx, pmY + 1.9f, counterZ));
        canMat = glm::rotate(canMat, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Slanted
        canMat = glm::scale(canMat, glm::vec3(stripW, 0.1f, 1.6f));
        shader.setMat4("model", canMat);
        shader.setVec3("objectColor", col);
        glBindVertexArray(c.cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Display Procedural Popcorn Boxes on the counter
    drawPopcornBoxHelper(glm::vec3(counterX + 1.0f, pmY + 0.18f, counterZ - 0.5f));
    drawPopcornBoxHelper(glm::vec3(counterX + 1.5f, pmY + 0.18f, counterZ - 0.3f));
    drawPopcornBoxHelper(glm::vec3(counterX + 2.0f, pmY + 0.18f, counterZ - 0.6f));
    
    // Cash Register
    drawCube(shader, c.cubeVAO, glm::vec3(counterX + 3.0f, pmY + 0.2f, counterZ + 0.2f),
             glm::vec3(0.6f, 0.4f, 0.8f), glm::vec3(0.2f), 0, 0.2f, 0.5f, 0.0f, 4.0f);
             
             
    // --- 5. FREE-STANDING MOVIE BILLBOARDS (Lobby) ---
    // Place them midway between the concession stand and the divider wall
    // Z = 75.0f, facing +Z (so people entering from escalator see them)
    glm::vec3 pColor(0.8f, 0.8f, 0.8f);
    
    auto drawBillboard = [&](float bx, float bz, unsigned int texPoster) {
        // Stand base
        drawCube(shader, c.cubeVAO, glm::vec3(bx, floorY + 0.1f, bz),
                 glm::vec3(3.0f, 0.2f, 1.5f), glm::vec3(0.1f), 0, 0.2f, 0.5f, 0.3f, 16.0f);
        // Stand legs
        drawCube(shader, c.cubeVAO, glm::vec3(bx - 1.0f, floorY + 1.5f, bz),
                 glm::vec3(0.1f, 3.0f, 0.1f), glm::vec3(0.1f), 0, 0.2f, 0.4f, 0.2f, 16.0f);
        drawCube(shader, c.cubeVAO, glm::vec3(bx + 1.0f, floorY + 1.5f, bz),
                 glm::vec3(0.1f, 3.0f, 0.1f), glm::vec3(0.1f), 0, 0.2f, 0.4f, 0.2f, 16.0f);
                 
        // Poster Plane
        glm::mat4 bm = glm::mat4(1.0f);
        bm = glm::translate(bm, glm::vec3(bx, floorY + 3.0f, bz + 0.1f));
        bm = glm::rotate(bm, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Face +Z
        bm = glm::scale(bm, glm::vec3(2.5f, 4.0f, 0.1f)); // Portrait poster
        
        if (texPoster != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texPoster);
            shader.setInt("texture1", 0);
            shader.setInt("useTexture", 1);
            shader.setInt("textureType", 0);
        } else {
            shader.setInt("useTexture", 0);
            shader.setInt("textureType", 0);
        }
        
        shader.setMat4("model", bm);
        shader.setVec3("objectColor", pColor);
        shader.setFloat("ambientStrength", 1.5f); // Brightly lit poster
        shader.setFloat("diffuseStrength", 0.1f);
        shader.setFloat("specularStrength", 0.0f);
        shader.setFloat("shininess", 2.0f);
        
        glBindVertexArray(c.cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        shader.setInt("useTexture", 0);
    };
    
    drawBillboard(45.0f, 75.0f, c.texBillboardClockwork);
    drawBillboard(95.0f, 75.0f, c.texBillboardInterstellar);
    
    // --- 6. RENDER NPCs ---
    // Ensure we reset any stray point light (camera flash) before drawing NPCs
    shader.setVec3("pointLights[31].ambient", glm::vec3(0.0f));
    shader.setVec3("pointLights[31].diffuse", glm::vec3(0.0f));
    shader.setVec3("pointLights[31].specular", glm::vec3(0.0f));
    
    for (auto& n : npcs) {
        drawProceduralNPC(n, n.walkCycleTime);
    }
    
    // --- 7. VIP CINEMA CHAIRS (Auditorium Z=0 to 70) ---
    // Place chairs perfectly on the flat generic floor
    int rows = 12;
    int chairsPerRow = 16;
    float chairSpacingX = 2.8f;
    float rowSpacingZ = 3.5f;
    float startZ = 20.0f;
    float corridorWidth = 6.0f;
    
    for (int r = 0; r < rows; ++r) {
        float zPos = startZ + r * rowSpacingZ;
        float yPos = floorY + 0.2f;
        
        float startX = roomWidth / 2.0f - (chairsPerRow / 2.0f) * chairSpacingX;
        
        for (int c = 0; c < chairsPerRow; ++c) {
            float xPos = startX + c * chairSpacingX;
            // Skip middle ones for the corridor
            if (xPos > roomWidth / 2.0f - corridorWidth / 2.0f && xPos < roomWidth / 2.0f + corridorWidth / 2.0f) {
                continue;
            }
            
            float xOffsetFromCenter = xPos - (roomWidth / 2.0f);
            float focusYaw = 180.0f - (xOffsetFromCenter * 0.3f);
            
            drawCinemaChair(glm::vec3(xPos, yPos, zPos), focusYaw);
        }
    }
}

} // namespace SecondFloorDesign

#endif
