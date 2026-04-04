#ifndef THIRD_FLOOR_LAYOUT_H
#define THIRD_FLOOR_LAYOUT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "shader.h"
#include "npc.h"
#include "tree.h"

// ─────────────────────────────────────────────────────────────────────────────
// External helpers (defined in main.cpp / utils)
// ─────────────────────────────────────────────────────────────────────────────
extern void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
                     glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
extern void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
                            glm::vec3 rotation, glm::vec3 color, int texType,
                            float ambient, float diffuse, float specular, float shininess);

namespace ThirdFloorDesign {

// ─────────────────────────────────────────────────────────────────────────────
// RENDER CONTEXT
// ─────────────────────────────────────────────────────────────────────────────

struct RenderContext {
    Shader*      shader       = nullptr;
    unsigned int cubeVAO      = 0;
    unsigned int cylVAO       = 0;
    unsigned int sphereVAO    = 0;
    int          sphereCount  = 0;
    int          cylSegments  = 16;
    // Optional textures (0 = untextured fallback)
    unsigned int texCartMenu  = 0;   // burger/pizza menu sign
    unsigned int texCoffeeLogo= 0;   // coffee shop logo
    unsigned int texTableTop  = 0;   // marble/wood for tabletop
    unsigned int texRestaurantLogos[4] = {0}; // massive restaurant brand signs
};

inline RenderContext& ctx() {
    static RenderContext c;
    return c;
}

inline void setRenderContext(Shader& shader,
                             unsigned int cubeVAO,
                             unsigned int cylVAO,
                             unsigned int sphereVAO,
                             int          sphereCount,
                             int          cylSegments  = 16,
                             unsigned int texCartMenu  = 0,
                             unsigned int texCoffeeLogo= 0,
                             unsigned int texTableTop  = 0,
                             unsigned int* texRestaurantLogos = nullptr) {
    RenderContext& c = ctx();
    c.shader        = &shader;
    c.cubeVAO       = cubeVAO;
    c.cylVAO        = cylVAO;
    c.sphereVAO     = sphereVAO;
    c.sphereCount   = sphereCount;
    c.cylSegments   = cylSegments;
    c.texCartMenu   = texCartMenu;
    c.texCoffeeLogo = texCoffeeLogo;
    c.texTableTop   = texTableTop;
    if (texRestaurantLogos) {
        for (int i=0; i<4; i++) {
            c.texRestaurantLogos[i] = texRestaurantLogos[i];
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPER: push a quad (two triangles) with normals and UVs into a vertex list
// stride = pos(3) + normal(3) + uv(2) = 8 floats per vertex
// ─────────────────────────────────────────────────────────────────────────────
static inline void pushQuad(std::vector<float>& v,
    glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
    glm::vec3 n,
    glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3) {
    auto push = [&](glm::vec3 p, glm::vec2 uv) {
        v.insert(v.end(), {p.x,p.y,p.z, n.x,n.y,n.z, uv.x,uv.y});
    };
    push(p0,uv0); push(p1,uv1); push(p2,uv2);
    push(p0,uv0); push(p2,uv2); push(p3,uv3);
}

// ─────────────────────────────────────────────────────────────────────────────
// 11-FLOAT GEOMETRY HELPERS FOR RESTAURANT
// stride = pos(3) + normal(3) + uv(2) + color(3) = 11 floats per vertex
// ─────────────────────────────────────────────────────────────────────────────
static inline void pushQuad11(std::vector<float>& v,
    glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
    glm::vec3 n,
    glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, glm::vec3 col) {
    auto push = [&](glm::vec3 p, glm::vec2 uv) {
        v.insert(v.end(), {p.x,p.y,p.z, n.x,n.y,n.z, uv.x,uv.y, col.r,col.g,col.b});
    };
    push(p0,uv0); push(p1,uv1); push(p2,uv2);
    push(p0,uv0); push(p2,uv2); push(p3,uv3);
}

static inline void pushBox11(std::vector<float>& v, glm::vec3 center, glm::vec3 halfExtents, glm::vec3 col, glm::vec2 uvMin = {0,0}, glm::vec2 uvMax = {1,1}) {
    float cx=center.x, cy=center.y, cz=center.z;
    float hx=halfExtents.x, hy=halfExtents.y, hz=halfExtents.z;
    pushQuad11(v, {cx-hx, cy-hy, cz+hz}, {cx+hx, cy-hy, cz+hz}, {cx+hx, cy+hy, cz+hz}, {cx-hx, cy+hy, cz+hz}, {0,0,1}, {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}, col);
    pushQuad11(v, {cx+hx, cy-hy, cz-hz}, {cx-hx, cy-hy, cz-hz}, {cx-hx, cy+hy, cz-hz}, {cx+hx, cy+hy, cz-hz}, {0,0,-1}, {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}, col);
    pushQuad11(v, {cx-hx, cy-hy, cz-hz}, {cx-hx, cy-hy, cz+hz}, {cx-hx, cy+hy, cz+hz}, {cx-hx, cy+hy, cz-hz}, {-1,0,0}, {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}, col);
    pushQuad11(v, {cx+hx, cy-hy, cz+hz}, {cx+hx, cy-hy, cz-hz}, {cx+hx, cy+hy, cz-hz}, {cx+hx, cy+hy, cz+hz}, {1,0,0}, {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}, col);
    pushQuad11(v, {cx-hx, cy+hy, cz+hz}, {cx+hx, cy+hy, cz+hz}, {cx+hx, cy+hy, cz-hz}, {cx-hx, cy+hy, cz-hz}, {0,1,0}, {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}, col);
    pushQuad11(v, {cx-hx, cy-hy, cz-hz}, {cx+hx, cy-hy, cz-hz}, {cx+hx, cy-hy, cz+hz}, {cx-hx, cy-hy, cz+hz}, {0,-1,0}, {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y}, col);
}

static inline void pushRotatedBox11(std::vector<float>& v, glm::vec3 center, glm::vec3 halfExtents, float angleY, glm::vec3 col) {
    float s = sin(angleY), c = cos(angleY);
    auto rotate = [&](float x, float z) -> glm::vec2 { return {x * c - z * s, x * s + z * c}; };
    float hx = halfExtents.x, hy = halfExtents.y, hz = halfExtents.z;
    glm::vec3 pts[8];
    for(int i=0; i<8; i++) {
        float x = (i & 1) ? hx : -hx;
        float y = (i & 2) ? hy : -hy;
        float z = (i & 4) ? hz : -hz;
        glm::vec2 r = rotate(x, z);
        pts[i] = center + glm::vec3(r.x, y, r.y);
    }
    auto pq = [&](int a, int b, int c_idx, int d, glm::vec3 n) { pushQuad11(v, pts[a], pts[b], pts[c_idx], pts[d], n, {0,0},{1,0},{1,1},{0,1}, col); };
    glm::vec2 n1 = rotate(0, 1), n2 = rotate(0, -1), n3 = rotate(-1, 0), n4 = rotate(1, 0);
    pq(4, 5, 7, 6, {n1.x, 0, n1.y}); 
    pq(1, 0, 2, 3, {n2.x, 0, n2.y}); 
    pq(0, 4, 6, 2, {n3.x, 0, n3.y}); 
    pq(5, 1, 3, 7, {n4.x, 0, n4.y}); 
    pq(2, 6, 7, 3, {0, 1, 0});       
    pq(0, 1, 5, 4, {0, -1, 0});      
}

static inline void pushCylinder11(std::vector<float>& v, glm::vec3 center, float radius, float height, int slices, glm::vec3 col) {
    float halfH = height * 0.5f;
    float step = glm::two_pi<float>() / slices;
    for (int i = 0; i < slices; i++) {
        float a0 = i * step, a1 = (i + 1) * step;
        float x0 = cos(a0) * radius, z0 = sin(a0) * radius;
        float x1 = cos(a1) * radius, z1 = sin(a1) * radius;
        glm::vec3 b0(center.x+x0, center.y-halfH, center.z+z0);
        glm::vec3 b1(center.x+x1, center.y-halfH, center.z+z1);
        glm::vec3 t0(center.x+x0, center.y+halfH, center.z+z0);
        glm::vec3 t1(center.x+x1, center.y+halfH, center.z+z1);
        glm::vec3 n0 = glm::normalize(glm::vec3(x0, 0, z0)), n1 = glm::normalize(glm::vec3(x1, 0, z1));
        pushQuad11(v, b0, b1, t1, t0, glm::normalize(n0+n1), {0,0}, {1,0}, {1,1}, {0,1}, col);
        // Top cap triangle
        v.insert(v.end(), {center.x, center.y+halfH, center.z, 0,1,0, 0.5f,0.5f, col.r,col.g,col.b});
        v.insert(v.end(), {t1.x, t1.y, t1.z, 0,1,0, 0.5f+x1/(2*radius), 0.5f+z1/(2*radius), col.r,col.g,col.b});
        v.insert(v.end(), {t0.x, t0.y, t0.z, 0,1,0, 0.5f+x0/(2*radius), 0.5f+z0/(2*radius), col.r,col.g,col.b});
    }
}

static inline unsigned int uploadVAO11(const std::vector<float>& verts) {
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11*sizeof(float), (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
    return VAO;
}

// ─────────────────────────────────────────────────────────────────────────────
// VAO helpers
// ─────────────────────────────────────────────────────────────────────────────
static inline unsigned int uploadVAO(const std::vector<float>& verts) {
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return VAO;
}

// Append a cylinder ring (closed top or open) into a vertex buffer
static inline void appendCylinder(std::vector<float>& v,
    glm::vec3 centre, float radius, float halfHeight, int segs,
    bool caps = true) {
    float step = glm::two_pi<float>() / segs;
    for (int i = 0; i < segs; ++i) {
        float a0 = step * i, a1 = step * (i + 1);
        float x0 = cosf(a0)*radius, z0 = sinf(a0)*radius;
        float x1 = cosf(a1)*radius, z1 = sinf(a1)*radius;
        glm::vec3 n0 = glm::normalize(glm::vec3(x0,0,z0));
        glm::vec3 n1 = glm::normalize(glm::vec3(x1,0,z1));
        float u0 = (float)i/segs, u1 = (float)(i+1)/segs;
        // side strip
        glm::vec3 bl(centre.x+x0, centre.y-halfHeight, centre.z+z0);
        glm::vec3 br(centre.x+x1, centre.y-halfHeight, centre.z+z1);
        glm::vec3 tl(centre.x+x0, centre.y+halfHeight, centre.z+z0);
        glm::vec3 tr(centre.x+x1, centre.y+halfHeight, centre.z+z1);
        pushQuad(v, bl,br,tr,tl, (n0+n1)*0.5f,
            {u0,0},{u1,0},{u1,1},{u0,1});
        if (caps) {
            // bottom cap tri
            v.insert(v.end(),{centre.x,centre.y-halfHeight,centre.z, 0,-1,0, 0.5f,0.5f});
            v.insert(v.end(),{bl.x,bl.y,bl.z, 0,-1,0, 0.5f+0.5f*cosf(a0),0.5f+0.5f*sinf(a0)});
            v.insert(v.end(),{br.x,br.y,br.z, 0,-1,0, 0.5f+0.5f*cosf(a1),0.5f+0.5f*sinf(a1)});
            // top cap tri
            v.insert(v.end(),{centre.x,centre.y+halfHeight,centre.z, 0,1,0, 0.5f,0.5f});
            v.insert(v.end(),{tr.x,tr.y,tr.z, 0,1,0, 0.5f+0.5f*cosf(a1),0.5f+0.5f*sinf(a1)});
            v.insert(v.end(),{tl.x,tl.y,tl.z, 0,1,0, 0.5f+0.5f*cosf(a0),0.5f+0.5f*sinf(a0)});
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LARGE RESTAURANT SHOP (11-float vertex geometry)
// ─────────────────────────────────────────────────────────────────────────────
inline unsigned int createLargeRestaurantVAO(int& outCount) {
    std::vector<float> v;

    float hw = 8.0f;  // Width: 16m wide
    float hd = 6.0f;  // Depth: 12m deep
    float h = 6.0f;   // Height: 6m tall
    float wt = 0.4f;  // Wall thickness
    
    glm::vec3 c_wall(0.95f, 0.95f, 0.92f); // Off-white walls
    glm::vec3 c_divider(0.85f, 0.15f, 0.15f); // Brand color (Red or change later)
    glm::vec3 c_desk(0.1f, 0.1f, 0.12f); // Dark marble serving desk
    glm::vec3 c_marquee(0.8f, 0.8f, 0.85f);
    glm::vec3 c_metal(0.3f, 0.3f, 0.3f);
    
    // 1. Back Wall & Floor/Ceiling bounds
    pushBox11(v, {0, h/2, -hd + wt/2}, {hw, h/2, wt/2}, c_wall); // Back wall
    
    auto addCurvedPartition = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float height, float thick, glm::vec3 color) {
        int segments = 30;
        std::vector<glm::vec3> ptsRaw(segments + 1);
        std::vector<glm::vec3> nRaw(segments + 1);
        for(int i = 0; i <= segments; ++i) {
            float t = (float)i / segments;
            float tm1 = 1.0f - t;
            ptsRaw[i] = tm1*tm1*tm1*p0 + 3.0f*tm1*tm1*t*p1 + 3.0f*tm1*t*t*p2 + t*t*t*p3;
            glm::vec3 d = 3.0f*tm1*tm1*(p1-p0) + 6.0f*tm1*t*(p2-p1) + 3.0f*t*t*(p3-p2);
            // Normal in XZ plane (rotate tangent by 90 degrees)
            nRaw[i] = glm::normalize(glm::vec3(-d.z, 0.0f, d.x));
        }
        
        float dist = 0.0f; // Calculate approximate distance for U mapping
        for(int i = 0; i < segments; ++i) {
            glm::vec3 b0 = ptsRaw[i];
            glm::vec3 b1 = ptsRaw[i+1];
            glm::vec3 t0 = b0 + glm::vec3(0, height, 0);
            glm::vec3 t1 = b1 + glm::vec3(0, height, 0);
            
            // Outer wall
            glm::vec3 bo0 = b0 + nRaw[i] * thick;
            glm::vec3 bo1 = b1 + nRaw[i+1] * thick;
            glm::vec3 to0 = bo0 + glm::vec3(0, height, 0);
            glm::vec3 to1 = bo1 + glm::vec3(0, height, 0);
            
            float u0 = dist;
            dist += glm::length(b1 - b0) * 0.2f; // scale texture repeats
            float u1 = dist;
            
            glm::vec3 nAvg = glm::normalize(nRaw[i] + nRaw[i+1]);
            
            // Inner face 
            pushQuad11(v, b0, b1, t1, t0, nAvg, {u0, 0}, {u1, 0}, {u1, 1}, {u0, 1}, color);
            // Outer face
            pushQuad11(v, bo1, bo0, to0, to1, -nAvg, {u1, 0}, {u0, 0}, {u0, 1}, {u1, 1}, color);
            
            // End Caps
            if (i == 0) {
                glm::vec3 nBack = glm::normalize(glm::vec3(b0.x - bo0.x, 0, b0.z - bo0.z));
                pushQuad11(v, bo0, b0, t0, to0, nBack, {0,0}, {1,0}, {1,1}, {0,1}, color);
            }
            if (i == segments - 1) {
                glm::vec3 nFront = glm::normalize(glm::vec3(b1.x - bo1.x, 0, b1.z - bo1.z));
                pushQuad11(v, b1, bo1, to1, t1, nFront, {0,0}, {1,0}, {1,1}, {0,1}, color);
            }
            
            // Top Cap 
            pushQuad11(v, t0, t1, to1, to0, {0,1,0}, {0,0}, {1,0}, {1,1}, {0,1}, color);
        }
    };

    glm::vec3 c_acrylic(0.85f, 0.90f, 0.95f); // Frosted Acrylic Material

    // Left curved sweep
    glm::vec3 l_start(-hw + wt, 0, -hd);
    glm::vec3 l_c1(-hw - 2.5f, 0, -hd * 0.3f);
    glm::vec3 l_c2(-hw + 3.0f, 0, hd * 0.3f);
    glm::vec3 l_end(-hw + 1.0f, 0, hd);
    addCurvedPartition(l_start, l_c1, l_c2, l_end, h, wt, c_acrylic);

    // Right curved sweep (mirrored across X)
    glm::vec3 r_start(hw - wt, 0, -hd);
    glm::vec3 r_c1(hw + 2.5f, 0, -hd * 0.3f);
    glm::vec3 r_c2(hw - 3.0f, 0, hd * 0.3f);
    glm::vec3 r_end(hw - 1.0f, 0, hd);
    
    // For the right wall, we swap the order of the inner/outer faces through normal logic
    // by reversing the control points or just drawing it! Normal is generated in +X direction if drawn from back to front.
    // Left: d.z is positive. d.x is mostly 0 to start. (-d.z, 0, d.x) points -X initially. Wait.
    // If we draw from Z=-hd to Z=+hd, d.z > 0.
    // (-d.z, 0, d.x) implies (-val, 0, d.x). This points -X (leftwards).
    // For right side, d.z > 0. We want normal to point +X (rightwards).
    // We can just negate the normal inside addCurvedPartition by passing a flip flag or just let it be thick! It's a thick wall so it has both an inner and outer wall regardless! Wait, it has both faces because it's double-sided. So flipping isn't strictly necessary visually as long as normals point outwards from the thickness. But my inner/outer assignment `bo0 = b0 + nRaw * thick` depends on `nRaw`.
    // If `bo0` is always to the left (-X), then for the right wall, the thickness goes INWARD into the stall! We want it to go OUTWARD.
    // Actually letting the Right wall go inwards is mostly fine, we just adjust the start point to be `hw` instead of `hw-wt`.
    // Let's use `hw` so `bo` goes into `hw - wt`
    addCurvedPartition(glm::vec3(hw, 0, -hd), glm::vec3(hw + 2.5f, 0, -hd * 0.3f), glm::vec3(hw - 3.0f, 0, hd * 0.3f), glm::vec3(hw - 1.0f, 0, hd), h, -wt, c_acrylic);

    
    // 2. The Massive Serving Desk spanning the storefront!
    float deskZ = hd - 2.0f;  // Front edge
    pushBox11(v, {0, 0.5f, deskZ}, {hw - wt, 0.5f, 1.5f}, c_desk); // Base of desk (1.0m tall, 3m deep)
    pushBox11(v, {0, 1.02f, deskZ}, {hw - wt + 0.1f, 0.04f, 1.6f}, c_wall); // Countertop (white marble) overhanging slightly

    // 3. Overhead Marquee / Signage
    float marqueeH = 2.0f;
    float marqueeY = h - marqueeH/2.0f;
    pushBox11(v, {0, marqueeY, hd - 0.5f}, {hw - wt, marqueeH/2.0f, 0.5f}, c_marquee);
    
    // Marquee Perfect UV Canvas (Split into 3 panels to prevent stretching)
    auto addMarquee = [&](glm::vec3 center, float totalWidth, float height, float logoWidth, glm::vec3 baseColor) {
        float mzf = center.z + 0.01f;
        float my = center.y;
        float mh2 = height / 2.0f;
        
        float leftWidth = (totalWidth - logoWidth) / 2.0f;
        float rightWidth = leftWidth;
        
        // Left Panel (Solid color, degenerate UVs so it samples an edge pixel or blends)
        float lxMin = center.x - totalWidth/2.0f;
        float lxMax = center.x - totalWidth/2.0f + leftWidth;
        pushQuad11(v, 
            {lxMin, my - mh2, mzf}, {lxMax, my - mh2, mzf},
            {lxMax, my + mh2, mzf}, {lxMin, my + mh2, mzf},
            {0,0,1}, {0,0}, {0,0}, {0,0}, {0,0}, baseColor);
            
        // Center Panel (Perfect UVs [0,1])
        float cxMin = center.x - logoWidth/2.0f;
        float cxMax = center.x + logoWidth/2.0f;
        pushQuad11(v, 
            {cxMin, my - mh2, mzf}, {cxMax, my - mh2, mzf},
            {cxMax, my + mh2, mzf}, {cxMin, my + mh2, mzf},
            {0,0,1}, {0,0}, {1,0}, {1,1}, {0,1}, glm::vec3(1.0f)); // White so texture isn't tinted
            
        // Right Panel (Solid color, degenerate UVs)
        float rxMin = center.x + logoWidth/2.0f;
        float rxMax = center.x + totalWidth/2.0f;
        pushQuad11(v, 
            {rxMin, my - mh2, mzf}, {rxMax, my - mh2, mzf},
            {rxMax, my + mh2, mzf}, {rxMin, my + mh2, mzf},
            {0,0,1}, {0,0}, {0,0}, {0,0}, {0,0}, baseColor);
    };

    // The restaurant logo is roughly a typical wide rectangle, maybe 5x2 meters.
    addMarquee(glm::vec3(0, marqueeY, hd), (hw - wt) * 2.0f, marqueeH, 5.0f, c_marquee);

    // 4. Lambdas for Food Stuffs
    auto addDrinkCup = [&](glm::vec3 center, glm::vec3 color) {
        pushCylinder11(v, center + glm::vec3(0, 0.08f, 0), 0.05f, 0.16f, 12, color); // Cup
        pushCylinder11(v, center + glm::vec3(0, 0.165f, 0), 0.055f, 0.01f, 12, glm::vec3(0.9f)); // Lid
        pushCylinder11(v, center + glm::vec3(0, 0.20f, 0), 0.005f, 0.06f, 6, glm::vec3(0.1f)); // Straw
    };
    
    auto addPizzaBox = [&](glm::vec3 center, glm::vec3 color) {
        pushRotatedBox11(v, center + glm::vec3(0, 0.02f, 0), glm::vec3(0.3f, 0.02f, 0.3f), (rand()%100)/100.0f, color); // Flat box
    };
    
    auto addFoodTray = [&](glm::vec3 center, glm::vec3 color) {
        pushRotatedBox11(v, center + glm::vec3(0, 0.01f, 0), glm::vec3(0.35f, 0.01f, 0.25f), ((rand()%100) - 50)/200.0f, color);
    };

    // 5. Populate Serving Desk
    float deskTop = 1.061f; // Y surface + 0.001 air gap to prevent Z fighting
    
    glm::vec3 colPizza(0.85f, 0.65f, 0.45f);
    glm::vec3 colCupRed(0.8f, 0.1f, 0.1f);
    glm::vec3 colCupBlue(0.1f, 0.3f, 0.8f);
    glm::vec3 colTray(0.15f, 0.15f, 0.15f);

    for (int i = 0; i < 15; i++) {
        // Randomly place trays
        float rx = -hw + 2.0f + (hw*2.0f - 4.0f) * ((float)rand()/RAND_MAX);
        float rz = deskZ - 0.5f + ((float)rand()/RAND_MAX);
        
        addFoodTray(glm::vec3(rx, deskTop, rz), colTray);
        
        // Add food on tray
        if (rand() % 2 == 0) {
            addDrinkCup(glm::vec3(rx - 0.15f, deskTop + 0.02f, rz), colCupRed);
        } else {
            addDrinkCup(glm::vec3(rx + 0.15f, deskTop + 0.02f, rz), colCupBlue);
        }
        
        if (rand() % 2 == 0) {
            addPizzaBox(glm::vec3(rx, deskTop + 0.02f, rz), colPizza);
            if (rand() % 2 == 0) addPizzaBox(glm::vec3(rx, deskTop + 0.06f, rz), colPizza);
        }
    }
    
    // Stack some pizza boxes generically at the back counter
    for (int i=0; i<8; i++) {
        float px = -hw + 1.5f + i * 2.0f;
        int stacks = 3 + rand()%4;
        for (int s=0; s<stacks; s++) {
            addPizzaBox(glm::vec3(px, deskTop + s*0.04f, deskZ - 1.0f), colPizza);
        }
    }

    outCount = (int)(v.size() / 11);
    return uploadVAO11(v);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. FANCY SEATING VAO — circular table + 4 radial chairs baked into one VAO
// ─────────────────────────────────────────────────────────────────────────────

inline unsigned int createFancySeatingVAO(int& outCount) {
    const int discSegs = 48;
    const int cylSegs  = 16;
    const int arcSegs  = 8;  // chair backrest arc segments
    std::vector<float> v;

    // ── TABLETOP DISC — radial UV (0.5+0.5*cos, 0.5+0.5*sin) ──
    const float tableR = 0.85f;
    const float tableY = 0.82f;
    for (int i = 0; i < discSegs; ++i) {
        float a0 = glm::two_pi<float>()/discSegs*i;
        float a1 = glm::two_pi<float>()/discSegs*(i+1);
        float x0=cosf(a0)*tableR, z0=sinf(a0)*tableR;
        float x1=cosf(a1)*tableR, z1=sinf(a1)*tableR;
        // top face
        v.insert(v.end(),{0,tableY,0, 0,1,0, 0.5f,0.5f});
        v.insert(v.end(),{x0,tableY,z0, 0,1,0, 0.5f+0.5f*cosf(a0), 0.5f+0.5f*sinf(a0)});
        v.insert(v.end(),{x1,tableY,z1, 0,1,0, 0.5f+0.5f*cosf(a1), 0.5f+0.5f*sinf(a1)});
    }
    // tabletop edge ring (thin cylinder, no caps)
    appendCylinder(v, glm::vec3(0,tableY-0.025f,0), tableR, 0.025f, discSegs, false);

    // ── PEDESTAL STEM ──
    appendCylinder(v, glm::vec3(0, 0.41f, 0), 0.06f, 0.41f, cylSegs, false);
    // ── PEDESTAL BASE ──
    appendCylinder(v, glm::vec3(0, 0.05f, 0), 0.30f, 0.05f, cylSegs, true);

    // ── 4 CHAIRS at 0°, 90°, 180°, 270° ──
    const float chairDist = 1.15f;  // distance from table centre
    const float seatY     = 0.46f;
    const float seatR     = 0.28f;
    const float legLen    = 0.22f;
    const float legR      = 0.025f;

    for (int ci = 0; ci < 4; ++ci) {
        float angle = glm::half_pi<float>() * ci;
        float cx = cosf(angle) * chairDist;
        float cz = sinf(angle) * chairDist;

        // Seat disc (flat cylinder)
        appendCylinder(v, glm::vec3(cx, seatY, cz), seatR, 0.03f, cylSegs, true);

        // 4 legs per chair (front-left, front-right, back-left, back-right)
        const float lo = 0.18f; // leg radial offset from chair centre
        float legAngles[4] = { angle+0.6f, angle-0.6f, angle+2.5f, angle-2.5f };
        for (int li = 0; li < 4; ++li) {
            float lx = cx + cosf(legAngles[li])*lo;
            float lz = cz + sinf(legAngles[li])*lo;
            appendCylinder(v, glm::vec3(lx, seatY-legLen, lz), legR, legLen, 6, false);
        }

        // Curved backrest arc (8-segment arc bending backwards)
        // Arc sweeps from seatY+0.05 to seatY+0.55, leaning backward
        const float arcR = 0.30f; // arc radius from chair centre
        const float arcThick = 0.025f;
        // "backward" direction from table
        float bx = cosf(angle), bz = sinf(angle);
        for (int ai = 0; ai < arcSegs; ++ai) {
            float t0 = (float)ai / arcSegs;
            float t1 = (float)(ai+1) / arcSegs;
            // arc angle: from 0 to ~40° lean
            float arcAngle0 = t0 * glm::radians(40.0f);
            float arcAngle1 = t1 * glm::radians(40.0f);
            float h0 = seatY + 0.05f + t0 * 0.52f;
            float h1 = seatY + 0.05f + t1 * 0.52f;
            // position offset along "back" direction
            float lean0 = cosf(arcAngle0) * lo;
            float lean1 = cosf(arcAngle1) * lo;

            // Left rail of backrest
            glm::vec3 llb(cx + bx*lean0 - (-bz)*arcR, h0, cz + bz*lean0 - bx*arcR);
            glm::vec3 llt(cx + bx*lean1 - (-bz)*arcR, h1, cz + bz*lean1 - bx*arcR);
            glm::vec3 lrb(cx + bx*lean0 + (-bz)*arcR, h0, cz + bz*lean0 + bx*arcR);
            glm::vec3 lrt(cx + bx*lean1 + (-bz)*arcR, h1, cz + bz*lean1 + bx*arcR);
            glm::vec3 nB = glm::normalize(glm::cross(lrt-llb, llt-llb));
            float u0t = t0, u1t = t1;
            // backrest horizontal bar mesh
            pushQuad(v, llb,lrb,lrt,llt, nB, {0,u0t},{1,u0t},{1,u1t},{0,u1t});
        }
    }

    outCount = (int)(v.size() / 8);
    return uploadVAO(v);
}

// ─────────────────────────────────────────────────────────────────────────────
// DRAW HELPER SHORTCUTS
// ─────────────────────────────────────────────────────────────────────────────
static inline void setMat(Shader& s, const glm::mat4& m) { s.setMat4("model", m); }
static inline void setColor(Shader& s, glm::vec3 c, float amb=0.2f, float diff=0.65f,
                             float spec=0.3f, float shin=32.f) {
    s.setVec3("objectColor", c);
    s.setFloat("ambientStrength",  amb);
    s.setFloat("diffuseStrength",  diff);
    s.setFloat("specularStrength", spec);
    s.setFloat("shininess",        shin);
    s.setInt("textureType", 0);
    s.setFloat("objectAlpha", 1.0f);
}
static inline glm::mat4 TRS(glm::vec3 t, float ry, glm::vec3 sc) {
    glm::mat4 m(1.0f);
    m = glm::translate(m, t);
    m = glm::rotate(m, glm::radians(ry), glm::vec3(0,1,0));
    m = glm::scale(m, sc);
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// NPC VENDOR SYSTEM
// ─────────────────────────────────────────────────────────────────────────────

struct StallAnchor {
    glm::vec3 origin = glm::vec3(0.0f);
    float yawDeg = 0.0f;
};

struct FoodCourtVendor {
    NPCShared::NPC npc;
    int stallIndex = -1;
};

struct QueueSpot {
    glm::vec3 position = glm::vec3(0.0f);
    float facingYaw = 0.0f;
    int stallIndex = -1;
    int order = 0;
};

struct FoodCourtCustomer {
    enum class State {
        WALKING,
        GOING_TO_QUEUE,
        QUEUING
    };

    NPCShared::NPC npc;
    State state = State::WALKING;
    int queueSpotIndex = -1;
    float stateTimer = 0.0f;
};

static std::vector<StallAnchor> g_stallAnchors;
static std::vector<FoodCourtVendor> g_vendors;
static std::vector<QueueSpot> g_queueSpots;
static std::vector<int> g_queueOccupancy;
static std::vector<FoodCourtCustomer> g_customers;

inline float rand01() {
    return (float)(std::rand() % 10000) / 10000.0f;
}

inline float randRange(float lo, float hi) {
    return lo + (hi - lo) * rand01();
}

inline glm::vec3 rotateYLocal(const glm::vec3& local, float yawDeg) {
    float r = glm::radians(yawDeg);
    float c = std::cos(r);
    float s = std::sin(r);
    return glm::vec3(c * local.x + s * local.z, local.y, -s * local.x + c * local.z);
}

inline glm::vec3 anchorToWorld(const StallAnchor& anchor, const glm::vec3& local) {
    return anchor.origin + rotateYLocal(local, anchor.yawDeg);
}

inline void collectRestaurantStallAnchors(float floorY,
                                          float worldCenterX,
                                          float worldCenterZ,
                                          float roomHalfW,
                                          float roomHalfD,
                                          std::vector<StallAnchor>& outAnchors) {
    outAnchors.clear();

    float spacingX = 18.0f;
    float spacingZ = 20.0f;

    float wxL = worldCenterX - roomHalfW + 1.0f;
    for (float z = worldCenterZ - roomHalfD + 12.0f; z <= worldCenterZ + roomHalfD - 12.0f; z += spacingZ) {
        StallAnchor a;
        a.origin = glm::vec3(wxL, floorY, z);
        a.yawDeg = 90.0f;
        outAnchors.push_back(a);
    }

    float wxR = worldCenterX + roomHalfW - 1.0f;
    for (float z = worldCenterZ - roomHalfD + 12.0f; z <= worldCenterZ + roomHalfD - 12.0f; z += spacingZ) {
        StallAnchor a;
        a.origin = glm::vec3(wxR, floorY, z);
        a.yawDeg = -90.0f;
        outAnchors.push_back(a);
    }

    float backZ = worldCenterZ - roomHalfD + 1.2f;
    for (float x = worldCenterX - roomHalfW + 24.0f; x <= worldCenterX + roomHalfW - 24.0f; x += spacingX) {
        StallAnchor a;
        a.origin = glm::vec3(x, floorY, backZ);
        a.yawDeg = 0.0f;
        outAnchors.push_back(a);
    }
}

inline glm::vec3 randomFoodCourtWalkTarget(float floorY,
                                            float worldCenterX,
                                            float worldCenterZ,
                                            float roomHalfW,
                                            float roomHalfD) {
    float marginX = 10.0f;
    float marginZ = 10.0f;
    return glm::vec3(
        randRange(worldCenterX - roomHalfW + marginX, worldCenterX + roomHalfW - marginX),
        floorY,
        randRange(worldCenterZ - roomHalfD + marginZ, worldCenterZ + roomHalfD - marginZ)
    );
}

inline void initQueueSpotsFromStalls() {
    if (!g_queueSpots.empty()) return;

    const int spotsPerStall = 4;
    g_queueSpots.reserve(g_stallAnchors.size() * spotsPerStall);
    g_queueOccupancy.assign(g_stallAnchors.size() * spotsPerStall, -1);

    for (int s = 0; s < (int)g_stallAnchors.size(); ++s) {
        for (int o = 0; o < spotsPerStall; ++o) {
            QueueSpot spot;
            spot.position = anchorToWorld(g_stallAnchors[s], glm::vec3(0.0f, 0.0f, 7.2f + 1.15f * o));
            spot.facingYaw = g_stallAnchors[s].yawDeg + 180.0f;
            spot.stallIndex = s;
            spot.order = o;
            g_queueSpots.push_back(spot);
        }
    }
}

inline int reserveQueueSpotForStall(int stallIndex, int customerIndex) {
    const int spotsPerStall = 4;
    int base = stallIndex * spotsPerStall;
    int order = 0;
    while (order < spotsPerStall && g_queueOccupancy[base + order] != -1) {
        order++;
    }
    if (order >= spotsPerStall) return -1;

    int idx = base + order;
    g_queueOccupancy[idx] = customerIndex;
    return idx;
}

inline void compactQueueForStall(int stallIndex) {
    const int spotsPerStall = 4;
    int base = stallIndex * spotsPerStall;

    bool moved = true;
    while (moved) {
        moved = false;
        for (int o = 1; o < spotsPerStall; ++o) {
            int prev = base + (o - 1);
            int cur = base + o;
            if (g_queueOccupancy[prev] == -1 && g_queueOccupancy[cur] != -1) {
                int customerIdx = g_queueOccupancy[cur];
                g_queueOccupancy[cur] = -1;
                g_queueOccupancy[prev] = customerIdx;

                FoodCourtCustomer& customer = g_customers[customerIdx];
                customer.queueSpotIndex = prev;
                customer.state = FoodCourtCustomer::State::GOING_TO_QUEUE;
                customer.npc.targetPosition = g_queueSpots[prev].position;
                moved = true;
            }
        }
    }
}

inline bool assignCustomerToQueue(int customerIndex) {
    if (g_stallAnchors.empty() || g_queueSpots.empty()) return false;

    int stallCount = (int)g_stallAnchors.size();
    int start = std::rand() % stallCount;
    for (int t = 0; t < stallCount; ++t) {
        int stall = (start + t) % stallCount;
        int spotIdx = reserveQueueSpotForStall(stall, customerIndex);
        if (spotIdx != -1) {
            FoodCourtCustomer& customer = g_customers[customerIndex];
            customer.queueSpotIndex = spotIdx;
            customer.state = FoodCourtCustomer::State::GOING_TO_QUEUE;
            customer.npc.targetPosition = g_queueSpots[spotIdx].position;
            return true;
        }
    }
    return false;
}

inline void initFoodCourtVendors(float floorY,
                                  float worldCenterX,
                                  float worldCenterZ,
                                  float roomHalfW,
                                  float roomHalfD,
                                  int   cartCount = 3) {
    (void)cartCount;

    if (g_stallAnchors.empty()) {
        collectRestaurantStallAnchors(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, g_stallAnchors);
    }
    if (!g_vendors.empty()) return;

    glm::vec3 apronRed(0.80f, 0.12f, 0.12f);
    glm::vec3 apronGreen(0.12f, 0.55f, 0.28f);
    glm::vec3 apronBrown(0.45f, 0.25f, 0.10f);
    glm::vec3 apronBlue(0.15f, 0.30f, 0.70f);
    glm::vec3 aprons[] = { apronRed, apronGreen, apronBrown, apronBlue };

    for (int s = 0; s < (int)g_stallAnchors.size(); ++s) {
        const StallAnchor& a = g_stallAnchors[s];
        for (int k = 0; k < 2; ++k) {
            FoodCourtVendor fv;
            float localX = (k == 0) ? -1.8f : 1.8f;
            fv.npc.position = anchorToWorld(a, glm::vec3(localX, 0.0f, 2.35f));
            fv.npc.targetPosition = fv.npc.position;
            fv.npc.rotationY = a.yawDeg;
            fv.npc.speed = 0.0f;
            fv.npc.walkCycleTime = randRange(0.0f, 6.0f);
            fv.npc.gender = ((s + k) % 2 == 0) ? NPCShared::MALE : NPCShared::FEMALE;
            fv.npc.clothingColor = aprons[(s + k) % 4];
            fv.stallIndex = s;
            g_vendors.push_back(fv);
        }
    }
}

inline void initFoodCourtCustomers(float floorY,
                                   float worldCenterX,
                                   float worldCenterZ,
                                   float roomHalfW,
                                   float roomHalfD,
                                   int customerCount = 30) {
    if (!g_customers.empty()) return;

    g_customers.reserve(customerCount);
    for (int i = 0; i < customerCount; ++i) {
        FoodCourtCustomer c;
        c.npc.position = randomFoodCourtWalkTarget(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);
        c.npc.targetPosition = randomFoodCourtWalkTarget(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);
        glm::vec3 dir = c.npc.targetPosition - c.npc.position;
        c.npc.rotationY = glm::degrees(std::atan2(dir.x, dir.z));
        c.npc.speed = randRange(0.85f, 1.45f);
        c.npc.walkCycleTime = randRange(0.0f, 8.0f);
        c.npc.gender = (std::rand() % 2 == 0) ? NPCShared::MALE : NPCShared::FEMALE;
        c.npc.clothingColor = glm::vec3(randRange(0.20f, 0.95f), randRange(0.20f, 0.95f), randRange(0.20f, 0.95f));
        c.state = FoodCourtCustomer::State::WALKING;
        g_customers.push_back(c);
    }

    int seeded = std::min((int)g_customers.size(), std::max(8, (int)g_stallAnchors.size()));
    for (int i = 0; i < seeded; ++i) {
        assignCustomerToQueue(i);
    }
}

inline void moveCustomerTowards(FoodCourtCustomer& c, float deltaTime, float stopDistance = 0.12f) {
    glm::vec3 dir = c.npc.targetPosition - c.npc.position;
    dir.y = 0.0f;
    float dist = glm::length(dir);
    if (dist <= stopDistance) return;

    glm::vec3 n = dir / dist;
    float step = c.npc.speed * deltaTime;
    if (step > dist) step = dist;
    c.npc.position += n * step;
    c.npc.rotationY = glm::degrees(std::atan2(n.x, n.z));
}

inline void updateFoodCourtVendors(float deltaTime) {
    for (auto& fv : g_vendors)
        fv.npc.walkCycleTime += deltaTime * 4.5f;
}

inline void updateFoodCourtCustomers(float deltaTime,
                                     float floorY,
                                     float worldCenterX,
                                     float worldCenterZ,
                                     float roomHalfW,
                                     float roomHalfD) {
    for (int i = 0; i < (int)g_customers.size(); ++i) {
        FoodCourtCustomer& c = g_customers[i];

        if (c.state == FoodCourtCustomer::State::QUEUING) {
            c.npc.walkCycleTime += deltaTime * 1.5f;
        } else {
            c.npc.walkCycleTime += deltaTime * 4.0f;
        }

        if (c.state == FoodCourtCustomer::State::WALKING) {
            moveCustomerTowards(c, deltaTime);
            glm::vec3 toTarget = c.npc.targetPosition - c.npc.position;
            toTarget.y = 0.0f;
            if (glm::length(toTarget) <= 0.12f) {
                if ((std::rand() % 100) < 42 && assignCustomerToQueue(i)) {
                    // Customer enters a queue line.
                } else {
                    c.npc.targetPosition = randomFoodCourtWalkTarget(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);
                }
            }
            continue;
        }

        if (c.state == FoodCourtCustomer::State::GOING_TO_QUEUE) {
            if (c.queueSpotIndex < 0 || c.queueSpotIndex >= (int)g_queueSpots.size()) {
                c.state = FoodCourtCustomer::State::WALKING;
                c.npc.targetPosition = randomFoodCourtWalkTarget(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);
                continue;
            }

            c.npc.targetPosition = g_queueSpots[c.queueSpotIndex].position;
            moveCustomerTowards(c, deltaTime, 0.08f);
            glm::vec3 d = c.npc.targetPosition - c.npc.position;
            d.y = 0.0f;
            if (glm::length(d) <= 0.08f) {
                c.state = FoodCourtCustomer::State::QUEUING;
                c.stateTimer = randRange(4.0f, 9.0f);
                c.npc.rotationY = g_queueSpots[c.queueSpotIndex].facingYaw;
            }
            continue;
        }

        if (c.queueSpotIndex >= 0 && c.queueSpotIndex < (int)g_queueSpots.size()) {
            c.npc.targetPosition = g_queueSpots[c.queueSpotIndex].position;
            c.npc.rotationY = g_queueSpots[c.queueSpotIndex].facingYaw;
            c.npc.position.y = floorY;
        }

        c.stateTimer -= deltaTime;
        if (c.stateTimer <= 0.0f) {
            int spotIdx = c.queueSpotIndex;
            if (spotIdx >= 0 && spotIdx < (int)g_queueSpots.size()) {
                int stallIdx = g_queueSpots[spotIdx].stallIndex;
                g_queueOccupancy[spotIdx] = -1;
                compactQueueForStall(stallIdx);
            }

            c.queueSpotIndex = -1;
            c.state = FoodCourtCustomer::State::WALKING;
            c.npc.targetPosition = randomFoodCourtWalkTarget(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);
        }
    }
}

inline void drawFoodCourtVendors() {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0 || c.cylVAO == 0 || c.sphereVAO == 0) return;

    NPCShared::DrawParams params;
    params.selling = true;
    params.sitting = false;
    params.enableCameraFlash = false;

    for (auto& fv : g_vendors)
        NPCShared::drawNPC(*c.shader, c.cubeVAO, c.cylVAO, c.sphereVAO,
                           c.sphereCount, fv.npc, params, c.cylSegments);
}

inline void drawFoodCourtCustomers() {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0 || c.cylVAO == 0 || c.sphereVAO == 0) return;

    NPCShared::DrawParams params;
    params.selling = false;
    params.sitting = false;
    params.enableCameraFlash = false;

    for (auto& customer : g_customers)
        NPCShared::drawNPC(*c.shader, c.cubeVAO, c.cylVAO, c.sphereVAO,
                           c.sphereCount, customer.npc, params, c.cylSegments);
}

// ─────────────────────────────────────────────────────────────────────────────
// DRAW: LARGE RESTAURANTS (Lining the Left, Right, and Back Walls)
// ─────────────────────────────────────────────────────────────────────────────
inline void drawLargeRestaurants(float floorY, Shader& gouraudShader, 
                                 float worldCenterX, float worldCenterZ, 
                                 float roomHalfW, float roomHalfD) {
    static unsigned int resVAO = 0; 
    static int resCount = 0;
    if (resVAO == 0) resVAO = createLargeRestaurantVAO(resCount);

    gouraudShader.use();
    glBindVertexArray(resVAO);

    RenderContext& c = ctx();
    
    int restoIdx = 0;
    auto drawStall = [&](glm::mat4 m) {
        gouraudShader.setMat4("model", m);
        if (c.texRestaurantLogos[restoIdx % 4] != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, c.texRestaurantLogos[restoIdx % 4]);
            gouraudShader.setInt("texture1", 0);
            gouraudShader.setInt("useTexture", 1);
        } else {
            gouraudShader.setInt("useTexture", 0);
        }
        glDrawArrays(GL_TRIANGLES, 0, resCount);
        restoIdx++;
    };

    collectRestaurantStallAnchors(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, g_stallAnchors);
    for (const StallAnchor& anchor : g_stallAnchors) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), anchor.origin);
        if (std::abs(anchor.yawDeg) > 0.01f) {
            m = glm::rotate(m, glm::radians(anchor.yawDeg), glm::vec3(0, 1, 0));
        }
        drawStall(m);
    }
    
    // reset texture
    gouraudShader.setInt("useTexture", 0);

    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// DRAW: SEATING AREA (central grid of fancy table+chair groups)
// ─────────────────────────────────────────────────────────────────────────────

inline void drawSeatingArea(float floorY,
                             float worldCenterX,
                             float worldCenterZ,
                             float roomHalfW,
                             float roomHalfD,
                             int   rows      = 5,
                             int   cols      = 6,
                             float spacingX  = 4.8f,
                             float spacingZ  = 5.0f) {
    RenderContext& c = ctx();
    if (!c.shader) return;
    Shader& s = *c.shader;

    static unsigned int seatVAO   = 0;
    static int          seatCount = 0;
    if (seatVAO == 0) seatVAO = createFancySeatingVAO(seatCount);

    glm::vec3 tableTopColor(0.92f, 0.88f, 0.82f);  // light marble/cream
    glm::vec3 pedestalColor(0.30f, 0.30f, 0.32f);  // brushed metal
    glm::vec3 chairColor(0.55f, 0.35f, 0.20f);     // warm wood

    float totalW = (cols - 1) * spacingX;
    float totalD = (rows - 1) * spacingZ;
    float startX = -totalW * 0.5f;
    float startZ = -totalD * 0.5f;

    // Bind marble texture if available
    if (c.texTableTop != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, c.texTableTop);
        s.setInt("texture1", 0);
        s.setInt("useTexture", 1);
    } else {
        s.setInt("useTexture", 0);
    }

    for (int r = 0; r < rows; ++r) {
        for (int cl = 0; cl < cols; ++cl) {
            float bx = startX + cl * spacingX;
            float bz = startZ + r  * spacingZ;

            // Deterministic pseudo-random rotation per group
            float randRot = fmodf(sinf((float)r * 17.3f + (float)cl * 31.7f) * 180.0f, 360.0f);

            glm::mat4 groupM = glm::mat4(1.0f);
            groupM = glm::translate(groupM, glm::vec3(worldCenterX + bx, floorY, worldCenterZ + bz));
            groupM = glm::rotate(groupM, glm::radians(randRot), glm::vec3(0,1,0));
            
            // Enlarge the table and chairs!
            groupM = glm::scale(groupM, glm::vec3(1.6f));

            // Table top color (with texture if bound)
            setMat(s, groupM);
            setColor(s, tableTopColor, 0.22f, 0.65f, 0.80f, 64.0f);
            s.setMat4("model", groupM);

            glBindVertexArray(seatVAO);
            glDrawArrays(GL_TRIANGLES, 0, seatCount);
        }
    }

    s.setInt("useTexture", 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// MASTER DRAW FUNCTION
// ─────────────────────────────────────────────────────────────────────────────

inline void drawThirdFloor(float floorY,
                            float deltaTime,
                            Shader& gouraudShader,
                            float worldCenterX = 70.0f,
                            float worldCenterZ = 50.0f,
                            float roomHalfW = 60.0f,
                            float roomHalfD = 40.0f) {
    // Keep all generated content safely inside the floor even if caller passes lot half-extents.
    roomHalfW = std::max(18.0f, std::min(roomHalfW, 60.0f));
    roomHalfD = std::max(22.0f, std::min(roomHalfD, 40.0f));

    // 1. Large Restaurants lining Left, Right, & Back walls (uses 11-float shader)
    drawLargeRestaurants(floorY, gouraudShader, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);

    // Switch back to standard shader for 8-float items
    RenderContext& c = ctx();
    if (c.shader) c.shader->use();

    if (g_queueSpots.empty()) {
        initQueueSpotsFromStalls();
    }
    initFoodCourtVendors(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);
    initFoodCourtCustomers(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);

    updateFoodCourtVendors(deltaTime);
    updateFoodCourtCustomers(deltaTime, floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);

    // 2. Fancy circular seating in central area
    drawSeatingArea(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, 6, 8, 6.0f, 6.0f);

    // 3. Animated salesmen in each stall and walking/queued customers.
    drawFoodCourtVendors();
    drawFoodCourtCustomers();
}

} // namespace ThirdFloorDesign

#endif // THIRD_FLOOR_LAYOUT_H
