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
    int cylSegments = 16;
    unsigned int sphereVAO = 0;
    int sphereCount = 0;
    unsigned int texSofa = 0;
};

inline RenderContext& ctx() {
    static RenderContext context;
    return context;
}

inline void setRenderContext(Shader& shader,
                             unsigned int cubeVAO,
                             int cylSegments,
                             unsigned int sphereVAO,
                             int sphereCount,
                             unsigned int texSofa) {
    RenderContext& c = ctx();
    c.shader = &shader;
    c.cubeVAO = cubeVAO;
    c.cylSegments = cylSegments;
    c.sphereVAO = sphereVAO;
    c.sphereCount = sphereCount;
    c.texSofa = texSofa;
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
    
    // --- 3. VIP CINEMA CHAIRS (Flat Generic Layout) ---
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
