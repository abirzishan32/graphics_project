#ifndef FIRST_FLOOR_LAYOUT_H
#define FIRST_FLOOR_LAYOUT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <vector>
#include <cmath>
#include <algorithm>

#include "shader.h"

void drawCube(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
              glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);
void drawCubeAlpha(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale,
                   glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess, float alpha);
void drawCubeRotated(Shader& shader, unsigned int VAO, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation,
                     glm::vec3 color, int texType, float ambient, float diffuse, float specular, float shininess);

namespace FirstFloorDesign {

struct RenderContext {
    Shader* shader    = nullptr;
    unsigned int cubeVAO   = 0;
    unsigned int cylVAO    = 0;
    int          cylSegments = 16;
    unsigned int coneVAO   = 0;
    int          coneCount = 0;
    unsigned int sphereVAO = 0;
    int          sphereCount = 0;
};

struct AtriumMesh {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    int indexCount   = 0;
    bool built       = false;
};

inline RenderContext& ctx() {
    static RenderContext context;
    return context;
}

inline AtriumMesh& atriumMesh() {
    static AtriumMesh mesh;
    return mesh;
}

inline glm::vec3 rotateY(const glm::vec3& local, float yawDeg) {
    float r = glm::radians(yawDeg);
    float c = std::cos(r);
    float s = std::sin(r);
    return glm::vec3(c * local.x + s * local.z, local.y, -s * local.x + c * local.z);
}

inline glm::vec3 localToWorld(const glm::vec3& origin, const glm::vec3& local, float yawDeg) {
    return origin + rotateY(local, yawDeg);
}

inline void setRenderContext(Shader& shader,
                             unsigned int cubeVAO,
                             unsigned int cylVAO,
                             int          cylSegments,
                             unsigned int coneVAO,
                             int          coneCount,
                             unsigned int sphereVAO,
                             int          sphereCount) {
    RenderContext& c = ctx();
    c.shader      = &shader;
    c.cubeVAO     = cubeVAO;
    c.cylVAO      = cylVAO;
    c.cylSegments = cylSegments;
    c.coneVAO     = coneVAO;
    c.coneCount   = coneCount;
    c.sphereVAO   = sphereVAO;
    c.sphereCount = sphereCount;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: draw an alpha-blended cube with full rotation
// ─────────────────────────────────────────────────────────────────────────────
inline void drawRotatedAlphaCube(Shader&      shader,
                                 unsigned int cubeVAO,
                                 glm::vec3    position,
                                 glm::vec3    scale,
                                 glm::vec3    rotationDeg,
                                 glm::vec3    color,
                                 int          texType,
                                 float ambient, float diffuse, float specular, float shininess,
                                 float alpha) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationDeg.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationDeg.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationDeg.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);

    shader.setMat4("model",            model);
    shader.setVec3("objectColor",      color);
    shader.setInt("textureType",       texType);
    shader.setFloat("ambientStrength", ambient);
    shader.setFloat("diffuseStrength", diffuse);
    shader.setFloat("specularStrength",specular);
    shader.setFloat("shininess",       shininess);
    shader.setFloat("objectAlpha",     alpha);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    shader.setFloat("objectAlpha", 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  FLOOR GEOMETRY: Square with a true circular hole
//
//  Strategy:
//  1. Ring strip — for each of `segments` angles, project a ray onto the
//     square boundary (outer) and the circle boundary (inner).  Connect
//     consecutive pairs into quads (2 triangles).  This produces zero
//     triangles inside the circle radius.
//  2. Corner fans — the ring-strip outer points lie ON the square edges but
//     they never land exactly on the 4 corners.  Four triangular "notches"
//     remain uncovered at each corner.  We close them by emitting a fan of
//     triangles from the corner vertex to each successive outer ring point
//     whose angle falls in that corner's sector.
// ─────────────────────────────────────────────────────────────────────────────
inline void buildAtriumFloorMesh(float halfExtent, float innerRadius, int segments) {
    AtriumMesh& mesh = atriumMesh();
    if (mesh.built) return;

    const float TWO_PI = glm::two_pi<float>();
    auto pushVert = [&](std::vector<float>& v, float x, float z, float nx, float ny, float nz, float u, float w2) {
        v.insert(v.end(), { x, 0.0f, z, nx, ny, nz, u, w2 });
    };

    // ── 1) Build the ring-strip vertices ─────────────────────────────────────
    std::vector<float>        verts;
    std::vector<unsigned int> idx;
    verts.reserve((segments + 4) * 2 * 8);
    idx.reserve(segments * 6 + 128);

    struct Pt { float x, z; };
    std::vector<Pt> outerPts(segments), innerPts(segments);

    for (int i = 0; i < segments; ++i) {
        float a  = TWO_PI * (float)i / (float)segments;
        float dx = std::cos(a);
        float dz = std::sin(a);

        float t  = halfExtent / std::max(std::abs(dx), std::abs(dz));
        outerPts[i] = { dx * t,           dz * t };
        innerPts[i] = { dx * innerRadius,  dz * innerRadius };
    }

    // Push all ring strip vertices (outer, inner alternating)
    auto uvOf = [&](float x, float z) -> std::pair<float,float> {
        return { x / (2.0f * halfExtent) + 0.5f,
                 z / (2.0f * halfExtent) + 0.5f };
    };

    for (int i = 0; i < segments; ++i) {
        auto [uo, vo] = uvOf(outerPts[i].x, outerPts[i].z);
        auto [ui, vi] = uvOf(innerPts[i].x, innerPts[i].z);
        pushVert(verts, outerPts[i].x, outerPts[i].z, 0, 1, 0, uo, vo); // even = outer
        pushVert(verts, innerPts[i].x, innerPts[i].z, 0, 1, 0, ui, vi); // odd  = inner
    }

    // Ring-strip indices (no triangles inside circle)
    for (int i = 0; i < segments; ++i) {
        int n  = (i + 1) % segments;
        unsigned int o0 = (unsigned int)(2 * i);
        unsigned int i0 = o0 + 1;
        unsigned int o1 = (unsigned int)(2 * n);
        unsigned int i1 = o1 + 1;

        idx.push_back(o0); idx.push_back(i0); idx.push_back(o1);
        idx.push_back(i0); idx.push_back(i1); idx.push_back(o1);
    }

    // ── 2) Corner fans ───────────────────────────────────────────────────────
    // Corners in CCW order: (+,+), (-,+), (-,-), (+,-)
    // For each corner, its ideal angle (ray direction) bisects the 90° sector.
    // We collect which ring-strip outer points fall in [cornerAngle-45°, cornerAngle+45°]
    // and add a fan from that corner vertex to those points.
    struct Corner { float cx, cz, bisect; };
    Corner corners[4] = {
        {  halfExtent,  halfExtent,  glm::quarter_pi<float>()       },  // NE (+,+): bisect = 45°
        { -halfExtent,  halfExtent,  3.0f * glm::quarter_pi<float>()},  // NW (-,+): bisect = 135°
        { -halfExtent, -halfExtent, -3.0f * glm::quarter_pi<float>()},  // SW (-,-): bisect = -135° (225°)
        {  halfExtent, -halfExtent, -glm::quarter_pi<float>()       },  // SE (+,-): bisect = -45° (315°)
    };

    for (int ci = 0; ci < 4; ++ci) {
        // Push the corner vertex
        auto [uc, vc] = uvOf(corners[ci].cx, corners[ci].cz);
        unsigned int cornerIdx = (unsigned int)(verts.size() / 8);
        pushVert(verts, corners[ci].cx, corners[ci].cz, 0, 1, 0, uc, vc);

        // Collect outer ring indices in this corner's 90° sector
        // Sector: angles within π/4 of the bisector
        std::vector<int> inSector;
        for (int i = 0; i < segments; ++i) {
            float a = TWO_PI * (float)i / (float)segments;
            // Normalise angle difference into (-π, π]
            float diff = a - corners[ci].bisect;
            while (diff >  glm::pi<float>()) diff -= TWO_PI;
            while (diff < -glm::pi<float>()) diff += TWO_PI;
            if (std::abs(diff) <= glm::quarter_pi<float>() + 1e-4f)
                inSector.push_back(i);
        }

        // Sort by angle to keep CCW winding
        std::sort(inSector.begin(), inSector.end(), [&](int a, int b) {
            float aa = TWO_PI * (float)a / (float)segments;
            float ab = TWO_PI * (float)b / (float)segments;
            return aa < ab;
        });

        // Emit a fan of triangles: corner → outer[k] → outer[k+1]
        for (int k = 0; k + 1 < (int)inSector.size(); ++k) {
            unsigned int vA = (unsigned int)(2 * inSector[k]);     // outer ring point
            unsigned int vB = (unsigned int)(2 * inSector[k + 1]); // next outer ring point
            idx.push_back(cornerIdx);
            idx.push_back(vA);
            idx.push_back(vB);
        }
    }

    // ── 3) Upload to GPU ─────────────────────────────────────────────────────
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(idx.size() * sizeof(unsigned int)),
                 idx.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    mesh.indexCount = (int)idx.size();
    mesh.built      = true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawMannequin  (unchanged from original)
// ─────────────────────────────────────────────────────────────────────────────
inline void drawMannequin(glm::vec3 pos) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;

    glm::vec3 standColor(0.18f, 0.18f, 0.20f);
    glm::vec3 bodyColor (0.78f, 0.76f, 0.72f);
    glm::vec3 dressColor(0.16f, 0.16f, 0.18f);

    // Head (sphere)
    if (c.sphereVAO != 0 && c.sphereCount > 0) {
        glm::mat4 head = glm::mat4(1.0f);
        head = glm::translate(head, pos + glm::vec3(0.0f, 1.95f, 0.0f));
        head = glm::scale(head, glm::vec3(0.16f));
        shader.setMat4("model",            head);
        shader.setVec3("objectColor",      bodyColor);
        shader.setInt("textureType",       0);
        shader.setFloat("ambientStrength", 0.12f);
        shader.setFloat("diffuseStrength", 0.70f);
        shader.setFloat("specularStrength",0.25f);
        shader.setFloat("shininess",       22.0f);
        shader.setFloat("objectAlpha",     1.0f);
        glBindVertexArray(c.sphereVAO);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
    }

    // Torso + stand (cylinders)
    if (c.cylVAO != 0) {
        glm::mat4 torso = glm::mat4(1.0f);
        torso = glm::translate(torso, pos + glm::vec3(0.0f, 1.25f, 0.0f));
        torso = glm::scale(torso, glm::vec3(0.20f, 0.55f, 0.14f));
        shader.setMat4("model",            torso);
        shader.setVec3("objectColor",      bodyColor);
        shader.setInt("textureType",       0);
        shader.setFloat("ambientStrength", 0.12f);
        shader.setFloat("diffuseStrength", 0.72f);
        shader.setFloat("specularStrength",0.22f);
        shader.setFloat("shininess",       20.0f);
        shader.setFloat("objectAlpha",     1.0f);
        glBindVertexArray(c.cylVAO);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);

        glm::mat4 stand = glm::mat4(1.0f);
        stand = glm::translate(stand, pos + glm::vec3(0.0f, 0.45f, 0.0f));
        stand = glm::scale(stand, glm::vec3(0.08f, 0.90f, 0.08f));
        shader.setMat4("model",            stand);
        shader.setVec3("objectColor",      standColor);
        shader.setInt("textureType",       3);
        shader.setFloat("ambientStrength", 0.10f);
        shader.setFloat("diffuseStrength", 0.55f);
        shader.setFloat("specularStrength",0.85f);
        shader.setFloat("shininess",       140.0f);
        shader.setFloat("objectAlpha",     1.0f);
        glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
    }

    // Stylized dress (cone)
    if (c.coneVAO != 0 && c.coneCount > 0) {
        glm::mat4 dress = glm::mat4(1.0f);
        dress = glm::translate(dress, pos + glm::vec3(0.0f, 0.70f, 0.0f));
        dress = glm::scale(dress, glm::vec3(0.42f, 0.75f, 0.42f));
        shader.setMat4("model",            dress);
        shader.setVec3("objectColor",      dressColor);
        shader.setInt("textureType",       3);
        shader.setFloat("ambientStrength", 0.10f);
        shader.setFloat("diffuseStrength", 0.68f);
        shader.setFloat("specularStrength",0.35f);
        shader.setFloat("shininess",       38.0f);
        shader.setFloat("objectAlpha",     1.0f);
        glBindVertexArray(c.coneVAO);
        glDrawElements(GL_TRIANGLES, c.coneCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PROCEDURAL INTERIOR: Clothing Rack
//
//  A T-shaped chrome hanging rail:
//   • Two thin vertical posts (cylinders)
//   • One horizontal bar at the top connecting them
//   • Two flat disc bases at ground level
// ─────────────────────────────────────────────────────────────────────────────
inline void drawClothingRack(glm::vec3 position) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;

    const glm::vec3 chrome(0.72f, 0.74f, 0.78f);
    const float rackHalfLen = 0.85f;   // half-length of the horizontal bar
    const float postHeight  = 1.65f;
    const float postRadius  = 0.03f;
    const float barRadius   = 0.025f;
    const float baseRadius  = 0.11f;
    const float baseHeight  = 0.04f;

    auto setChromeUniforms = [&]() {
        shader.setVec3("objectColor",       chrome);
        shader.setInt("textureType",        3);
        shader.setFloat("ambientStrength",  0.08f);
        shader.setFloat("diffuseStrength",  0.45f);
        shader.setFloat("specularStrength", 0.95f);
        shader.setFloat("shininess",        180.0f);
        shader.setFloat("objectAlpha",      1.0f);
    };

    // Left post
    if (c.cylVAO) {
        glm::vec3 postOffsets[2] = {
            glm::vec3(-rackHalfLen, postHeight * 0.5f, 0.0f),
            glm::vec3( rackHalfLen, postHeight * 0.5f, 0.0f)
        };
        glBindVertexArray(c.cylVAO);
        for (int p = 0; p < 2; ++p) {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, position + postOffsets[p]);
            m = glm::scale(m, glm::vec3(postRadius, postHeight, postRadius));
            setChromeUniforms();
            shader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
        }

        // Horizontal bar
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, position + glm::vec3(0.0f, postHeight, 0.0f));
            m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // rotate so height axis → X
            m = glm::scale(m, glm::vec3(barRadius, rackHalfLen * 2.0f, barRadius));
            setChromeUniforms();
            shader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
        }

        // Base discs
        for (int p = 0; p < 2; ++p) {
            glm::vec3 baseOffset = (p == 0)
                ? glm::vec3(-rackHalfLen, baseHeight * 0.5f, 0.0f)
                : glm::vec3( rackHalfLen, baseHeight * 0.5f, 0.0f);
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, position + baseOffset);
            m = glm::scale(m, glm::vec3(baseRadius, baseHeight, baseRadius));
            setChromeUniforms();
            shader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
        }
    }
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PROCEDURAL INTERIOR: Clothes on Rack
//
//  For each item: a thin metallic hanger cube + a garment (flattened cylinder
//  or cone) in one of several warm/cool fashion colours.
// ─────────────────────────────────────────────────────────────────────────────
inline void drawClothesOnRack(glm::vec3 startPos, int count) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;

    // Small hand-picked palette of garment colours
    const glm::vec3 palette[] = {
        glm::vec3(0.80f, 0.22f, 0.22f),   // crimson
        glm::vec3(0.20f, 0.35f, 0.65f),   // navy
        glm::vec3(0.15f, 0.50f, 0.35f),   // forest green
        glm::vec3(0.55f, 0.20f, 0.55f),   // plum
        glm::vec3(0.85f, 0.68f, 0.20f),   // gold
        glm::vec3(0.25f, 0.25f, 0.25f),   // charcoal
        glm::vec3(0.90f, 0.88f, 0.84f),   // off-white
        glm::vec3(0.75f, 0.42f, 0.20f),   // caramel
    };
    const int paletteSize = (int)(sizeof(palette) / sizeof(palette[0]));

    const float spacing    = 0.22f;  // X-spacing between items
    const float hangerY    = startPos.y + 1.63f;  // height of horizontal bar — RELATIVE to floor
    const float garmentLen = 0.38f;  // length of garment below hanger

    // Start slightly to the left so items are centred around startPos
    float startX = startPos.x - spacing * (count - 1) * 0.5f;

    glBindVertexArray(c.cubeVAO);
    for (int i = 0; i < count; ++i) {
        float itemX = startX + spacing * i;
        glm::vec3 hangerPos(itemX, hangerY, startPos.z);  // hangerY already absolute

        // — Hanger: very thin V-shaped approximated as two thin rotated cuboids
        glm::vec3 hangerColor(0.68f, 0.70f, 0.74f); // silver
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, hangerPos);
            m = glm::scale(m, glm::vec3(0.18f, 0.022f, 0.012f));
            shader.setMat4("model",            m);
            shader.setVec3("objectColor",      hangerColor);
            shader.setInt("textureType",       3);
            shader.setFloat("ambientStrength", 0.10f);
            shader.setFloat("diffuseStrength", 0.42f);
            shader.setFloat("specularStrength",0.92f);
            shader.setFloat("shininess",       160.0f);
            shader.setFloat("objectAlpha",     1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // — Garment: a flattened cuboid hanging below hanger, coloured from palette
        glm::vec3 gColor = palette[i % paletteSize];
        {
            glm::vec3 gPos = hangerPos + glm::vec3(0.0f, -garmentLen * 0.5f - 0.02f, 0.0f);
            // Slight tilt so clothes look draped naturally
            float tiltDeg = (i % 2 == 0) ? 2.5f : -2.5f;
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, gPos);
            m = glm::rotate(m, glm::radians(tiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
            m = glm::scale(m, glm::vec3(0.17f, garmentLen, 0.04f));
            shader.setMat4("model",            m);
            shader.setVec3("objectColor",      gColor);
            shader.setInt("textureType",       0);
            shader.setFloat("ambientStrength", 0.14f);
            shader.setFloat("diffuseStrength", 0.70f);
            shader.setFloat("specularStrength",0.12f);
            shader.setFloat("shininess",       18.0f);
            shader.setFloat("objectAlpha",     1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PROCEDURAL INTERIOR: Display Table
//
//  A low dark-wood table with 4 legs and a stack of 3–5 "folded clothes"
//  on top.  Each folded layer is a very thin cube, slightly rotated on Y
//  so the pile looks naturally uneven.
// ─────────────────────────────────────────────────────────────────────────────
inline void drawDisplayTable(glm::vec3 position) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;

    const glm::vec3 woodDark (0.30f, 0.20f, 0.12f);
    const glm::vec3 woodMid  (0.40f, 0.28f, 0.18f);

    const float tableW  = 1.10f;
    const float tableD  = 0.65f;
    const float tableH  = 0.48f;   // top surface height
    const float topThk  = 0.07f;
    const float legThk  = 0.08f;
    const float legH    = tableH - topThk;

    glBindVertexArray(c.cubeVAO);

    auto setWood = [&](glm::vec3 col) {
        shader.setVec3("objectColor",       col);
        shader.setInt("textureType",        0);
        shader.setFloat("ambientStrength",  0.14f);
        shader.setFloat("diffuseStrength",  0.62f);
        shader.setFloat("specularStrength", 0.18f);
        shader.setFloat("shininess",        22.0f);
        shader.setFloat("objectAlpha",      1.0f);
    };

    // Table top
    {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position + glm::vec3(0.0f, tableH - topThk * 0.5f, 0.0f));
        m = glm::scale(m, glm::vec3(tableW, topThk, tableD));
        setWood(woodMid);
        shader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // 4 legs
    float lx[2] = { -tableW * 0.5f + legThk * 0.5f,  tableW * 0.5f - legThk * 0.5f };
    float lz[2] = { -tableD * 0.5f + legThk * 0.5f,  tableD * 0.5f - legThk * 0.5f };
    for (int xi = 0; xi < 2; ++xi) {
        for (int zi = 0; zi < 2; ++zi) {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, position + glm::vec3(lx[xi], legH * 0.5f, lz[zi]));
            m = glm::scale(m, glm::vec3(legThk, legH, legThk));
            setWood(woodDark);
            shader.setMat4("model", m);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    // Stacked folded clothes on top (3–5 layers)
    const int   numLayers = 4;
    const float layerThk  = 0.032f;
    const float layerW    = 0.34f;
    const float layerD    = 0.28f;
    // Palette for the fold stack
    const glm::vec3 foldColors[] = {
        glm::vec3(0.86f, 0.82f, 0.78f),   // cream
        glm::vec3(0.22f, 0.42f, 0.60f),   // denim
        glm::vec3(0.65f, 0.22f, 0.22f),   // rose
        glm::vec3(0.32f, 0.28f, 0.22f),   // taupe
        glm::vec3(0.88f, 0.72f, 0.30f),   // amber
    };

    float stackBaseY = tableH;
    for (int li = 0; li < numLayers; ++li) {
        float yc     = stackBaseY + layerThk * (li + 0.5f);
        float rotDeg = 7.3f * (float)li + (float)(li % 3) * 4.1f;  // slight per-layer rotation

        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position + glm::vec3(0.0f, yc, 0.0f));
        m = glm::rotate(m, glm::radians(rotDeg), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::scale(m, glm::vec3(layerW, layerThk, layerD));

        glm::vec3 fc = foldColors[li % 5];
        shader.setMat4("model",            m);
        shader.setVec3("objectColor",      fc);
        shader.setInt("textureType",       0);
        shader.setFloat("ambientStrength", 0.15f);
        shader.setFloat("diffuseStrength", 0.65f);
        shader.setFloat("specularStrength",0.08f);
        shader.setFloat("shininess",       12.0f);
        shader.setFloat("objectAlpha",     1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawFashionOutlet  (upgraded with 3D pushed-open door and new interior funcs)
// ─────────────────────────────────────────────────────────────────────────────
inline void drawFashionOutlet(glm::vec3 position, glm::vec2 size, float rotation) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;

    const float w      = size.x;
    const float d      = size.y;
    const float h      = 5.6f;
    const float wallT  = 0.16f;
    const float frameT = 0.20f;

    glm::vec3 woodColor      (0.45f, 0.31f, 0.20f);
    glm::vec3 interiorWallColor(0.88f, 0.86f, 0.82f);
    glm::vec3 floorColor     (0.60f, 0.58f, 0.54f);
    glm::vec3 glassColor     (0.78f, 0.88f, 0.96f);
    glm::vec3 frameMetalColor(0.32f, 0.24f, 0.16f); // dark wood / metal frame

    // Helper: draw a cube in store-local space
    auto drawLocalCube = [&](glm::vec3 localPos, glm::vec3 localScale, glm::vec3 color, int texType,
                             float ambient, float diffuse, float specular, float shininess) {
        glm::vec3 worldPos = localToWorld(position, localPos, rotation);
        drawCubeRotated(shader, c.cubeVAO, worldPos, localScale,
                        glm::vec3(0.0f, rotation, 0.0f), color, texType,
                        ambient, diffuse, specular, shininess);
    };

    // ── Shell ──────────────────────────────────────────────────────────────
    drawLocalCube(glm::vec3(0.0f, 0.08f, 0.0f),            glm::vec3(w, 0.16f, d),   floorColor,          0, 0.12f, 0.60f, 0.18f, 18.0f);
    drawLocalCube(glm::vec3(0.0f, h - 0.08f, 0.0f),        glm::vec3(w, 0.16f, d),   interiorWallColor,   0, 0.10f, 0.58f, 0.20f, 20.0f);
    drawLocalCube(glm::vec3(0.0f, h * 0.5f, d * 0.5f - wallT * 0.5f), glm::vec3(w, h, wallT), interiorWallColor, 0, 0.10f, 0.56f, 0.20f, 20.0f);
    drawLocalCube(glm::vec3(-w * 0.5f + wallT * 0.5f, h * 0.5f, 0.0f), glm::vec3(wallT, h, d), interiorWallColor, 0, 0.10f, 0.56f, 0.20f, 20.0f);
    drawLocalCube(glm::vec3( w * 0.5f - wallT * 0.5f, h * 0.5f, 0.0f), glm::vec3(wallT, h, d), interiorWallColor, 0, 0.10f, 0.56f, 0.20f, 20.0f);

    // ── Thick wooden/metallic outer storefront frame ───────────────────────
    float frontZ = -d * 0.5f + wallT * 0.45f;
    // Top beam
    drawLocalCube(glm::vec3(0.0f, h - frameT * 0.5f, frontZ), glm::vec3(w, frameT, wallT), woodColor, 0, 0.16f, 0.66f, 0.30f, 28.0f);
    // Left column
    drawLocalCube(glm::vec3(-w * 0.5f + frameT * 0.5f, h * 0.5f, frontZ), glm::vec3(frameT, h, wallT), woodColor, 0, 0.16f, 0.66f, 0.30f, 28.0f);
    // Right column
    drawLocalCube(glm::vec3( w * 0.5f - frameT * 0.5f, h * 0.5f, frontZ), glm::vec3(frameT, h, wallT), woodColor, 0, 0.16f, 0.66f, 0.30f, 28.0f);
    // Base threshold (sill)
    drawLocalCube(glm::vec3(0.0f, frameT * 0.25f, frontZ), glm::vec3(w, frameT * 0.5f, wallT), woodColor, 0, 0.16f, 0.62f, 0.25f, 24.0f);

    // ── Glass + 3D Pushed-Open Door ────────────────────────────────────────
    float openingW  = w - 2.0f * frameT;
    float openingH  = h - 2.0f * frameT;

    // Widths:  left panel 38%  |  door 24%  |  right panel 38%
    float sidePanelW = openingW * 0.38f;
    float doorW      = openingW * 0.24f;
    float panelH     = openingH;
    float panelMidY  = frameT + panelH * 0.5f;

    // Local X centres of the three zones (relative to store centre)
    float leftPanelX  = -openingW * 0.5f + sidePanelW * 0.5f;
    float doorCentreX =  0.0f;                                   // door centred
    float rightPanelX =  openingW * 0.5f - sidePanelW * 0.5f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Left static glass panel
    {
        glm::vec3 wPos = localToWorld(position, glm::vec3(leftPanelX, panelMidY, frontZ), rotation);
        drawCubeAlpha(shader, c.cubeVAO, wPos,
                      glm::vec3(sidePanelW - 0.04f, panelH, wallT * 0.5f),
                      glassColor, 4, 0.02f, 0.10f, 1.0f, 256.0f, 0.28f);
    }

    // Right static glass panel
    {
        glm::vec3 wPos = localToWorld(position, glm::vec3(rightPanelX, panelMidY, frontZ), rotation);
        drawCubeAlpha(shader, c.cubeVAO, wPos,
                      glm::vec3(sidePanelW - 0.04f, panelH, wallT * 0.5f),
                      glassColor, 4, 0.02f, 0.10f, 1.0f, 256.0f, 0.28f);
    }

    // ── 3D Pushed-Open Door ──────────────────────────────────────────────
    // The door is hinged at its LEFT vertical edge (local X = doorCentreX - doorW/2).
    // Strategy:
    //   1. Translate to hinge position in local space
    //   2. Rotate around Y by -35° (so the door opens inward/outward)
    //   3. Translate the panel so its left edge is at the hinge pivot
    //
    const float doorOpenAngle = -35.0f; // negative = opens toward viewer
    glm::vec3 hingeLocal(doorCentreX - doorW * 0.5f, panelMidY, frontZ);
    glm::vec3 hingeWorld = localToWorld(position, hingeLocal, rotation);

    // Compute world-space Y rotation that combines store rotation + door swing
    float doorWorldYaw = rotation + doorOpenAngle; // door's final world yaw

    // Door glass fill (semi-transparent)
    {
        // After rotation, the door panel's local centre is at (+doorW/2, 0, 0)
        // relative to the hinge. We replicate this with a custom matrix.
        glm::mat4 doorMatrix = glm::mat4(1.0f);
        doorMatrix = glm::translate(doorMatrix, hingeWorld);
        // Combine store yaw + door swing around the hinge's Y axis
        doorMatrix = glm::rotate(doorMatrix,
                                 glm::radians(rotation + doorOpenAngle),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
        doorMatrix = glm::translate(doorMatrix, glm::vec3(doorW * 0.5f, 0.0f, 0.0f));
        doorMatrix = glm::scale(doorMatrix, glm::vec3(doorW - 0.06f, panelH - 0.06f, wallT * 0.50f));

        shader.setMat4("model",             doorMatrix);
        shader.setVec3("objectColor",       glassColor);
        shader.setInt("textureType",        4);
        shader.setFloat("ambientStrength",  0.02f);
        shader.setFloat("diffuseStrength",  0.10f);
        shader.setFloat("specularStrength", 1.0f);
        shader.setFloat("shininess",        256.0f);
        shader.setFloat("objectAlpha",      0.35f);
        glBindVertexArray(c.cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        shader.setFloat("objectAlpha", 1.0f);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Door frame (thin metallic strips around door perimeter, fully opaque)
    // We build these in the same hinge-pivoted coordinate system
    auto drawDoorFramePiece = [&](glm::vec3 localOffset, glm::vec3 pieceScale) {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, hingeWorld);
        m = glm::rotate(m, glm::radians(rotation + doorOpenAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::translate(m, glm::vec3(doorW * 0.5f, 0.0f, 0.0f) + localOffset);
        m = glm::scale(m, pieceScale);
        shader.setMat4("model",             m);
        shader.setVec3("objectColor",       frameMetalColor);
        shader.setInt("textureType",        0);
        shader.setFloat("ambientStrength",  0.16f);
        shader.setFloat("diffuseStrength",  0.62f);
        shader.setFloat("specularStrength", 0.35f);
        shader.setFloat("shininess",        32.0f);
        shader.setFloat("objectAlpha",      1.0f);
        glBindVertexArray(c.cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    };

    const float df = 0.04f; // door frame thickness
    const float dz = wallT * 0.60f;
    // Top rail
    drawDoorFramePiece(glm::vec3(0.0f,  panelH * 0.5f, 0.0f), glm::vec3(doorW, df, dz));
    // Bottom rail
    drawDoorFramePiece(glm::vec3(0.0f, -panelH * 0.5f, 0.0f), glm::vec3(doorW, df, dz));
    // Left stile
    drawDoorFramePiece(glm::vec3(-doorW * 0.5f + df * 0.5f, 0.0f, 0.0f), glm::vec3(df, panelH, dz));
    // Right stile
    drawDoorFramePiece(glm::vec3( doorW * 0.5f - df * 0.5f, 0.0f, 0.0f), glm::vec3(df, panelH, dz));

    // ── Procedural interior furniture ────────────────────────────────────────
    // Clothing rack #1 (left-centre)
    {
        glm::vec3 rack1World = localToWorld(position, glm::vec3(-w * 0.22f, 0.0f, d * 0.08f), rotation);
        drawClothingRack(rack1World);
        drawClothesOnRack(rack1World + glm::vec3(0.0f, 0.0f, 0.0f), 7);
    }
    // Clothing rack #2 (right-centre)
    {
        glm::vec3 rack2World = localToWorld(position, glm::vec3( w * 0.22f, 0.0f, d * 0.08f), rotation);
        drawClothingRack(rack2World);
        drawClothesOnRack(rack2World, 7);
    }

    // Display tables (near front window)
    {
        glm::vec3 table1World = localToWorld(position, glm::vec3(-w * 0.16f, 0.0f, -d * 0.25f), rotation);
        drawDisplayTable(table1World);
    }
    {
        glm::vec3 table2World = localToWorld(position, glm::vec3( w * 0.16f, 0.0f, -d * 0.25f), rotation);
        drawDisplayTable(table2World);
    }

    // Mannequins (near window display)
    drawMannequin(localToWorld(position, glm::vec3(-w * 0.18f, 0.0f, -d * 0.18f), rotation));
    drawMannequin(localToWorld(position, glm::vec3( w * 0.18f, 0.0f, -d * 0.22f), rotation));
}

// ─────────────────────────────────────────────────────────────────────────────
//  drawAtriumAndBalustrade  (unchanged logic, uses upgraded hollow mesh)
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
//  GRAND ATRIUM CENTERPIECE
//
//  A premium shopping-mall atrium focal point featuring:
//    • Raised marble floor platform with cross and corner inlays
//    • 3-tiered grand fountain: chrome column, two cascading basins,
//      semi-transparent water pool, gold sphere finial
//    • 8 ornate marble columns arranged in a circle, with gold accent rings
//      and capital discs
//    • 4 decorative planters between the columns (foliage cubes)
//    • 4 curved wooden benches facing the fountain
//    • Circular glass balustrade with gold handrail at the outer boundary
// ─────────────────────────────────────────────────────────────────────────────
inline void drawAtriumCenterpiece(float floorY) {
    RenderContext& c = ctx();
    if (!c.shader || c.cubeVAO == 0) return;
    Shader& shader = *c.shader;

    const glm::vec3 ctr(70.0f, floorY, 50.0f);

    // ── Palette ──────────────────────────────────────────────────────────────
    const glm::vec3 marbleWhite (0.93f, 0.91f, 0.88f);
    const glm::vec3 marbleMid   (0.80f, 0.78f, 0.74f);
    const glm::vec3 marbleDark  (0.60f, 0.58f, 0.55f);
    const glm::vec3 gold        (0.88f, 0.72f, 0.20f);
    const glm::vec3 chrome      (0.72f, 0.74f, 0.78f);
    const glm::vec3 waterBlue   (0.48f, 0.70f, 0.88f);
    const glm::vec3 leafGreen   (0.20f, 0.50f, 0.18f);
    const glm::vec3 leafLight   (0.30f, 0.62f, 0.24f);
    const glm::vec3 benchWood   (0.52f, 0.36f, 0.20f);
    const glm::vec3 benchMetal  (0.50f, 0.50f, 0.54f);
    const glm::vec3 glassColor  (0.78f, 0.88f, 0.97f);
    const float     inlay       = 0.003f; // thickness of decorative inlay strips

    // ── 1) RAISED MARBLE FLOOR PLATFORM ─────────────────────────────────────
    // Main slab
    drawCube(shader, c.cubeVAO,
             ctr + glm::vec3(0.0f, 0.045f, 0.0f),
             glm::vec3(22.0f, 0.09f, 22.0f),
             marbleWhite, 0, 0.18f, 0.72f, 0.50f, 80.0f);
    // Cross-shaped darker inlay: horizontal bar
    drawCube(shader, c.cubeVAO,
             ctr + glm::vec3(0.0f, 0.09f + inlay, 0.0f),
             glm::vec3(22.0f, inlay, 2.0f),
             marbleDark, 0, 0.14f, 0.65f, 0.40f, 60.0f);
    // Cross-shaped darker inlay: vertical bar
    drawCube(shader, c.cubeVAO,
             ctr + glm::vec3(0.0f, 0.09f + inlay * 2, 0.0f),
             glm::vec3(2.0f, inlay, 22.0f),
             marbleDark, 0, 0.14f, 0.65f, 0.40f, 60.0f);
    // 4 corner accent squares
    for (int xi = -1; xi <= 1; xi += 2) {
        for (int zi = -1; zi <= 1; zi += 2) {
            drawCube(shader, c.cubeVAO,
                     ctr + glm::vec3(xi * 8.0f, 0.09f + inlay * 3, zi * 8.0f),
                     glm::vec3(3.2f, inlay, 3.2f),
                     marbleMid, 0, 0.16f, 0.70f, 0.45f, 70.0f);
        }
    }
    // Thin gold border strip around platform edge
    for (int side = 0; side < 4; ++side) {
        bool horiz    = (side < 2);
        float offX    =  horiz ? 0.0f : (side == 2 ? -11.0f :  11.0f);
        float offZ    = !horiz ? 0.0f : (side == 0 ? -11.0f :  11.0f);
        float scaleX  =  horiz ? 22.2f : 0.12f;
        float scaleZ  = !horiz ? 22.2f : 0.12f;
        drawCube(shader, c.cubeVAO,
                 ctr + glm::vec3(offX, 0.09f + inlay * 4, offZ),
                 glm::vec3(scaleX, inlay * 2, scaleZ),
                 gold, 0, 0.20f, 0.70f, 0.90f, 160.0f);
    }

    // ── 2) GRAND TIERED FOUNTAIN ─────────────────────────────────────────────
    if (c.cylVAO != 0) {
        glBindVertexArray(c.cylVAO);

        auto setCylMat = [&](glm::vec3 pos, glm::vec3 scale, glm::vec3 col,
                             float amb, float diff, float spec, float shi) {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, pos);
            m = glm::scale(m, scale);
            shader.setMat4("model",             m);
            shader.setVec3("objectColor",       col);
            shader.setInt("textureType",        0);
            shader.setFloat("ambientStrength",  amb);
            shader.setFloat("diffuseStrength",  diff);
            shader.setFloat("specularStrength", spec);
            shader.setFloat("shininess",        shi);
            shader.setFloat("objectAlpha",      1.0f);
            glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
        };

        // Outer basin rim (wide, low)
        setCylMat(ctr + glm::vec3(0.0f, 0.30f, 0.0f), glm::vec3(4.2f, 0.32f, 4.2f),
                  marbleWhite, 0.18f, 0.72f, 0.60f, 90.0f);
        // Inner basin floor
        setCylMat(ctr + glm::vec3(0.0f, 0.18f, 0.0f), glm::vec3(3.9f, 0.06f, 3.9f),
                  marbleMid, 0.16f, 0.68f, 0.50f, 70.0f);

        // Central chrome column
        setCylMat(ctr + glm::vec3(0.0f, 1.70f, 0.0f), glm::vec3(0.16f, 3.10f, 0.16f),
                  chrome, 0.10f, 0.48f, 0.96f, 220.0f);

        // Tier 1 bowl (mid height)
        setCylMat(ctr + glm::vec3(0.0f, 2.05f, 0.0f), glm::vec3(2.20f, 0.22f, 2.20f),
                  marbleWhite, 0.18f, 0.72f, 0.60f, 90.0f);
        // Tier 1 column stub
        setCylMat(ctr + glm::vec3(0.0f, 2.50f, 0.0f), glm::vec3(0.14f, 0.70f, 0.14f),
                  chrome, 0.10f, 0.48f, 0.96f, 220.0f);

        // Tier 2 bowl (high)
        setCylMat(ctr + glm::vec3(0.0f, 2.95f, 0.0f), glm::vec3(1.30f, 0.18f, 1.30f),
                  marbleMid, 0.16f, 0.70f, 0.55f, 80.0f);
        // Tier 2 column stub
        setCylMat(ctr + glm::vec3(0.0f, 3.22f, 0.0f), glm::vec3(0.10f, 0.46f, 0.10f),
                  chrome, 0.10f, 0.48f, 0.96f, 220.0f);

        // Ornate gold collar rings on column
        for (int ri = 0; ri < 3; ++ri) {
            float ry = 0.80f + ri * 1.06f;
            setCylMat(ctr + glm::vec3(0.0f, ry, 0.0f), glm::vec3(0.24f, 0.08f, 0.24f),
                      gold, 0.20f, 0.72f, 0.90f, 180.0f);
        }

        // Semi-transparent water pool inside basin
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, ctr + glm::vec3(0.0f, 0.33f, 0.0f));
            m = glm::scale(m, glm::vec3(3.80f, 0.05f, 3.80f));
            shader.setMat4("model",             m);
            shader.setVec3("objectColor",       waterBlue);
            shader.setInt("textureType",        4);
            shader.setFloat("ambientStrength",  0.22f);
            shader.setFloat("diffuseStrength",  0.60f);
            shader.setFloat("specularStrength", 1.00f);
            shader.setFloat("shininess",        256.0f);
            shader.setFloat("objectAlpha",      0.65f);
            glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
            shader.setFloat("objectAlpha", 1.0f);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        glBindVertexArray(0);
    }

    // Gold sphere finial at top of fountain
    if (c.sphereVAO != 0 && c.sphereCount > 0) {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, ctr + glm::vec3(0.0f, 3.60f, 0.0f));
        m = glm::scale(m, glm::vec3(0.30f));
        shader.setMat4("model",             m);
        shader.setVec3("objectColor",       gold);
        shader.setInt("textureType",        0);
        shader.setFloat("ambientStrength",  0.22f);
        shader.setFloat("diffuseStrength",  0.75f);
        shader.setFloat("specularStrength", 0.92f);
        shader.setFloat("shininess",        200.0f);
        shader.setFloat("objectAlpha",      1.0f);
        glBindVertexArray(c.sphereVAO);
        glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // ── 3) 8 ORNATE MARBLE COLUMNS ───────────────────────────────────────────
    const int   numCols   = 8;
    const float colRadius = 9.0f;

    if (c.cylVAO != 0) {
        glBindVertexArray(c.cylVAO);
        for (int i = 0; i < numCols; ++i) {
            float ang = glm::two_pi<float>() * (float)i / (float)numCols;
            float cx  = ctr.x + std::cos(ang) * colRadius;
            float cz2 = ctr.z + std::sin(ang) * colRadius;
            float cy  = floorY;

            auto setCol = [&](glm::vec3 pos, glm::vec3 sc, glm::vec3 col,
                              float amb, float diff, float spec, float shi) {
                glm::mat4 m = glm::mat4(1.0f);
                m = glm::translate(m, pos);
                m = glm::scale(m, sc);
                shader.setMat4("model",             m);
                shader.setVec3("objectColor",       col);
                shader.setInt("textureType",        0);
                shader.setFloat("ambientStrength",  amb);
                shader.setFloat("diffuseStrength",  diff);
                shader.setFloat("specularStrength", spec);
                shader.setFloat("shininess",        shi);
                shader.setFloat("objectAlpha",      1.0f);
                glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
            };

            // Base pedestal disc
            setCol(glm::vec3(cx, cy + 0.12f, cz2), glm::vec3(0.42f, 0.24f, 0.42f),
                   marbleDark, 0.16f, 0.68f, 0.45f, 60.0f);
            // Column shaft
            setCol(glm::vec3(cx, cy + 2.50f, cz2), glm::vec3(0.22f, 4.76f, 0.22f),
                   marbleWhite, 0.18f, 0.72f, 0.52f, 80.0f);
            // Capital disc
            setCol(glm::vec3(cx, cy + 5.05f, cz2), glm::vec3(0.42f, 0.18f, 0.42f),
                   marbleMid, 0.16f, 0.70f, 0.50f, 72.0f);
            // Gold accent ring at 1/3 shaft
            setCol(glm::vec3(cx, cy + 1.80f, cz2), glm::vec3(0.28f, 0.09f, 0.28f),
                   gold, 0.22f, 0.72f, 0.92f, 180.0f);
            // Gold accent ring at 2/3 shaft
            setCol(glm::vec3(cx, cy + 3.50f, cz2), glm::vec3(0.25f, 0.08f, 0.25f),
                   gold, 0.22f, 0.72f, 0.92f, 180.0f);

            // Small sphere cap on capital
            if (c.sphereVAO && c.sphereCount > 0) {
                glm::mat4 ms = glm::mat4(1.0f);
                ms = glm::translate(ms, glm::vec3(cx, cy + 5.26f, cz2));
                ms = glm::scale(ms, glm::vec3(0.20f));
                shader.setMat4("model", ms);
                shader.setVec3("objectColor", gold);
                shader.setFloat("specularStrength", 0.92f);
                shader.setFloat("shininess", 180.0f);
                glBindVertexArray(c.sphereVAO);
                glDrawElements(GL_TRIANGLES, c.sphereCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(c.cylVAO);
            }
        }
        glBindVertexArray(0);
    }

    // ── 4) 4 ORNATE PLANTERS (at 45° offset from columns, radius=9) ──────────
    if (c.cylVAO != 0) {
        glBindVertexArray(c.cylVAO);
        for (int i = 0; i < 4; ++i) {
            float ang = glm::two_pi<float>() * (float)i / 4.0f + glm::quarter_pi<float>();
            float px  = ctr.x + std::cos(ang) * colRadius;
            float pz  = ctr.z + std::sin(ang) * colRadius;

            auto setPot = [&](glm::vec3 pos, glm::vec3 sc, glm::vec3 col,
                              float amb, float diff, float spec, float shi) {
                glm::mat4 m = glm::mat4(1.0f);
                m = glm::translate(m, pos);
                m = glm::scale(m, sc);
                shader.setMat4("model",             m);
                shader.setVec3("objectColor",       col);
                shader.setInt("textureType",        0);
                shader.setFloat("ambientStrength",  amb);
                shader.setFloat("diffuseStrength",  diff);
                shader.setFloat("specularStrength", spec);
                shader.setFloat("shininess",        shi);
                shader.setFloat("objectAlpha",      1.0f);
                glDrawArrays(GL_TRIANGLES, 0, c.cylSegments * 12);
            };

            // Planter body (tall pot)
            setPot(glm::vec3(px, floorY + 0.45f, pz), glm::vec3(0.62f, 0.90f, 0.62f),
                   marbleDark, 0.15f, 0.65f, 0.38f, 50.0f);
            // Rim (wider disc on top)
            setPot(glm::vec3(px, floorY + 0.94f, pz), glm::vec3(0.76f, 0.10f, 0.76f),
                   marbleMid, 0.15f, 0.65f, 0.38f, 50.0f);
            // Soil disc
            setPot(glm::vec3(px, floorY + 0.99f, pz), glm::vec3(0.54f, 0.06f, 0.54f),
                   glm::vec3(0.32f, 0.22f, 0.14f), 0.14f, 0.60f, 0.12f, 10.0f);
            glBindVertexArray(0);

            // Foliage (3 overlapping cubes per planter)
            for (int li = 0; li < 3; ++li) {
                float offx = (li == 1) ? 0.16f : (li == 2 ? -0.16f : 0.0f);
                float offz = (li == 0) ? 0.16f : (li == 1 ? -0.10f : 0.08f);
                float sc   = 0.42f - li * 0.05f;
                drawCube(shader, c.cubeVAO,
                         glm::vec3(px + offx, floorY + 1.20f + li * 0.10f, pz + offz),
                         glm::vec3(sc, sc * 0.85f, sc),
                         (li % 2 == 0) ? leafGreen : leafLight,
                         0, 0.18f, 0.72f, 0.10f, 8.0f);
            }
            glBindVertexArray(c.cylVAO);
        }
        glBindVertexArray(0);
    }

    // ── 5) 4 ELEGANT BENCHES (facing fountain, at radius 5.5) ────────────────
    {
        const float benchR = 5.5f;
        for (int i = 0; i < 4; ++i) {
            float ang     = glm::two_pi<float>() * (float)i / 4.0f;
            float bx      = ctr.x + std::cos(ang) * benchR;
            float bz      = ctr.z + std::sin(ang) * benchR;
            // Bench faces the fountain center: yaw so front points inward
            float yawDeg  = glm::degrees(ang) + 90.0f;

            // Seat slab
            drawCubeRotated(shader, c.cubeVAO,
                            glm::vec3(bx, floorY + 0.50f, bz),
                            glm::vec3(1.70f, 0.08f, 0.44f),
                            glm::vec3(0.0f, yawDeg, 0.0f),
                            benchWood, 0, 0.16f, 0.65f, 0.24f, 28.0f);
            // Back rest
            drawCubeRotated(shader, c.cubeVAO,
                            glm::vec3(bx, floorY + 0.76f, bz) + rotateY(glm::vec3(0.0f, 0.0f, -0.16f), yawDeg),
                            glm::vec3(1.70f, 0.46f, 0.07f),
                            glm::vec3(0.0f, yawDeg, 0.0f),
                            benchWood, 0, 0.16f, 0.65f, 0.24f, 28.0f);
            // Armrests (2 per bench)
            for (int ai = 0; ai < 2; ++ai) {
                float ax = (ai == 0) ? -0.76f : 0.76f;
                drawCubeRotated(shader, c.cubeVAO,
                                glm::vec3(bx, floorY + 0.56f, bz) + rotateY(glm::vec3(ax, 0.0f, 0.0f), yawDeg),
                                glm::vec3(0.07f, 0.12f, 0.44f),
                                glm::vec3(0.0f, yawDeg, 0.0f),
                                benchWood, 0, 0.16f, 0.65f, 0.24f, 28.0f);
            }
            // 4 metal legs
            for (int li = 0; li < 4; ++li) {
                float lx = (li < 2) ? -0.62f : 0.62f;
                float lz = (li % 2 == 0) ? -0.14f : 0.14f;
                drawCubeRotated(shader, c.cubeVAO,
                                glm::vec3(bx, floorY + 0.24f, bz) + rotateY(glm::vec3(lx, 0.0f, lz), yawDeg),
                                glm::vec3(0.055f, 0.48f, 0.055f),
                                glm::vec3(0.0f, yawDeg, 0.0f),
                                benchMetal, 3, 0.10f, 0.52f, 0.80f, 100.0f);
            }
        }
    }

    // ── 6) CIRCULAR GLASS BALUSTRADE WITH GOLD HANDRAIL ──────────────────────
    const glm::vec3 aC(70.0f, floorY + 0.05f, 50.0f);
    const float     balR = 12.22f;
    const int       bSeg = 72;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < bSeg; ++i) {
        float a0  = glm::two_pi<float>() * (float)i       / (float)bSeg;
        float a1  = glm::two_pi<float>() * (float)(i + 1) / (float)bSeg;
        glm::vec3 p0  = aC + glm::vec3(std::cos(a0) * balR, 0.0f, std::sin(a0) * balR);
        glm::vec3 p1  = aC + glm::vec3(std::cos(a1) * balR, 0.0f, std::sin(a1) * balR);
        glm::vec3 mid = (p0 + p1) * 0.5f;
        float segLen  = glm::length(p1 - p0);
        float yaw     = glm::degrees(std::atan2(p1.z - p0.z, p1.x - p0.x));

        // Glass panel
        drawRotatedAlphaCube(shader, c.cubeVAO,
                             glm::vec3(mid.x, floorY + 0.72f, mid.z),
                             glm::vec3(segLen, 1.10f, 0.036f),
                             glm::vec3(0.0f, yaw, 0.0f),
                             glassColor, 4, 0.02f, 0.10f, 1.0f, 256.0f, 0.28f);
        // Gold top rail
        drawCubeRotated(shader, c.cubeVAO,
                        glm::vec3(mid.x, floorY + 1.33f, mid.z),
                        glm::vec3(segLen, 0.09f, 0.14f),
                        glm::vec3(0.0f, yaw, 0.0f),
                        gold, 0, 0.18f, 0.68f, 0.85f, 140.0f);
        // Gold vertical post every 3 segments
        if ((i % 3) == 0) {
            drawCube(shader, c.cubeVAO,
                     glm::vec3(mid.x, floorY + 0.52f, mid.z),
                     glm::vec3(0.06f, 0.96f, 0.06f),
                     gold, 3, 0.12f, 0.52f, 0.90f, 150.0f);
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}



// ─────────────────────────────────────────────────────────────────────────────
//  Entry point
// ─────────────────────────────────────────────────────────────────────────────
inline void drawFirstFloorLayout(float floorY, float ceilingY) {
    (void)ceilingY;

    drawAtriumCenterpiece(floorY);

    // Large flagship outlets around atrium walkway.
    // User enters from the RIGHT side (X=LOT_WIDTH=140) heading left (-X).
    // Stores on the LEFT side of the floor (small X) must face +X = rotation 270°.
    // Stores on the RIGHT side (large X) must face -X = rotation 90°.
    // Stores at the TOP/BOTTOM walls use 0°/180° to face inward along Z.
    drawFashionOutlet(glm::vec3( 24.0f, floorY, 17.0f), glm::vec2(20.0f, 12.0f), 270.0f);  // left-side store, face +X
    drawFashionOutlet(glm::vec3(116.0f, floorY, 20.0f), glm::vec2(22.0f, 12.0f),  90.0f);  // right-side store, face -X
    drawFashionOutlet(glm::vec3( 22.0f, floorY, 82.0f), glm::vec2(18.0f, 12.0f), 270.0f);  // left-side store, face +X
    drawFashionOutlet(glm::vec3(116.0f, floorY, 80.0f), glm::vec2(20.0f, 14.0f),  90.0f);  // right-side store, face -X
}

} // namespace FirstFloorDesign

#endif
