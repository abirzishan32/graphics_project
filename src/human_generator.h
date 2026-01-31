#ifndef HUMAN_GENERATOR_H
#define HUMAN_GENERATOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "shader.h"

// ============================================================
// REALISTIC HUMAN GENERATOR
// Detail level: High (Fingers, Face Features, Hair, Clothing)
// ============================================================

class ProceduralHuman {
public:
    unsigned int skinVAO, skinVBO;
    unsigned int shirtVAO, shirtVBO;
    unsigned int pantVAO, pantVBO;
    unsigned int hairVAO, hairVBO;
    unsigned int eyeVAO, eyeVBO;
    
    int skinCount = 0;
    int shirtCount = 0;
    int pantCount = 0;
    int hairCount = 0;
    int eyeCount = 0;
    
    // Skin color (Realistic tone)
    glm::vec3 skinColor = glm::vec3(0.85f, 0.65f, 0.55f);
    glm::vec3 hairColor = glm::vec3(0.1f, 0.08f, 0.05f); // Dark brown
    glm::vec3 shirtColor = glm::vec3(0.9f, 0.1f, 0.1f); // Red shirt
    glm::vec3 pantColor = glm::vec3(0.1f, 0.1f, 0.3f);  // Dark blue pants
    glm::vec3 eyeWhiteColor = glm::vec3(0.95f, 0.95f, 0.95f);
    glm::vec3 pupilColor = glm::vec3(0.1f, 0.1f, 0.1f);
    
    ProceduralHuman() {
        generateGeometry();
    }
    
    ~ProceduralHuman() {
        glDeleteVertexArrays(1, &skinVAO); glDeleteBuffers(1, &skinVBO);
        glDeleteVertexArrays(1, &shirtVAO); glDeleteBuffers(1, &shirtVBO);
        glDeleteVertexArrays(1, &pantVAO); glDeleteBuffers(1, &pantVBO);
        glDeleteVertexArrays(1, &hairVAO); glDeleteBuffers(1, &hairVBO);
        glDeleteVertexArrays(1, &eyeVAO); glDeleteBuffers(1, &eyeVBO);
    }
    
    void generateGeometry() {
        std::vector<float> skinVerts;
        std::vector<float> shirtVerts;
        std::vector<float> pantVerts;
        std::vector<float> hairVerts;
        std::vector<float> eyeVerts;
        
        generateHead(skinVerts, hairVerts, eyeVerts);
        generateBody(shirtVerts, skinVerts); // Neck is skin
        generateArms(shirtVerts, skinVerts); // Sleeves shirt, arms skin
        generateLegs(pantVerts); // Pants, shoes can be special
        
        skinVAO = createVAO(skinVerts); skinCount = skinVerts.size()/8;
        shirtVAO = createVAO(shirtVerts); shirtCount = shirtVerts.size()/8;
        pantVAO = createVAO(pantVerts); pantCount = pantVerts.size()/8;
        hairVAO = createVAO(hairVerts); hairCount = hairVerts.size()/8;
        eyeVAO = createVAO(eyeVerts); eyeCount = eyeVerts.size()/8;
    }
    
    // ============================================================
    // GEOMETRY GENERATORS
    // ============================================================
    
    void generateHead(std::vector<float>& skin, std::vector<float>& hair, std::vector<float>& eyes) {
        // Skull
        addSphere(skin, glm::vec3(0.0f, 0.0f, 0.0f), 0.12f);
        // Jaw
        addBox(skin, glm::vec3(0.0f, -0.1f, 0.02f), glm::vec3(0.14f, 0.1f, 0.15f));
        // Nose
        addBox(skin, glm::vec3(0.0f, -0.02f, 0.12f), glm::vec3(0.04f, 0.06f, 0.04f));
        
        // Eyes
        // Left
        addSphere(eyes, glm::vec3(-0.04f, 0.02f, 0.105f), 0.025f);
        // Right
        addSphere(eyes, glm::vec3(0.04f, 0.02f, 0.105f), 0.025f);
        // Pupils (simplified as part of eye mesh or we'd need another color batch. 
        // For simplicity let's make eyes solid white/dark or mix vertices if we had color attribute. 
        // Using Texture coordinates hack? No.
        // I'll just render pupils as small black spheres in eye batch? No, they need different color transform.
        // I'll put pupils in Hair batch (dark color) for efficiency/hack!
        addSphere(hair, glm::vec3(-0.04f, 0.02f, 0.128f), 0.008f);
        addSphere(hair, glm::vec3(0.04f, 0.02f, 0.128f), 0.008f);
        
        // Ears
        addSphere(skin, glm::vec3(-0.12f, 0.0f, 0.0f), 0.035f);
        addSphere(skin, glm::vec3(0.12f, 0.0f, 0.0f), 0.035f);
        
        // Hair
        addSphere(hair, glm::vec3(0.0f, 0.08f, -0.02f), 0.13f); // Top
        addBox(hair, glm::vec3(0.0f, 0.0f, -0.08f), glm::vec3(0.25f, 0.25f, 0.1f)); // Back
    }
    
    void generateBody(std::vector<float>& shirt, std::vector<float>& skin) {
        // Torso
        addBox(shirt, glm::vec3(0.0f, -0.35f, 0.0f), glm::vec3(0.4f, 0.55f, 0.25f));
        // Neck
        addCylinder(skin, glm::vec3(0.0f, -0.1f, 0.0f), 0.06f, 0.1f);
        // Collar
        addCylinder(shirt, glm::vec3(0.0f, -0.13f, 0.0f), 0.07f, 0.02f);
    }
    
    void generateArms(std::vector<float>& shirt, std::vector<float>& skin) {
        // Left Arm
        addCylinderRotated(shirt, glm::vec3(-0.25f, -0.15f, 0.0f), 0.07f, 0.15f, glm::vec3(0, 0, 45));
        addCylinderRotated(skin, glm::vec3(-0.35f, -0.3f, 0.1f), 0.05f, 0.35f, glm::vec3(30, 0, 20));
        
        // Left Hand
        glm::vec3 handPosL(-0.35f, -0.45f, 0.25f);
        addBox(skin, handPosL, glm::vec3(0.08f, 0.1f, 0.03f));
        for(int i=0; i<5; i++) {
           addCylinderRotated(skin, handPosL + glm::vec3(-0.03f + i*0.015f, -0.06f, 0.0f), 0.006f, 0.06f, glm::vec3(10, 0, 0));
        }
        
        // Right Arm
        addCylinderRotated(shirt, glm::vec3(0.25f, -0.15f, 0.0f), 0.07f, 0.15f, glm::vec3(0, 0, -45));
        addCylinderRotated(skin, glm::vec3(0.35f, -0.3f, 0.1f), 0.05f, 0.35f, glm::vec3(30, 0, -20));
        
        // Right Hand
        glm::vec3 handPosR(0.35f, -0.45f, 0.25f);
        addBox(skin, handPosR, glm::vec3(0.08f, 0.1f, 0.03f));
        for(int i=0; i<5; i++) {
           addCylinderRotated(skin, handPosR + glm::vec3(-0.03f + i*0.015f, -0.06f, 0.0f), 0.006f, 0.06f, glm::vec3(10, 0, 0));
        }
    }
    
    void generateLegs(std::vector<float>& pants) {
        // Thighs
        addCylinderRotated(pants, glm::vec3(-0.12f, -0.65f, 0.2f), 0.08f, 0.5f, glm::vec3(90, 0, 0));
        addCylinderRotated(pants, glm::vec3(0.12f, -0.65f, 0.2f), 0.08f, 0.5f, glm::vec3(90, 0, 0));
        // Lower legs
        addCylinder(pants, glm::vec3(-0.12f, -0.9f, 0.45f), 0.07f, 0.45f);
        addCylinder(pants, glm::vec3(0.12f, -0.9f, 0.45f), 0.07f, 0.45f);
    }
    
    // ============================================================
    // PRIMITIVE HELPER FUNCTIONS
    // ============================================================
    
    void addSphere(std::vector<float>& verts, glm::vec3 center, float radius) {
        int stacks = 12;
        int sectors = 12;
        for(int i = 0; i < stacks; ++i) {
            float lat0 = 3.14159f * (-0.5f + (float)(i) / stacks);
            float z0  = sin(lat0); float zr0 =  cos(lat0);
            float lat1 = 3.14159f * (-0.5f + (float)(i+1) / stacks);
            float z1 = sin(lat1); float zr1 = cos(lat1);
            
            for(int j = 0; j < sectors; ++j) {
                float lng0 = 2 * 3.14159f * (float)(j - 1) / sectors;
                float x0 = cos(lng0); float y0 = sin(lng0);
                float lng1 = 2 * 3.14159f * (float)(j) / sectors;
                float x1 = cos(lng1); float y1 = sin(lng1);
                
                glm::vec3 v0(x0 * zr0, y0 * zr0, z0);
                glm::vec3 v1(x0 * zr1, y0 * zr1, z1);
                glm::vec3 v2(x1 * zr0, y1 * zr0, z0);
                glm::vec3 v3(x1 * zr1, y1 * zr1, z1);
                
                glm::vec3 n0 = glm::normalize(v0);
                glm::vec3 n1 = glm::normalize(v1);
                glm::vec3 n2 = glm::normalize(v2);
                glm::vec3 n3 = glm::normalize(v3);
                
                addVertex(verts, center + v0 * radius, n0);
                addVertex(verts, center + v1 * radius, n1);
                addVertex(verts, center + v2 * radius, n2);
                addVertex(verts, center + v1 * radius, n1);
                addVertex(verts, center + v3 * radius, n3);
                addVertex(verts, center + v2 * radius, n2);
            }
        }
    }
    
    void addBox(std::vector<float>& verts, glm::vec3 center, glm::vec3 size) {
        glm::vec3 half = size * 0.5f;
        glm::vec3 p[8] = {
            center + glm::vec3(-half.x, -half.y, -half.z),
            center + glm::vec3( half.x, -half.y, -half.z),
            center + glm::vec3( half.x,  half.y, -half.z),
            center + glm::vec3(-half.x,  half.y, -half.z),
            center + glm::vec3(-half.x, -half.y,  half.z),
            center + glm::vec3( half.x, -half.y,  half.z),
            center + glm::vec3( half.x,  half.y,  half.z),
            center + glm::vec3(-half.x,  half.y,  half.z)
        };
        addQuad(verts, p[4], p[5], p[6], p[7], glm::vec3(0,0,1));
        addQuad(verts, p[1], p[0], p[3], p[2], glm::vec3(0,0,-1));
        addQuad(verts, p[3], p[2], p[6], p[7], glm::vec3(0,1,0));
        addQuad(verts, p[4], p[5], p[1], p[0], glm::vec3(0,-1,0));
        addQuad(verts, p[5], p[1], p[2], p[6], glm::vec3(1,0,0));
        addQuad(verts, p[0], p[4], p[7], p[3], glm::vec3(-1,0,0));
    }
    
    void addCylinder(std::vector<float>& verts, glm::vec3 center, float radius, float height) {
        addCylinderRotated(verts, center, radius, height, glm::vec3(0.0f));
    }
    
    void addCylinderRotated(std::vector<float>& verts, glm::vec3 center, float radius, float height, glm::vec3 rotation) {
        int segments = 16;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, center);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0,0,1));
        
        for(int i = 0; i < segments; ++i) {
            float theta0 = 2.0f * 3.14159f * i / segments;
            float theta1 = 2.0f * 3.14159f * (i+1) / segments;
            
            glm::vec3 p0(radius * cos(theta0), height/2, radius * sin(theta0));
            glm::vec3 p1(radius * cos(theta1), height/2, radius * sin(theta1));
            glm::vec3 p2(radius * cos(theta0), -height/2, radius * sin(theta0));
            glm::vec3 p3(radius * cos(theta1), -height/2, radius * sin(theta1));
            
            glm::vec3 n0(cos(theta0), 0, sin(theta0));
            glm::vec3 n1(cos(theta1), 0, sin(theta1));
            
            addVertexTransformed(verts, p0, n0, model);
            addVertexTransformed(verts, p2, n0, model);
            addVertexTransformed(verts, p1, n1, model);
            
            addVertexTransformed(verts, p1, n1, model);
            addVertexTransformed(verts, p2, n0, model);
            addVertexTransformed(verts, p3, n1, model);
            
            // Caps
            addVertexTransformed(verts, glm::vec3(0,height/2,0), glm::vec3(0,1,0), model);
            addVertexTransformed(verts, p1, glm::vec3(0,1,0), model);
            addVertexTransformed(verts, p0, glm::vec3(0,1,0), model);
            
            addVertexTransformed(verts, glm::vec3(0,-height/2,0), glm::vec3(0,-1,0), model);
            addVertexTransformed(verts, p2, glm::vec3(0,-1,0), model);
            addVertexTransformed(verts, p3, glm::vec3(0,-1,0), model);
        }
    }
    
    void addQuad(std::vector<float>& verts, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 normal) {
        addVertex(verts, p0, normal); addVertex(verts, p1, normal); addVertex(verts, p2, normal);
        addVertex(verts, p0, normal); addVertex(verts, p2, normal); addVertex(verts, p3, normal);
    }
    
    void addVertex(std::vector<float>& verts, glm::vec3 pos, glm::vec3 normal) {
        verts.push_back(pos.x); verts.push_back(pos.y); verts.push_back(pos.z);
        verts.push_back(normal.x); verts.push_back(normal.y); verts.push_back(normal.z);
        verts.push_back(0.0f); verts.push_back(0.0f);
    }
    
    void addVertexTransformed(std::vector<float>& verts, glm::vec3 pos, glm::vec3 normal, glm::mat4 model) {
        glm::vec4 p = model * glm::vec4(pos, 1.0f);
        glm::vec4 n = model * glm::vec4(normal, 0.0f);
        addVertex(verts, glm::vec3(p), glm::normalize(glm::vec3(n)));
    }
    
    unsigned int createVAO(const std::vector<float>& vertices) {
        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
        return VAO;
    }
    
    void render(Shader& shader, glm::vec3 position, float rotation) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 1, 0));
        
        shader.setMat4("model", model);
        shader.setInt("textureType", 0);
        
        // Skin (Matte/Subsurface feel)
        shader.setVec3("objectColor", skinColor);
        shader.setFloat("ambientStrength", 0.6f);
        shader.setFloat("diffuseStrength", 0.6f);
        shader.setFloat("specularStrength", 0.1f);
        glBindVertexArray(skinVAO);
        glDrawArrays(GL_TRIANGLES, 0, skinCount);
        
        // Shirt (Red fabric)
        shader.setVec3("objectColor", shirtColor);
        shader.setFloat("specularStrength", 0.05f);
        glBindVertexArray(shirtVAO);
        glDrawArrays(GL_TRIANGLES, 0, shirtCount);
        
        // Pants (Dark fabric)
        shader.setVec3("objectColor", pantColor);
        glBindVertexArray(pantVAO);
        glDrawArrays(GL_TRIANGLES, 0, pantCount);
        
        // Hair (Dark, low spec)
        shader.setVec3("objectColor", hairColor);
        shader.setFloat("specularStrength", 0.1f);
        glBindVertexArray(hairVAO);
        glDrawArrays(GL_TRIANGLES, 0, hairCount);
        
        // Eyes (Shiny)
        shader.setVec3("objectColor", eyeWhiteColor);
        shader.setFloat("specularStrength", 0.9f);
        shader.setFloat("shininess", 64.0f);
        glBindVertexArray(eyeVAO);
        glDrawArrays(GL_TRIANGLES, 0, eyeCount);
    }
};

#endif // HUMAN_GENERATOR_H
