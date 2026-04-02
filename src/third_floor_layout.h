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
    unsigned int texRestaurantLogo = 0; // massive restaurant brand sign
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
                             unsigned int texRestaurantLogo = 0) {
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
    c.texRestaurantLogo = texRestaurantLogo;
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
    pushBox11(v, {-hw + wt/2, h/2, 0}, {wt/2, h/2, hd}, c_divider); // Left Divider Wall
    pushBox11(v, {hw - wt/2, h/2, 0}, {wt/2, h/2, hd}, c_divider); // Right Divider Wall
    
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

struct FoodCourtVendor {
    NPCShared::NPC npc;
    bool           isCoffeeBarista = false;
};

static std::vector<FoodCourtVendor> g_vendors;

inline void initFoodCourtVendors(float floorY,
                                  float worldCenterX,
                                  float worldCenterZ,
                                  float roomHalfW,
                                  float roomHalfD,
                                  int   cartCount = 3) {
    if (!g_vendors.empty()) return;

    glm::vec3 apronRed(0.80f, 0.12f, 0.12f);
    glm::vec3 apronGreen(0.12f, 0.55f, 0.28f);
    glm::vec3 apronBrown(0.45f, 0.25f, 0.10f);
    glm::vec3 apronBlue(0.15f, 0.30f, 0.70f);

    glm::vec3 aprons[] = { apronRed, apronGreen, apronBrown, apronBlue };

    // Front-wall cart vendors
    float spacingX = (roomHalfW * 1.6f) / (cartCount + 1);
    float startX   = -roomHalfW * 0.8f + spacingX;
    float frontWallZ = worldCenterZ + roomHalfD;
    for (int i = 0; i < cartCount; ++i) {
        FoodCourtVendor fv;
        fv.npc.position       = glm::vec3(worldCenterX + startX + i*spacingX, floorY, frontWallZ - 1.4f);
        fv.npc.targetPosition = fv.npc.position;
        fv.npc.rotationY      = 180.0f;  // face toward room
        fv.npc.speed          = 0.0f;
        fv.npc.walkCycleTime  = (float)i * 1.1f;
        fv.npc.gender         = (i % 2 == 0) ? NPCShared::MALE : NPCShared::FEMALE;
        fv.npc.clothingColor  = aprons[i % 4];
        g_vendors.push_back(fv);
    }

    // Back-wall coffee barista
    {
        float backWallZ = worldCenterZ - roomHalfD;
        FoodCourtVendor fv;
        fv.npc.position       = glm::vec3(worldCenterX, floorY, backWallZ + 1.5f);
        fv.npc.targetPosition = fv.npc.position;
        fv.npc.rotationY      = 0.0f;
        fv.npc.speed          = 0.0f;
        fv.npc.walkCycleTime  = 0.0f;
        fv.npc.gender         = NPCShared::FEMALE;
        fv.npc.clothingColor  = apronBlue;
        fv.isCoffeeBarista    = true;
        g_vendors.push_back(fv);
    }
}

inline void updateFoodCourtVendors(float deltaTime) {
    for (auto& fv : g_vendors)
        fv.npc.walkCycleTime += deltaTime * 4.5f;
}

inline void drawFoodCourtVendors() {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0 || c.cylVAO == 0 || c.sphereVAO == 0) return;

    NPCShared::DrawParams params;
    params.selling           = true;
    params.sitting           = false;
    params.enableCameraFlash = false;

    for (auto& fv : g_vendors)
        NPCShared::drawNPC(*c.shader, c.cubeVAO, c.cylVAO, c.sphereVAO,
                           c.sphereCount, fv.npc, params, c.cylSegments);
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
    if (c.texRestaurantLogo != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, c.texRestaurantLogo);
        gouraudShader.setInt("texture1", 0);
        gouraudShader.setInt("useTexture", 1);
    } else {
        gouraudShader.setInt("useTexture", 0);
    }

    float spacingX = 18.0f; // restaurants are 16m wide, so 18m spacing
    float spacingZ = 20.0f; 

    // Left Wall (-X) facing right
    float wxL = worldCenterX - roomHalfW + 1.0f;
    for (float z = worldCenterZ - roomHalfD + 12.0f; z <= worldCenterZ + roomHalfD - 12.0f; z += spacingZ) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(wxL, floorY, z));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0,1,0));
        gouraudShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, resCount);
    }
    
    // Right Wall (+X) facing left
    float wxR = worldCenterX + roomHalfW - 1.0f;
    for (float z = worldCenterZ - roomHalfD + 12.0f; z <= worldCenterZ + roomHalfD - 12.0f; z += spacingZ) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(wxR, floorY, z));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(0,1,0));
        gouraudShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, resCount);
    }
    
    // Back Wall (-Z) facing forward
    float backZ = worldCenterZ - roomHalfD + 1.2f;
    for (float x = worldCenterX - roomHalfW + 24.0f; x <= worldCenterX + roomHalfW - 24.0f; x += spacingX) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(x, floorY, backZ));
        gouraudShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, resCount);
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

    // 2. Fancy circular seating in central area
    drawSeatingArea(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, 6, 8, 6.0f, 6.0f);
}

} // namespace ThirdFloorDesign

#endif // THIRD_FLOOR_LAYOUT_H
