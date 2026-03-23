#ifndef CAR_GENERATOR_H
#define CAR_GENERATOR_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "shader.h"

// ============================================================
// PROCEDURAL SEDAN GENERATOR
// High-fidelity curvy sedan with smooth shading
// ============================================================

class ProceduralSedan {
public:
    // VAOs for different car parts
    unsigned int bodyVAO, bodyVBO;
    unsigned int wheelVAO, wheelVBO;
    unsigned int windowVAO, windowVBO;
    unsigned int headlightVAO, headlightVBO;
    unsigned int taillightVAO, taillightVBO;
    unsigned int mirrorVAO, mirrorVBO;
    unsigned int spokeVAO, spokeVBO;
    unsigned int rimVAO, rimVBO;
    unsigned int centerCapVAO, centerCapVBO;
    
    int bodyVertexCount;
    int wheelVertexCount;
    int windowVertexCount;
    int headlightVertexCount;
    int taillightVertexCount;
    int mirrorVertexCount;
    int spokeVertexCount;
    int rimVertexCount;
    int centerCapVertexCount;
    
    // Car dimensions
    float length = 4.5f;
    float width = 1.8f;
    float height = 1.4f;
    float wheelRadius = 0.35f;
    float wheelWidth = 0.22f;
    
    ProceduralSedan() {
        generateAllGeometry();
    }
    
    ~ProceduralSedan() {
        glDeleteVertexArrays(1, &bodyVAO);
        glDeleteBuffers(1, &bodyVBO);
        glDeleteVertexArrays(1, &wheelVAO);
        glDeleteBuffers(1, &wheelVBO);
        glDeleteVertexArrays(1, &windowVAO);
        glDeleteBuffers(1, &windowVBO);
        glDeleteVertexArrays(1, &headlightVAO);
        glDeleteBuffers(1, &headlightVBO);
        glDeleteVertexArrays(1, &taillightVAO);
        glDeleteBuffers(1, &taillightVBO);
        glDeleteVertexArrays(1, &mirrorVAO);
        glDeleteBuffers(1, &mirrorVBO);
        glDeleteVertexArrays(1, &spokeVAO);
        glDeleteBuffers(1, &spokeVBO);
        glDeleteVertexArrays(1, &rimVAO);
        glDeleteBuffers(1, &rimVBO);
        glDeleteVertexArrays(1, &centerCapVAO);
        glDeleteBuffers(1, &centerCapVBO);
    }
    
    // ============================================================
    // BEZIER CURVE UTILITIES
    // ============================================================
    
    // Cubic Bézier interpolation
    static glm::vec3 cubicBezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        
        glm::vec3 p = uuu * p0;
        p += 3.0f * uu * t * p1;
        p += 3.0f * u * tt * p2;
        p += ttt * p3;
        return p;
    }
    
    // Cosine interpolation for smooth curves
    static float cosineInterpolate(float a, float b, float t) {
        float t2 = (1.0f - cos(t * 3.14159f)) / 2.0f;
        return a * (1.0f - t2) + b * t2;
    }
    
    // ============================================================
    // SMOOTH NORMAL CALCULATION
    // ============================================================
    
    static glm::vec3 calculateNormal(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        return glm::normalize(glm::cross(edge1, edge2));
    }
    
    // ============================================================
    // BODY GEOMETRY - Curvy Sedan Profile
    // ============================================================
    
    void generateBodyGeometry(std::vector<float>& vertices) {
        // Car profile control points for Bézier curves
        // Front hood curve
        glm::vec3 frontBumper(0.0f, 0.35f, 0.0f);
        glm::vec3 hoodStart(0.3f, 0.45f, 0.0f);
        glm::vec3 hoodMid(0.8f, 0.55f, 0.0f);
        glm::vec3 windshieldBase(1.1f, 0.6f, 0.0f);
        
        // Windshield curve
        glm::vec3 windshieldTop(1.5f, 1.2f, 0.0f);
        
        // Roof curve
        glm::vec3 roofFront(1.7f, 1.35f, 0.0f);
        glm::vec3 roofMid(2.25f, 1.4f, 0.0f);
        glm::vec3 roofRear(2.8f, 1.35f, 0.0f);
        
        // Rear window curve
        glm::vec3 rearWindowTop(3.0f, 1.2f, 0.0f);
        glm::vec3 trunkStart(3.4f, 0.7f, 0.0f);
        
        // Trunk and rear
        glm::vec3 trunkEnd(4.2f, 0.55f, 0.0f);
        glm::vec3 rearBumper(4.5f, 0.35f, 0.0f);
        
        int segments = 20;
        float halfWidth = width / 2.0f;
        
        // Generate profile points using Bézier curves
        std::vector<glm::vec3> profilePoints;
        
        // Hood curve
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            profilePoints.push_back(cubicBezier(frontBumper, hoodStart, hoodMid, windshieldBase, t));
        }
        
        // Windshield curve
        for (int i = 1; i <= segments; i++) {
            float t = (float)i / segments;
            glm::vec3 p = cubicBezier(windshieldBase, 
                                      glm::vec3(1.2f, 0.8f, 0.0f),
                                      glm::vec3(1.4f, 1.1f, 0.0f),
                                      windshieldTop, t);
            profilePoints.push_back(p);
        }
        
        // Roof curve
        for (int i = 1; i <= segments; i++) {
            float t = (float)i / segments;
            glm::vec3 p = cubicBezier(roofFront, roofMid, roofMid, roofRear, t);
            profilePoints.push_back(p);
        }
        
        // Rear window curve
        for (int i = 1; i <= segments; i++) {
            float t = (float)i / segments;
            glm::vec3 p = cubicBezier(rearWindowTop, 
                                      glm::vec3(3.1f, 1.0f, 0.0f),
                                      glm::vec3(3.2f, 0.85f, 0.0f),
                                      trunkStart, t);
            profilePoints.push_back(p);
        }
        
        // Trunk curve
        for (int i = 1; i <= segments; i++) {
            float t = (float)i / segments;
            glm::vec3 p = cubicBezier(trunkStart, trunkEnd, trunkEnd, rearBumper, t);
            profilePoints.push_back(p);
        }
        
        // Generate body mesh by extruding profile
        for (size_t i = 0; i < profilePoints.size() - 1; i++) {
            glm::vec3 p0 = profilePoints[i];
            glm::vec3 p1 = profilePoints[i + 1];
            
            // Left side vertices
            glm::vec3 v0(p0.x, p0.y, -halfWidth);
            glm::vec3 v1(p1.x, p1.y, -halfWidth);
            // Right side vertices
            glm::vec3 v2(p0.x, p0.y, halfWidth);
            glm::vec3 v3(p1.x, p1.y, halfWidth);
            
            // Generate top surface (connecting left and right profiles)
            glm::vec3 normal = glm::normalize(glm::vec3(-(p1.y - p0.y), p1.x - p0.x, 0.0f));
            
            // Triangle 1
            addVertex(vertices, v0, normal);
            addVertex(vertices, v2, normal);
            addVertex(vertices, v1, normal);
            
            // Triangle 2
            addVertex(vertices, v1, normal);
            addVertex(vertices, v2, normal);
            addVertex(vertices, v3, normal);
        }
        
        // Generate side panels (curved)
        generateSidePanels(vertices, profilePoints, halfWidth);
        
        // Generate bottom
        generateBottom(vertices, halfWidth);
        
        // Generate front face
        generateFrontFace(vertices, halfWidth);
        
        // Generate rear face
        generateRearFace(vertices, halfWidth);
    }
    
    void generateSidePanels(std::vector<float>& vertices, 
                           const std::vector<glm::vec3>& profile, 
                           float halfWidth) {
        // Left side panel
        glm::vec3 leftNormal(0.0f, 0.0f, -1.0f);
        for (size_t i = 0; i < profile.size() - 1; i++) {
            glm::vec3 p0 = profile[i];
            glm::vec3 p1 = profile[i + 1];
            
            glm::vec3 v0(p0.x, p0.y, -halfWidth);
            glm::vec3 v1(p1.x, p1.y, -halfWidth);
            glm::vec3 v2(p0.x, 0.25f, -halfWidth);
            glm::vec3 v3(p1.x, 0.25f, -halfWidth);
            
            addVertex(vertices, v0, leftNormal);
            addVertex(vertices, v2, leftNormal);
            addVertex(vertices, v1, leftNormal);
            
            addVertex(vertices, v1, leftNormal);
            addVertex(vertices, v2, leftNormal);
            addVertex(vertices, v3, leftNormal);
        }
        
        // Right side panel
        glm::vec3 rightNormal(0.0f, 0.0f, 1.0f);
        for (size_t i = 0; i < profile.size() - 1; i++) {
            glm::vec3 p0 = profile[i];
            glm::vec3 p1 = profile[i + 1];
            
            glm::vec3 v0(p0.x, p0.y, halfWidth);
            glm::vec3 v1(p1.x, p1.y, halfWidth);
            glm::vec3 v2(p0.x, 0.25f, halfWidth);
            glm::vec3 v3(p1.x, 0.25f, halfWidth);
            
            addVertex(vertices, v0, rightNormal);
            addVertex(vertices, v1, rightNormal);
            addVertex(vertices, v2, rightNormal);
            
            addVertex(vertices, v1, rightNormal);
            addVertex(vertices, v3, rightNormal);
            addVertex(vertices, v2, rightNormal);
        }
    }
    
    void generateBottom(std::vector<float>& vertices, float halfWidth) {
        glm::vec3 normal(0.0f, -1.0f, 0.0f);
        float y = 0.25f;
        
        glm::vec3 v0(0.0f, y, -halfWidth);
        glm::vec3 v1(length, y, -halfWidth);
        glm::vec3 v2(0.0f, y, halfWidth);
        glm::vec3 v3(length, y, halfWidth);
        
        addVertex(vertices, v0, normal);
        addVertex(vertices, v1, normal);
        addVertex(vertices, v2, normal);
        
        addVertex(vertices, v1, normal);
        addVertex(vertices, v3, normal);
        addVertex(vertices, v2, normal);
    }
    
    void generateFrontFace(std::vector<float>& vertices, float halfWidth) {
        glm::vec3 normal(-1.0f, 0.0f, 0.0f);
        
        // Front grille area
        glm::vec3 v0(0.0f, 0.25f, -halfWidth);
        glm::vec3 v1(0.0f, 0.35f, -halfWidth);
        glm::vec3 v2(0.0f, 0.25f, halfWidth);
        glm::vec3 v3(0.0f, 0.35f, halfWidth);
        
        addVertex(vertices, v0, normal);
        addVertex(vertices, v2, normal);
        addVertex(vertices, v1, normal);
        
        addVertex(vertices, v1, normal);
        addVertex(vertices, v2, normal);
        addVertex(vertices, v3, normal);
    }
    
    void generateRearFace(std::vector<float>& vertices, float halfWidth) {
        glm::vec3 normal(1.0f, 0.0f, 0.0f);
        
        glm::vec3 v0(length, 0.25f, -halfWidth);
        glm::vec3 v1(length, 0.35f, -halfWidth);
        glm::vec3 v2(length, 0.25f, halfWidth);
        glm::vec3 v3(length, 0.35f, halfWidth);
        
        addVertex(vertices, v0, normal);
        addVertex(vertices, v1, normal);
        addVertex(vertices, v2, normal);
        
        addVertex(vertices, v1, normal);
        addVertex(vertices, v3, normal);
        addVertex(vertices, v2, normal);
    }
    
    // ============================================================
    // DETAILED WHEEL GEOMETRY
    // ============================================================
    
    void generateDetailedWheel(std::vector<float>& tireVerts, 
                               std::vector<float>& rimVerts,
                               std::vector<float>& spokeVerts,
                               std::vector<float>& capVerts) {
        int tireSegments = 32;
        int tubeSegments = 16;
        float outerRadius = wheelRadius;
        float innerRadius = wheelRadius * 0.7f;
        float tubeRadius = (outerRadius - innerRadius) / 2.0f;
        float torusRadius = innerRadius + tubeRadius;
        
        // =============== TIRE (Torus) ===============
        for (int i = 0; i < tireSegments; i++) {
            float theta1 = 2.0f * 3.14159f * i / tireSegments;
            float theta2 = 2.0f * 3.14159f * (i + 1) / tireSegments;
            
            for (int j = 0; j < tubeSegments; j++) {
                float phi1 = 2.0f * 3.14159f * j / tubeSegments;
                float phi2 = 2.0f * 3.14159f * (j + 1) / tubeSegments;
                
                // Four vertices of the quad
                glm::vec3 v0 = torusPoint(torusRadius, tubeRadius, theta1, phi1);
                glm::vec3 v1 = torusPoint(torusRadius, tubeRadius, theta2, phi1);
                glm::vec3 v2 = torusPoint(torusRadius, tubeRadius, theta1, phi2);
                glm::vec3 v3 = torusPoint(torusRadius, tubeRadius, theta2, phi2);
                
                glm::vec3 n0 = torusNormal(torusRadius, theta1, phi1);
                glm::vec3 n1 = torusNormal(torusRadius, theta2, phi1);
                glm::vec3 n2 = torusNormal(torusRadius, theta1, phi2);
                glm::vec3 n3 = torusNormal(torusRadius, theta2, phi2);
                
                // Two triangles
                addVertex(tireVerts, v0, n0);
                addVertex(tireVerts, v1, n1);
                addVertex(tireVerts, v2, n2);
                
                addVertex(tireVerts, v1, n1);
                addVertex(tireVerts, v3, n3);
                addVertex(tireVerts, v2, n2);
            }
        }
        
        // =============== RIM (Inner Cylinder) ===============
        float rimRadius = innerRadius * 0.95f;
        float rimDepth = wheelWidth * 0.8f;
        int rimSegments = 32;
        
        for (int i = 0; i < rimSegments; i++) {
            float theta1 = 2.0f * 3.14159f * i / rimSegments;
            float theta2 = 2.0f * 3.14159f * (i + 1) / rimSegments;
            
            // Parametric circle equation: x = r*cos(theta), z = r*sin(theta)
            float x1 = rimRadius * cos(theta1);
            float y1 = rimRadius * sin(theta1);
            float x2 = rimRadius * cos(theta2);
            float y2 = rimRadius * sin(theta2);
            
            // Outer rim surface
            glm::vec3 v0(x1, y1, -rimDepth/2);
            glm::vec3 v1(x2, y2, -rimDepth/2);
            glm::vec3 v2(x1, y1, rimDepth/2);
            glm::vec3 v3(x2, y2, rimDepth/2);
            
            glm::vec3 n0 = glm::normalize(glm::vec3(x1, y1, 0.0f));
            glm::vec3 n1 = glm::normalize(glm::vec3(x2, y2, 0.0f));
            
            addVertex(rimVerts, v0, n0);
            addVertex(rimVerts, v2, n0);
            addVertex(rimVerts, v1, n1);
            
            addVertex(rimVerts, v1, n1);
            addVertex(rimVerts, v2, n0);
            addVertex(rimVerts, v3, n1);
            
            // Front face of rim
            glm::vec3 frontNormal(0.0f, 0.0f, -1.0f);
            glm::vec3 center(0.0f, 0.0f, -rimDepth/2);
            addVertex(rimVerts, center, frontNormal);
            addVertex(rimVerts, v0, frontNormal);
            addVertex(rimVerts, v1, frontNormal);
        }
        
        // =============== SPOKES (5 spokes) ===============
        int numSpokes = 5;
        float spokeWidth = 0.03f;
        float spokeDepth = 0.02f;
        float hubRadius = 0.05f;
        
        for (int i = 0; i < numSpokes; i++) {
            // Parametric positioning using circle equation
            float theta = 2.0f * 3.14159f * i / numSpokes;
            
            // Spoke goes from hub to rim
            float startX = hubRadius * cos(theta);
            float startY = hubRadius * sin(theta);
            float endX = (rimRadius - 0.02f) * cos(theta);
            float endY = (rimRadius - 0.02f) * sin(theta);
            
            // Create spoke as a thin box
            glm::vec3 dir = glm::normalize(glm::vec3(endX - startX, endY - startY, 0.0f));
            glm::vec3 perp(-dir.y, dir.x, 0.0f);
            
            float spokeLen = glm::length(glm::vec2(endX - startX, endY - startY));
            float midX = (startX + endX) / 2.0f;
            float midY = (startY + endY) / 2.0f;
            float zPos = -rimDepth/2 + 0.01f;
            
            // Spoke vertices (flat box)
            glm::vec3 v0 = glm::vec3(midX, midY, zPos) + perp * spokeWidth/2.0f - dir * spokeLen/2.0f;
            glm::vec3 v1 = glm::vec3(midX, midY, zPos) - perp * spokeWidth/2.0f - dir * spokeLen/2.0f;
            glm::vec3 v2 = glm::vec3(midX, midY, zPos) + perp * spokeWidth/2.0f + dir * spokeLen/2.0f;
            glm::vec3 v3 = glm::vec3(midX, midY, zPos) - perp * spokeWidth/2.0f + dir * spokeLen/2.0f;
            
            glm::vec3 spokeNormal(0.0f, 0.0f, -1.0f);
            addVertex(spokeVerts, v0, spokeNormal);
            addVertex(spokeVerts, v1, spokeNormal);
            addVertex(spokeVerts, v2, spokeNormal);
            
            addVertex(spokeVerts, v1, spokeNormal);
            addVertex(spokeVerts, v3, spokeNormal);
            addVertex(spokeVerts, v2, spokeNormal);
        }
        
        // =============== CENTER CAP ===============
        float capRadius = hubRadius * 1.5f;
        float capDepth = 0.03f;
        int capSegments = 16;
        
        for (int i = 0; i < capSegments; i++) {
            float theta1 = 2.0f * 3.14159f * i / capSegments;
            float theta2 = 2.0f * 3.14159f * (i + 1) / capSegments;
            
            float x1 = capRadius * cos(theta1);
            float y1 = capRadius * sin(theta1);
            float x2 = capRadius * cos(theta2);
            float y2 = capRadius * sin(theta2);
            
            float zFront = -rimDepth/2 - capDepth;
            
            // Front face (cap)
            glm::vec3 frontNormal(0.0f, 0.0f, -1.0f);
            addVertex(capVerts, glm::vec3(0.0f, 0.0f, zFront), frontNormal);
            addVertex(capVerts, glm::vec3(x1, y1, zFront), frontNormal);
            addVertex(capVerts, glm::vec3(x2, y2, zFront), frontNormal);
            
            // Side of cap
            glm::vec3 sideN1 = glm::normalize(glm::vec3(x1, y1, 0.0f));
            glm::vec3 sideN2 = glm::normalize(glm::vec3(x2, y2, 0.0f));
            
            addVertex(capVerts, glm::vec3(x1, y1, zFront), sideN1);
            addVertex(capVerts, glm::vec3(x1, y1, -rimDepth/2), sideN1);
            addVertex(capVerts, glm::vec3(x2, y2, zFront), sideN2);
            
            addVertex(capVerts, glm::vec3(x2, y2, zFront), sideN2);
            addVertex(capVerts, glm::vec3(x1, y1, -rimDepth/2), sideN1);
            addVertex(capVerts, glm::vec3(x2, y2, -rimDepth/2), sideN2);
        }
    }
    
    glm::vec3 torusPoint(float R, float r, float theta, float phi) {
        float x = (R + r * cos(phi)) * cos(theta);
        float y = (R + r * cos(phi)) * sin(theta);
        float z = r * sin(phi);
        return glm::vec3(x, y, z);
    }
    
    glm::vec3 torusNormal(float R, float theta, float phi) {
        float nx = cos(phi) * cos(theta);
        float ny = cos(phi) * sin(theta);
        float nz = sin(phi);
        return glm::normalize(glm::vec3(nx, ny, nz));
    }
    
    // ============================================================
    // WINDOW GEOMETRY
    // ============================================================
    
    void generateWindowGeometry(std::vector<float>& vertices) {
        float halfWidth = width / 2.0f;
        float inset = 0.02f;  // Slightly inset into frame
        
        // Front windshield (curved to match body)
        glm::vec3 wsNormal = glm::normalize(glm::vec3(-0.5f, 0.7f, 0.0f));
        glm::vec3 ws0(1.15f, 0.65f, -halfWidth + 0.1f);
        glm::vec3 ws1(1.15f, 0.65f, halfWidth - 0.1f);
        glm::vec3 ws2(1.55f, 1.18f, -halfWidth + 0.15f);
        glm::vec3 ws3(1.55f, 1.18f, halfWidth - 0.15f);
        
        addVertex(vertices, ws0 + wsNormal * inset, wsNormal);
        addVertex(vertices, ws1 + wsNormal * inset, wsNormal);
        addVertex(vertices, ws2 + wsNormal * inset, wsNormal);
        
        addVertex(vertices, ws1 + wsNormal * inset, wsNormal);
        addVertex(vertices, ws3 + wsNormal * inset, wsNormal);
        addVertex(vertices, ws2 + wsNormal * inset, wsNormal);
        
        // Rear window
        glm::vec3 rwNormal = glm::normalize(glm::vec3(0.5f, 0.6f, 0.0f));
        glm::vec3 rw0(2.95f, 1.15f, -halfWidth + 0.15f);
        glm::vec3 rw1(2.95f, 1.15f, halfWidth - 0.15f);
        glm::vec3 rw2(3.35f, 0.75f, -halfWidth + 0.1f);
        glm::vec3 rw3(3.35f, 0.75f, halfWidth - 0.1f);
        
        addVertex(vertices, rw0 + rwNormal * inset, rwNormal);
        addVertex(vertices, rw1 + rwNormal * inset, rwNormal);
        addVertex(vertices, rw2 + rwNormal * inset, rwNormal);
        
        addVertex(vertices, rw1 + rwNormal * inset, rwNormal);
        addVertex(vertices, rw3 + rwNormal * inset, rwNormal);
        addVertex(vertices, rw2 + rwNormal * inset, rwNormal);
        
        // Left side windows (front and rear)
        glm::vec3 leftNormal(0.0f, 0.0f, -1.0f);
        // Front left
        addVertex(vertices, glm::vec3(1.6f, 0.7f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(2.2f, 0.7f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(1.7f, 1.25f, -halfWidth - inset), leftNormal);
        
        addVertex(vertices, glm::vec3(2.2f, 0.7f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(2.2f, 1.3f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(1.7f, 1.25f, -halfWidth - inset), leftNormal);
        
        // Rear left
        addVertex(vertices, glm::vec3(2.3f, 0.7f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(2.85f, 0.7f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(2.3f, 1.3f, -halfWidth - inset), leftNormal);
        
        addVertex(vertices, glm::vec3(2.85f, 0.7f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(2.8f, 1.2f, -halfWidth - inset), leftNormal);
        addVertex(vertices, glm::vec3(2.3f, 1.3f, -halfWidth - inset), leftNormal);
        
        // Right side windows
        glm::vec3 rightNormal(0.0f, 0.0f, 1.0f);
        // Front right
        addVertex(vertices, glm::vec3(1.6f, 0.7f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(1.7f, 1.25f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(2.2f, 0.7f, halfWidth + inset), rightNormal);
        
        addVertex(vertices, glm::vec3(2.2f, 0.7f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(1.7f, 1.25f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(2.2f, 1.3f, halfWidth + inset), rightNormal);
        
        // Rear right
        addVertex(vertices, glm::vec3(2.3f, 0.7f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(2.3f, 1.3f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(2.85f, 0.7f, halfWidth + inset), rightNormal);
        
        addVertex(vertices, glm::vec3(2.85f, 0.7f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(2.3f, 1.3f, halfWidth + inset), rightNormal);
        addVertex(vertices, glm::vec3(2.8f, 1.2f, halfWidth + inset), rightNormal);
        
        // Sunroof (distinct quad on roof)
        glm::vec3 roofNormal(0.0f, 1.0f, 0.0f);
        addVertex(vertices, glm::vec3(1.9f, 1.38f, -0.3f), roofNormal);
        addVertex(vertices, glm::vec3(2.6f, 1.38f, -0.3f), roofNormal);
        addVertex(vertices, glm::vec3(1.9f, 1.38f, 0.3f), roofNormal);
        
        addVertex(vertices, glm::vec3(2.6f, 1.38f, -0.3f), roofNormal);
        addVertex(vertices, glm::vec3(2.6f, 1.38f, 0.3f), roofNormal);
        addVertex(vertices, glm::vec3(1.9f, 1.38f, 0.3f), roofNormal);
    }
    
    // ============================================================
    // HEADLIGHTS & TAILLIGHTS
    // ============================================================
    
    void generateHeadlights(std::vector<float>& vertices) {
        float halfWidth = width / 2.0f;
        
        // Left headlight (curved to match body)
        generateCurvedLight(vertices, glm::vec3(0.05f, 0.4f, -halfWidth + 0.15f), 0.12f, 0.08f, true);
        // Right headlight
        generateCurvedLight(vertices, glm::vec3(0.05f, 0.4f, halfWidth - 0.15f), 0.12f, 0.08f, true);
    }
    
    void generateTaillights(std::vector<float>& vertices) {
        float halfWidth = width / 2.0f;
        
        // Left taillight
        generateCurvedLight(vertices, glm::vec3(length - 0.05f, 0.45f, -halfWidth + 0.15f), 0.1f, 0.06f, false);
        // Right taillight
        generateCurvedLight(vertices, glm::vec3(length - 0.05f, 0.45f, halfWidth - 0.15f), 0.1f, 0.06f, false);
    }
    
    void generateCurvedLight(std::vector<float>& vertices, glm::vec3 center, 
                            float width, float height, bool isFront) {
        int segments = 8;
        glm::vec3 normal = isFront ? glm::vec3(-1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        
        for (int i = 0; i < segments; i++) {
            float a1 = 3.14159f * i / segments - 3.14159f / 2;
            float a2 = 3.14159f * (i + 1) / segments - 3.14159f / 2;
            
            float x1 = (isFront ? -1 : 1) * 0.02f * (1 + cos(a1));
            float y1 = center.y + height * sin(a1);
            float x2 = (isFront ? -1 : 1) * 0.02f * (1 + cos(a2));
            float y2 = center.y + height * sin(a2);
            
            glm::vec3 v0(center.x + x1, y1, center.z - width/2);
            glm::vec3 v1(center.x + x1, y1, center.z + width/2);
            glm::vec3 v2(center.x + x2, y2, center.z - width/2);
            glm::vec3 v3(center.x + x2, y2, center.z + width/2);
            
            addVertex(vertices, v0, normal);
            addVertex(vertices, v1, normal);
            addVertex(vertices, v2, normal);
            
            addVertex(vertices, v1, normal);
            addVertex(vertices, v3, normal);
            addVertex(vertices, v2, normal);
        }
    }
    
    // ============================================================
    // SIDE MIRRORS
    // ============================================================
    
    void generateMirrors(std::vector<float>& vertices) {
        float halfWidth = width / 2.0f;
        
        // Left mirror
        generateMirror(vertices, glm::vec3(1.3f, 0.9f, -halfWidth - 0.1f), true);
        // Right mirror
        generateMirror(vertices, glm::vec3(1.3f, 0.9f, halfWidth + 0.1f), false);
    }
    
    void generateMirror(std::vector<float>& vertices, glm::vec3 base, bool isLeft) {
        // Mirror arm (small cylinder)
        int segments = 8;
        float armRadius = 0.015f;
        float armLen = 0.08f;
        float mirrorRadius = 0.05f;
        
        float zDir = isLeft ? -1.0f : 1.0f;
        
        // Arm
        for (int i = 0; i < segments; i++) {
            float theta1 = 2.0f * 3.14159f * i / segments;
            float theta2 = 2.0f * 3.14159f * (i + 1) / segments;
            
            float dx1 = armRadius * cos(theta1);
            float dy1 = armRadius * sin(theta1);
            float dx2 = armRadius * cos(theta2);
            float dy2 = armRadius * sin(theta2);
            
            glm::vec3 v0 = base + glm::vec3(dx1, dy1, 0);
            glm::vec3 v1 = base + glm::vec3(dx2, dy2, 0);
            glm::vec3 v2 = base + glm::vec3(dx1, dy1, zDir * armLen);
            glm::vec3 v3 = base + glm::vec3(dx2, dy2, zDir * armLen);
            
            glm::vec3 n = glm::normalize(glm::vec3(dx1 + dx2, dy1 + dy2, 0.0f) / 2.0f);
            
            addVertex(vertices, v0, n);
            addVertex(vertices, v2, n);
            addVertex(vertices, v1, n);
            
            addVertex(vertices, v1, n);
            addVertex(vertices, v2, n);
            addVertex(vertices, v3, n);
        }
        
        // Mirror housing (sphere-like)
        glm::vec3 mirrorCenter = base + glm::vec3(0, 0, zDir * (armLen + mirrorRadius * 0.5f));
        for (int i = 0; i < segments; i++) {
            float theta1 = 2.0f * 3.14159f * i / segments;
            float theta2 = 2.0f * 3.14159f * (i + 1) / segments;
            
            for (int j = 0; j < segments / 2; j++) {
                float phi1 = 3.14159f * j / (segments / 2) - 3.14159f / 2;
                float phi2 = 3.14159f * (j + 1) / (segments / 2) - 3.14159f / 2;
                
                glm::vec3 v0 = mirrorCenter + spherePoint(mirrorRadius, theta1, phi1);
                glm::vec3 v1 = mirrorCenter + spherePoint(mirrorRadius, theta2, phi1);
                glm::vec3 v2 = mirrorCenter + spherePoint(mirrorRadius, theta1, phi2);
                glm::vec3 v3 = mirrorCenter + spherePoint(mirrorRadius, theta2, phi2);
                
                glm::vec3 n0 = glm::normalize(v0 - mirrorCenter);
                glm::vec3 n1 = glm::normalize(v1 - mirrorCenter);
                glm::vec3 n2 = glm::normalize(v2 - mirrorCenter);
                glm::vec3 n3 = glm::normalize(v3 - mirrorCenter);
                
                addVertex(vertices, v0, n0);
                addVertex(vertices, v1, n1);
                addVertex(vertices, v2, n2);
                
                addVertex(vertices, v1, n1);
                addVertex(vertices, v3, n3);
                addVertex(vertices, v2, n2);
            }
        }
    }
    
    glm::vec3 spherePoint(float r, float theta, float phi) {
        return glm::vec3(
            r * cos(phi) * cos(theta),
            r * sin(phi),
            r * cos(phi) * sin(theta)
        );
    }
    
    // ============================================================
    // UTILITY FUNCTIONS
    // ============================================================
    
    void addVertex(std::vector<float>& verts, glm::vec3 pos, glm::vec3 normal) {
        verts.push_back(pos.x);
        verts.push_back(pos.y);
        verts.push_back(pos.z);
        verts.push_back(normal.x);
        verts.push_back(normal.y);
        verts.push_back(normal.z);
        verts.push_back(0.0f);  // U
        verts.push_back(0.0f);  // V
    }
    
    unsigned int createVAO(const std::vector<float>& vertices) {
        unsigned int VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // TexCoords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        return VAO;
    }
    
    void generateAllGeometry() {
        // Body
        std::vector<float> bodyVerts;
        generateBodyGeometry(bodyVerts);
        bodyVAO = createVAO(bodyVerts);
        bodyVertexCount = bodyVerts.size() / 8;
        
        // Wheels
        std::vector<float> tireVerts, rimVerts, spokeVerts, capVerts;
        generateDetailedWheel(tireVerts, rimVerts, spokeVerts, capVerts);
        wheelVAO = createVAO(tireVerts);
        wheelVertexCount = tireVerts.size() / 8;
        rimVAO = createVAO(rimVerts);
        rimVertexCount = rimVerts.size() / 8;
        spokeVAO = createVAO(spokeVerts);
        spokeVertexCount = spokeVerts.size() / 8;
        centerCapVAO = createVAO(capVerts);
        centerCapVertexCount = capVerts.size() / 8;
        
        // Windows
        std::vector<float> windowVerts;
        generateWindowGeometry(windowVerts);
        windowVAO = createVAO(windowVerts);
        windowVertexCount = windowVerts.size() / 8;
        
        // Headlights
        std::vector<float> headlightVerts;
        generateHeadlights(headlightVerts);
        headlightVAO = createVAO(headlightVerts);
        headlightVertexCount = headlightVerts.size() / 8;
        
        // Taillights
        std::vector<float> taillightVerts;
        generateTaillights(taillightVerts);
        taillightVAO = createVAO(taillightVerts);
        taillightVertexCount = taillightVerts.size() / 8;
        
        // Mirrors
        std::vector<float> mirrorVerts;
        generateMirrors(mirrorVerts);
        mirrorVAO = createVAO(mirrorVerts);
        mirrorVertexCount = mirrorVerts.size() / 8;
    }
    
    // ============================================================
    // RENDERING
    // ============================================================
    
    void render(Shader& shader, glm::vec3 position, float rotation, glm::vec3 bodyColor, float wheelAngle = 0.0f) {
        glm::mat4 baseModel = glm::mat4(1.0f);
        baseModel = glm::translate(baseModel, position);
        baseModel = glm::rotate(baseModel, glm::radians(rotation), glm::vec3(0, 1, 0));
        
        // Body
        shader.setMat4("model", baseModel);
        shader.setVec3("objectColor", bodyColor);
        shader.setInt("textureType", 2);  // Car paint
        shader.setFloat("ambientStrength", 0.15f);
        shader.setFloat("diffuseStrength", 0.7f);
        shader.setFloat("specularStrength", 0.8f);
        shader.setFloat("shininess", 64.0f);
        glBindVertexArray(bodyVAO);
        glDrawArrays(GL_TRIANGLES, 0, bodyVertexCount);
        
        // Windows (glass)
        shader.setVec3("objectColor", glm::vec3(0.1f, 0.1f, 0.15f));
        shader.setInt("textureType", 4);  // Glass
        shader.setFloat("ambientStrength", 0.1f);
        shader.setFloat("diffuseStrength", 0.2f);
        shader.setFloat("specularStrength", 1.0f);
        shader.setFloat("shininess", 128.0f);
        glBindVertexArray(windowVAO);
        glDrawArrays(GL_TRIANGLES, 0, windowVertexCount);
        
        // Headlights
        shader.setVec3("objectColor", glm::vec3(1.0f, 0.98f, 0.9f));
        shader.setInt("textureType", 4);
        shader.setFloat("ambientStrength", 0.8f);
        shader.setFloat("diffuseStrength", 0.3f);
        shader.setFloat("specularStrength", 0.9f);
        shader.setFloat("shininess", 96.0f);
        glBindVertexArray(headlightVAO);
        glDrawArrays(GL_TRIANGLES, 0, headlightVertexCount);
        
        // Taillights
        shader.setVec3("objectColor", glm::vec3(0.8f, 0.1f, 0.1f));
        shader.setFloat("ambientStrength", 0.5f);
        glBindVertexArray(taillightVAO);
        glDrawArrays(GL_TRIANGLES, 0, taillightVertexCount);
        
        // Mirrors
        shader.setVec3("objectColor", bodyColor * 0.8f);
        shader.setInt("textureType", 2);
        shader.setFloat("ambientStrength", 0.12f);
        shader.setFloat("diffuseStrength", 0.65f);
        shader.setFloat("specularStrength", 0.7f);
        shader.setFloat("shininess", 48.0f);
        glBindVertexArray(mirrorVAO);
        glDrawArrays(GL_TRIANGLES, 0, mirrorVertexCount);
        
        // Wheels (4 wheels) with spin animation
        renderWheel(shader, baseModel, glm::vec3(0.9f, wheelRadius, -width/2 - 0.02f), wheelAngle);
        renderWheel(shader, baseModel, glm::vec3(0.9f, wheelRadius, width/2 + 0.02f), wheelAngle);
        renderWheel(shader, baseModel, glm::vec3(3.3f, wheelRadius, -width/2 - 0.02f), wheelAngle);
        renderWheel(shader, baseModel, glm::vec3(3.3f, wheelRadius, width/2 + 0.02f), wheelAngle);
    }
    
    void renderWheel(Shader& shader, glm::mat4 baseModel, glm::vec3 offset, float spinAngle = 0.0f) {
        glm::mat4 wheelModel = baseModel;
        wheelModel = glm::translate(wheelModel, offset);
        // Wheel axle is along Z. Rolling spin + aesthetic 90° are both around Z.
        wheelModel = glm::rotate(wheelModel, glm::radians(-spinAngle + 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        
        // Tire (black rubber)
        shader.setMat4("model", wheelModel);
        shader.setVec3("objectColor", glm::vec3(0.1f, 0.1f, 0.1f));
        shader.setInt("textureType", 0);
        shader.setFloat("ambientStrength", 0.1f);
        shader.setFloat("diffuseStrength", 0.8f);
        shader.setFloat("specularStrength", 0.15f);
        shader.setFloat("shininess", 8.0f);
        glBindVertexArray(wheelVAO);
        glDrawArrays(GL_TRIANGLES, 0, wheelVertexCount);
        
        // Rim (metallic silver)
        shader.setVec3("objectColor", glm::vec3(0.75f, 0.75f, 0.8f));
        shader.setInt("textureType", 3);  // Metal
        shader.setFloat("ambientStrength", 0.15f);
        shader.setFloat("diffuseStrength", 0.6f);
        shader.setFloat("specularStrength", 0.9f);
        shader.setFloat("shininess", 96.0f);
        glBindVertexArray(rimVAO);
        glDrawArrays(GL_TRIANGLES, 0, rimVertexCount);
        
        // Spokes
        shader.setVec3("objectColor", glm::vec3(0.8f, 0.8f, 0.85f));
        glBindVertexArray(spokeVAO);
        glDrawArrays(GL_TRIANGLES, 0, spokeVertexCount);
        
        // Center cap
        shader.setVec3("objectColor", glm::vec3(0.3f, 0.3f, 0.35f));
        glBindVertexArray(centerCapVAO);
        glDrawArrays(GL_TRIANGLES, 0, centerCapVertexCount);
    }
};

#endif // CAR_GENERATOR_H
