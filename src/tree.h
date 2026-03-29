#ifndef SHARED_FRACTAL_TREE_H
#define SHARED_FRACTAL_TREE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>

#include "shader.h"

namespace FractalTree {

struct Style {
	glm::vec3 trunkColor;
	glm::vec3 leafPrimary;
	glm::vec3 leafSecondary;

	float trunkAmbient = 0.16f;
	float trunkDiffuse = 0.66f;
	float trunkSpecular = 0.10f;
	float trunkShininess = 14.0f;

	float leafAmbient = 0.24f;
	float leafDiffuse = 0.72f;
	float leafSpecular = 0.10f;
	float leafShininess = 18.0f;

	int branchFactor = 3;
	float tiltDegrees = 34.0f;
	float lengthRatio = 0.74f;
	float radiusRatio = 0.66f;
	float leafScale = 0.16f;
	int leafClusterCount = 3;
};

inline Style makeGroundGateStyle() {
	Style s;
	s.trunkColor = glm::vec3(0.23f, 0.13f, 0.08f);
	s.leafPrimary = glm::vec3(0.18f, 0.66f, 0.18f);
	s.leafSecondary = glm::vec3(0.28f, 0.78f, 0.24f);
	s.branchFactor = 3;
	s.tiltDegrees = 32.0f;
	s.lengthRatio = 0.75f;
	s.radiusRatio = 0.68f;
	s.leafScale = 0.17f;
	s.leafClusterCount = 6;
	return s;
}

inline Style makeAtriumStyle() {
	Style s;
	s.trunkColor = glm::vec3(0.24f, 0.14f, 0.08f);
	s.leafPrimary = glm::vec3(0.95f, 0.52f, 0.72f);
	s.leafSecondary = glm::vec3(0.36f, 0.78f, 0.34f);
	s.branchFactor = 3;
	s.tiltDegrees = 35.0f;
	s.lengthRatio = 0.75f;
	s.radiusRatio = 0.65f;
	s.leafScale = 0.18f;
	s.leafClusterCount = 4;
	return s;
}

inline void drawLeafCluster(Shader& shader,
							unsigned int cubeVAO,
							unsigned int sphereVAO,
							int sphereCount,
							const glm::mat4& parentMatrix,
							const Style& style,
							float seed) {
	glm::vec3 leafColor = (std::sin(seed) > 0.0f) ? style.leafPrimary : style.leafSecondary;

	const glm::vec3 offsets[8] = {
		glm::vec3(0.00f, style.leafScale * 0.65f, 0.00f),
		glm::vec3(style.leafScale * 0.55f, style.leafScale * 0.18f, style.leafScale * 0.25f),
		glm::vec3(-style.leafScale * 0.52f, style.leafScale * 0.24f, -style.leafScale * 0.28f),
		glm::vec3(style.leafScale * 0.22f, style.leafScale * 0.78f, -style.leafScale * 0.16f),
		glm::vec3(-style.leafScale * 0.26f, style.leafScale * 0.70f, style.leafScale * 0.18f),
		glm::vec3(style.leafScale * 0.68f, style.leafScale * 0.40f, -style.leafScale * 0.04f),
		glm::vec3(-style.leafScale * 0.66f, style.leafScale * 0.36f, style.leafScale * 0.02f),
		glm::vec3(0.00f, style.leafScale * 0.95f, style.leafScale * 0.05f)
	};
	int clusterCount = std::max(1, std::min(style.leafClusterCount, 8));

	shader.setVec3("objectColor", leafColor);
	shader.setInt("textureType", 0);
	shader.setFloat("ambientStrength", style.leafAmbient);
	shader.setFloat("diffuseStrength", style.leafDiffuse);
	shader.setFloat("specularStrength", style.leafSpecular);
	shader.setFloat("shininess", style.leafShininess);
	shader.setFloat("objectAlpha", 1.0f);

	if (sphereVAO != 0 && sphereCount > 0) {
		glBindVertexArray(sphereVAO);
		for (int i = 0; i < clusterCount; ++i) {
			glm::vec3 leafColor = ((i + (int)(seed * 10.0f)) % 2 == 0) ? style.leafPrimary : style.leafSecondary;
			shader.setVec3("objectColor", leafColor);
			glm::mat4 m = glm::translate(parentMatrix, offsets[i]);
			m = glm::scale(m, glm::vec3(style.leafScale + 0.014f * (float)i));
			shader.setMat4("model", m);
			glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, 0);
		}
	} else if (cubeVAO != 0) {
		glBindVertexArray(cubeVAO);
		glm::mat4 m = glm::translate(parentMatrix, glm::vec3(0.0f, style.leafScale * 0.55f, 0.0f));
		m = glm::scale(m, glm::vec3(style.leafScale * 1.4f, style.leafScale, style.leafScale * 1.3f));
		shader.setMat4("model", m);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
}

inline void drawFractalBranch(Shader& shader,
							  unsigned int cubeVAO,
							  unsigned int cylVAO,
							  int cylSegments,
							  unsigned int sphereVAO,
							  int sphereCount,
							  const glm::mat4& parentMatrix,
							  int depth,
							  float length,
							  float radius,
							  const Style& style,
							  float seed) {
	if (depth <= 0) {
		drawLeafCluster(shader, cubeVAO, sphereVAO, sphereCount, parentMatrix, style, seed);
		return;
	}

	glm::mat4 branchModel = glm::translate(parentMatrix, glm::vec3(0.0f, length * 0.5f, 0.0f));
	branchModel = glm::scale(branchModel, glm::vec3(radius, length, radius));

	shader.setMat4("model", branchModel);
	shader.setVec3("objectColor", style.trunkColor);
	shader.setInt("textureType", 0);
	shader.setFloat("ambientStrength", style.trunkAmbient);
	shader.setFloat("diffuseStrength", style.trunkDiffuse);
	shader.setFloat("specularStrength", style.trunkSpecular);
	shader.setFloat("shininess", style.trunkShininess);
	shader.setFloat("objectAlpha", 1.0f);
	glBindVertexArray(cylVAO);
	glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);

	glm::mat4 tipMatrix = glm::translate(parentMatrix, glm::vec3(0.0f, length, 0.0f));

	int branchFactor = std::max(2, std::min(style.branchFactor, 4));
	for (int i = 0; i < branchFactor; ++i) {
		float yawDeg = (360.0f / (float)branchFactor) * (float)i + std::sin(seed + i * 1.71f) * 7.0f;
		float pitchDeg = style.tiltDegrees + std::cos(seed + i * 0.83f) * 6.0f;

		glm::mat4 child = tipMatrix;
		child = glm::rotate(child, glm::radians(yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
		child = glm::rotate(child, glm::radians(pitchDeg), glm::vec3(0.0f, 0.0f, 1.0f));

		drawFractalBranch(shader, cubeVAO, cylVAO, cylSegments, sphereVAO, sphereCount,
						  child, depth - 1,
						  length * style.lengthRatio,
						  radius * style.radiusRatio,
						  style,
						  seed + 0.37f + (float)i * 0.51f);
	}
}

inline void drawFractalTree(Shader& shader,
							unsigned int cubeVAO,
							unsigned int cylVAO,
							int cylSegments,
							unsigned int sphereVAO,
							int sphereCount,
							glm::vec3 rootPosition,
							int maxDepth,
							float trunkLength,
							float trunkRadius,
							const Style& style,
							float seed = 0.0f) {
	if (cylVAO == 0 || cylSegments <= 0) return;

	int safeDepth = std::max(1, std::min(maxDepth, 5));
	float safeLength = std::max(0.15f, trunkLength);
	float safeRadius = std::max(0.02f, trunkRadius);

	glm::mat4 rootMatrix = glm::mat4(1.0f);
	rootMatrix = glm::translate(rootMatrix, rootPosition);

	float finalSeed = seed + rootPosition.x * 0.019f + rootPosition.z * 0.031f;
	drawFractalBranch(shader, cubeVAO, cylVAO, cylSegments, sphereVAO, sphereCount,
					  rootMatrix, safeDepth, safeLength, safeRadius, style, finalSeed);
}

} // namespace FractalTree

#endif
