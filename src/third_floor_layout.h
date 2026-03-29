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
                             unsigned int texTableTop  = 0) {
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
// 1. POTTED PLANT VAO (pot only — tree drawn via FractalTree)
// ─────────────────────────────────────────────────────────────────────────────

inline unsigned int createPottedPlantVAO(int& outCount) {
    const int segs = 20;
    std::vector<float> v;
    v.reserve(segs * 18 * 8);

    // Pot body: short fat cylinder
    appendCylinder(v, glm::vec3(0,0,0), 0.28f, 0.22f, segs, true);
    // Rim ring: thin wide disc at top
    appendCylinder(v, glm::vec3(0, 0.23f, 0), 0.31f, 0.025f, segs, false);
    // Soil disc on top
    for (int i = 0; i < segs; ++i) {
        float a0 = glm::two_pi<float>()/segs*i, a1 = glm::two_pi<float>()/segs*(i+1);
        float x0=cosf(a0)*0.27f, z0=sinf(a0)*0.27f;
        float x1=cosf(a1)*0.27f, z1=sinf(a1)*0.27f;
        glm::vec3 c(0, 0.245f, 0);
        v.insert(v.end(),{c.x,c.y,c.z, 0,1,0, 0.5f,0.5f});
        v.insert(v.end(),{c.x+x1,c.y,c.z+z1, 0,1,0, 0.5f+0.5f*cosf(a1),0.5f+0.5f*sinf(a1)});
        v.insert(v.end(),{c.x+x0,c.y,c.z+z0, 0,1,0, 0.5f+0.5f*cosf(a0),0.5f+0.5f*sinf(a0)});
    }

    outCount = (int)(v.size() / 8);
    return uploadVAO(v);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. FOOD CART VAO — UV-ready front & top faces
// ─────────────────────────────────────────────────────────────────────────────

inline unsigned int createFoodCartVAO(int& outCount) {
    // Cart body: 1.6w x 1.0h x 0.8d, local origin at bottom-centre
    const float W=0.8f, H=0.5f, D=0.4f; // half-extents
    const float fullH = 1.0f; // full height (body=0.8, counter=0.2)
    std::vector<float> v;

    // --- BODY ---
    // Front face (z=+D) — UV [0,1] for menu texture
    pushQuad(v,
        {-W,0,  D},{W,0,  D},{W,fullH*0.8f,D},{-W,fullH*0.8f,D},
        {0,0,1}, {0,0},{1,0},{1,1},{0,1});
    // Back face
    pushQuad(v,
        {W,0,-D},{-W,0,-D},{-W,fullH*0.8f,-D},{W,fullH*0.8f,-D},
        {0,0,-1}, {0,0},{1,0},{1,1},{0,1});
    // Left face
    pushQuad(v,
        {-W,0,-D},{-W,0,D},{-W,fullH*0.8f,D},{-W,fullH*0.8f,-D},
        {-1,0,0}, {0,0},{1,0},{1,1},{0,1});
    // Right face
    pushQuad(v,
        {W,0,D},{W,0,-D},{W,fullH*0.8f,-D},{W,fullH*0.8f,D},
        {1,0,0}, {0,0},{1,0},{1,1},{0,1});
    // Bottom
    pushQuad(v,
        {-W,0,-D},{W,0,-D},{W,0,D},{-W,0,D},
        {0,-1,0}, {0,0},{1,0},{1,1},{0,1});

    // --- COUNTER TOP (slightly wider, with UV [0,1] for logo/mat) ---
    float cY = fullH*0.8f;
    float cW = W+0.06f, cD = D+0.06f;
    pushQuad(v,
        {-cW,cY,-cD},{cW,cY,-cD},{cW,cY,cD},{-cW,cY,cD},
        {0,1,0}, {0,0},{1,0},{1,1},{0,1});
    // counter front lip
    pushQuad(v,
        {-cW,cY,cD},{cW,cY,cD},{cW,cY+0.08f,cD},{-cW,cY+0.08f,cD},
        {0,0,1}, {0,0},{1,0},{1,1},{0,1});
    // counter sides
    pushQuad(v,
        {-cW,cY,-cD},{-cW,cY+0.08f,-cD},{-cW,cY+0.08f,cD},{-cW,cY,cD},
        {-1,0,0}, {0,0},{1,0},{1,1},{0,1});
    pushQuad(v,
        {cW,cY,cD},{cW,cY+0.08f,cD},{cW,cY+0.08f,-cD},{cW,cY,-cD},
        {1,0,0}, {0,0},{1,0},{1,1},{0,1});

    // --- KICK PANEL (bottom strip detail) ---
    pushQuad(v,
        {-W,0,D},{W,0,D},{W,0.12f,D},{-W,0.12f,D},
        {0,0,1}, {0,0},{1,0},{1,1},{0,1});

    outCount = (int)(v.size() / 8);
    return uploadVAO(v);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. COFFEE SHOP COUNTER VAO
// ─────────────────────────────────────────────────────────────────────────────

inline unsigned int createCoffeeCounterVAO(int& outCount) {
    // Longer L-shaped counter: 4.0w x 1.1h x 0.9d
    const float W=2.0f, H=0.55f, D=0.45f;
    std::vector<float> v;

    // Front face — perfect UV for logo
    pushQuad(v, {-W,0,D},{W,0,D},{W,H*2.0f,D},{-W,H*2.0f,D},
        {0,0,1}, {0,0},{1,0},{1,1},{0,1});
    // Back face
    pushQuad(v, {W,0,-D},{-W,0,-D},{-W,H*2.0f,-D},{W,H*2.0f,-D},
        {0,0,-1}, {0,0},{1,0},{1,1},{0,1});
    // Left
    pushQuad(v, {-W,0,-D},{-W,0,D},{-W,H*2.0f,D},{-W,H*2.0f,-D},
        {-1,0,0}, {0,0},{1,0},{1,1},{0,1});
    // Right
    pushQuad(v, {W,0,D},{W,0,-D},{W,H*2.0f,-D},{W,H*2.0f,D},
        {1,0,0}, {0,0},{1,0},{1,1},{0,1});
    // Counter top — UV for texture
    float cY = H*2.0f;
    pushQuad(v, {-W,cY,-D},{W,cY,-D},{W,cY,D},{-W,cY,D},
        {0,1,0}, {0,0},{1,0},{1,1},{0,1});

    // Back shelf (half-depth, raised)
    float sY = cY + 0.1f;
    float sW = W*0.9f, sD = D*0.5f;
    // shelf top
    pushQuad(v, {-sW,sY,-D},{sW,sY,-D},{sW,sY,-D+sD},{-sW,sY,-D+sD},
        {0,1,0}, {0,0},{1,0},{1,1},{0,1});
    // shelf front lip
    pushQuad(v, {-sW,sY,-D+sD},{sW,sY,-D+sD},{sW,sY+0.05f,-D+sD},{-sW,sY+0.05f,-D+sD},
        {0,0,1}, {0,0},{1,0},{1,1},{0,1});

    // Overhead signboard (wide flat board)
    float signY = cY + 1.4f;
    pushQuad(v, {-W,signY-0.05f,-D-0.02f},{W,signY-0.05f,-D-0.02f},
                {W,signY+0.22f,-D-0.02f},{-W,signY+0.22f,-D-0.02f},
        {0,0,-1}, {0,0},{1,0},{1,1},{0,1});
    // front of signboard — UV for coffee logo
    pushQuad(v, {-W,signY-0.05f,D+0.01f},{W,signY-0.05f,D+0.01f},
                {W,signY+0.22f,D+0.01f},{-W,signY+0.22f,D+0.01f},
        {0,0,1}, {0,0},{1,0},{1,1},{0,1});

    outCount = (int)(v.size() / 8);
    return uploadVAO(v);
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
// DRAW: POTTED PLANT WALLS (Left = -X wall, Right = +X wall)
// ─────────────────────────────────────────────────────────────────────────────

inline void drawPlantWalls(float floorY,
                            float worldCenterX,
                            float worldCenterZ,
                            float roomHalfW,
                            float roomHalfD,
                            float spacing = 4.5f) {
    RenderContext& c = ctx();
    if (!c.shader || c.cylVAO == 0) return;
    Shader& s = *c.shader;

    // Static pot VAO (created once)
    static unsigned int potVAO   = 0;
    static int          potCount = 0;
    if (potVAO == 0) potVAO = createPottedPlantVAO(potCount);

    glm::vec3 potColor(0.42f, 0.24f, 0.12f);  // terracotta
    glm::vec3 soilColor(0.25f, 0.16f, 0.08f);

    FractalTree::Style plantStyle = FractalTree::makeAtriumStyle();
    // Override for compact indoor plants
    plantStyle.branchFactor  = 3;
    plantStyle.tiltDegrees   = 38.0f;
    plantStyle.lengthRatio   = 0.68f;
    plantStyle.leafScale     = 0.14f;
    plantStyle.leafClusterCount = 5;

    auto drawPottedPlant = [&](glm::vec3 pos) {
        // Draw pot
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = glm::scale(m, glm::vec3(1.0f));
        setMat(s, m);
        setColor(s, potColor, 0.18f, 0.60f, 0.35f, 24.0f);
        s.setInt("useTexture", 0);
        glBindVertexArray(potVAO);
        glDrawArrays(GL_TRIANGLES, 0, potCount);

        // Draw fractal tree from soil surface
        glm::vec3 treeRoot = pos + glm::vec3(0, 0.26f, 0);
        FractalTree::drawFractalTree(s, c.cubeVAO, c.cylVAO, c.cylSegments,
                                     c.sphereVAO, c.sphereCount,
                                     treeRoot, 3, 0.45f, 0.045f,
                                     plantStyle, pos.x * 0.07f + pos.z * 0.13f);
    };

    // Left wall  (x = -roomHalfW + wallOffset)
    float wxL = worldCenterX - roomHalfW + 1.0f;
    float wxR = worldCenterX + roomHalfW - 1.0f;
    for (float z = worldCenterZ - roomHalfD + 2.0f; z <= worldCenterZ + roomHalfD - 2.0f; z += spacing) {
        drawPottedPlant(glm::vec3(wxL, floorY, z));
        drawPottedPlant(glm::vec3(wxR, floorY, z));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DRAW: FOOD OUTLETS (Front wall = +Z, Back wall = -Z)
// 3 food carts on +Z wall, 1 coffee shop on -Z wall
// ─────────────────────────────────────────────────────────────────────────────

inline void drawFoodOutlets(float floorY,
                             float worldCenterX,
                             float worldCenterZ,
                             float roomHalfW,
                             float roomHalfD) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& s = *c.shader;

    static unsigned int cartVAO    = 0; static int cartCount    = 0;
    static unsigned int coffeeVAO  = 0; static int coffeeCount  = 0;
    if (cartVAO   == 0) cartVAO   = createFoodCartVAO(cartCount);
    if (coffeeVAO == 0) coffeeVAO = createCoffeeCounterVAO(coffeeCount);

    glm::vec3 cartBodyColor(0.90f, 0.90f, 0.92f);
    glm::vec3 cartAccent(0.95f, 0.38f, 0.12f);
    glm::vec3 coffeeColor(0.28f, 0.16f, 0.08f);
    glm::vec3 coffeeAccent(0.12f, 0.08f, 0.05f);
    glm::vec3 signYellow(0.98f, 0.85f, 0.10f);
    glm::vec3 awningRed(0.85f, 0.15f, 0.15f);
    glm::vec3 awningWhite(0.97f, 0.97f, 0.97f);
    glm::vec3 stripeBlue(0.10f, 0.30f, 0.75f);

    float frontZ  = worldCenterZ + roomHalfD - 0.8f;
    float backZ   = worldCenterZ - roomHalfD + 1.2f;

    // ── 3 FOOD CARTS on front wall ──
    float cartSpacing = (roomHalfW * 1.6f) / 4.0f;
    float cartXStart  = -roomHalfW * 0.8f + cartSpacing;
    glm::vec3 cartColors[3] = {
        glm::vec3(0.95f,0.20f,0.20f),  // Burger — red
        glm::vec3(0.15f,0.55f,0.85f),  // Pizza  — blue
        glm::vec3(0.20f,0.70f,0.30f)   // Tacos  — green
    };

    for (int i = 0; i < 3; ++i) {
        float cx = cartXStart + i * cartSpacing;
        glm::vec3 origin(worldCenterX + cx, floorY, frontZ);
        glm::mat4 mCart = glm::translate(glm::mat4(1.0f), origin);
        mCart = glm::rotate(mCart, glm::radians(180.0f), glm::vec3(0,1,0));

        // Bind cart menu texture if available
        if (c.texCartMenu != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, c.texCartMenu);
            s.setInt("texture1", 0);
            s.setInt("useTexture", 1);
        } else {
            s.setInt("useTexture", 0);
        }

        setMat(s, mCart);
        setColor(s, cartColors[i], 0.20f, 0.65f, 0.40f, 28.0f);
        glBindVertexArray(cartVAO);
        glDrawArrays(GL_TRIANGLES, 0, cartCount);

        s.setInt("useTexture", 0);

        // Canopy poles (two cylinders)
        glm::mat4 poleL = TRS(origin + glm::vec3(-0.65f, 1.0f, 0.02f), 0.0f, glm::vec3(0.04f, 0.7f, 0.04f));
        setMat(s, poleL); setColor(s, glm::vec3(0.3f), 0.2f, 0.5f, 0.8f, 64.0f);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);

        glm::mat4 poleR = TRS(origin + glm::vec3( 0.65f, 1.0f, 0.02f), 0.0f, glm::vec3(0.04f, 0.7f, 0.04f));
        setMat(s, poleR);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);

        // Canopy roof (flat cube, striped accent color)
        drawCubeRotated(s, c.cubeVAO, origin + glm::vec3(0, 1.72f, -0.1f),
                        glm::vec3(1.5f, 0.06f, 0.65f), glm::vec3(0,0,0),
                        (i%2==0) ? awningRed : signYellow, 0, 0.3f,0.6f,0.2f,8.f);

        // Signboard above counter
        drawCubeRotated(s, c.cubeVAO, origin + glm::vec3(0, 2.0f, -0.05f),
                        glm::vec3(1.1f, 0.28f, 0.05f), glm::vec3(0,0,0),
                        cartColors[i], 0, 0.3f,0.5f,0.3f,16.f);

        // Small sphere decorative knob on top of each pole
        glm::mat4 knobL = TRS(origin+glm::vec3(-0.65f,1.70f,0.02f),0,glm::vec3(0.07f));
        setMat(s,knobL); setColor(s,glm::vec3(0.9f,0.8f,0.1f),0.3f,0.5f,0.9f,80.f);
        glBindVertexArray(c.sphereVAO);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
        glm::mat4 knobR = TRS(origin+glm::vec3(0.65f,1.70f,0.02f),0,glm::vec3(0.07f));
        setMat(s,knobR);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);

        // Small potted condiments on counter top
        glm::mat4 condim = TRS(origin+glm::vec3(0.3f, 1.10f,-0.1f),0,glm::vec3(0.09f,0.16f,0.09f));
        setMat(s,condim); setColor(s,glm::vec3(0.8f,0.1f,0.1f),0.2f,0.6f,0.5f,32.f);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments*12);
    }

    // ── COFFEE SHOP on back wall ──
    glm::vec3 coffeeOrigin(worldCenterX, floorY, backZ);
    glm::mat4 mCoffee = glm::translate(glm::mat4(1.0f), coffeeOrigin);

    if (c.texCoffeeLogo != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, c.texCoffeeLogo);
        s.setInt("texture1", 0);
        s.setInt("useTexture", 1);
    } else {
        s.setInt("useTexture", 0);
    }
    setMat(s, mCoffee);
    setColor(s, coffeeColor, 0.18f, 0.60f, 0.50f, 40.0f);
    glBindVertexArray(coffeeVAO);
    glDrawArrays(GL_TRIANGLES, 0, coffeeCount);
    s.setInt("useTexture", 0);

    // Espresso machine (cylinders stacked)
    glm::vec3 machinePos = coffeeOrigin + glm::vec3(-0.8f, 1.2f, -0.2f);
    glm::mat4 machBody = TRS(machinePos, 0, glm::vec3(0.22f,0.26f,0.20f));
    setMat(s,machBody); setColor(s,glm::vec3(0.72f,0.72f,0.75f),0.2f,0.5f,0.9f,80.f);
    glBindVertexArray(c.cylVAO);
    glDrawArrays(GL_TRIANGLES, 0, c.cylSegments*12);

    // Coffee cups stacked on counter
    for (int ci = 0; ci < 3; ++ci) {
        glm::mat4 cup = TRS(coffeeOrigin + glm::vec3(0.5f + ci*0.22f, 1.18f, -0.15f),
                            0, glm::vec3(0.08f,0.10f,0.08f));
        setMat(s,cup); setColor(s,glm::vec3(0.97f,0.97f,0.97f),0.25f,0.55f,0.4f,24.f);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments*12);
    }

    // Overhead pendant lights above coffee shop
    for (int li = -1; li <= 1; ++li) {
        glm::vec3 lp = coffeeOrigin + glm::vec3(li * 1.2f, 2.8f, 0.0f);
        glm::mat4 lm = TRS(lp, 0, glm::vec3(0.12f, 0.18f, 0.12f));
        setMat(s,lm); setColor(s,glm::vec3(1.0f,0.92f,0.70f),0.9f,0.3f,0.1f,4.f);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments*12);
        // cord
        glm::mat4 cord = TRS(coffeeOrigin+glm::vec3(li*1.2f,3.35f,0),0,glm::vec3(0.018f,0.55f,0.018f));
        setMat(s,cord); setColor(s,glm::vec3(0.1f),0.1f,0.3f,0.1f,4.f);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments*12);
    }
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

inline void initFoodCourt(float floorY, float worldCenterX, float worldCenterZ, float roomHalfW, float roomHalfD) {
    initFoodCourtVendors(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, 3);
}

inline void drawThirdFloor(float floorY,
                            float deltaTime,
                            float worldCenterX = 70.0f,
                            float worldCenterZ = 50.0f,
                            float roomHalfW = 25.0f,
                            float roomHalfD = 35.0f) {
    // Keep all generated content safely inside the floor even if caller passes lot half-extents.
    roomHalfW = std::max(18.0f, std::min(roomHalfW, 30.0f));
    roomHalfD = std::max(22.0f, std::min(roomHalfD, 40.0f));

    // Initialise vendors on first call
    if (g_vendors.empty())
        initFoodCourt(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);

    // 1. Fractal potted plants lining Left & Right walls
    drawPlantWalls(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, 4.5f);

    // 2. Food carts & coffee shop on Front & Back walls
    drawFoodOutlets(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD);

    // 3. Vendor NPCs behind counters
    updateFoodCourtVendors(deltaTime);
    drawFoodCourtVendors();

    // 4. Fancy circular seating in central area
    drawSeatingArea(floorY, worldCenterX, worldCenterZ, roomHalfW, roomHalfD, 5, 6, 4.8f, 5.0f);
}

} // namespace ThirdFloorDesign

#endif // THIRD_FLOOR_LAYOUT_H
