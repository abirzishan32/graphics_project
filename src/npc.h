#ifndef SHARED_NPC_H
#define SHARED_NPC_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>
#include <string>

#include "shader.h"

namespace NPCShared {

enum Gender { MALE, FEMALE };

struct NPC {
	glm::vec3 position;
	glm::vec3 targetPosition;
	float rotationY = 0.0f;
	float speed = 1.0f;
	float walkCycleTime = 0.0f;
	Gender gender = MALE;
	glm::vec3 clothingColor = glm::vec3(0.2f, 0.4f, 0.7f);
	float flashTimer = 0.0f;
	float waitTimer = 0.0f;
};

struct DrawParams {
	bool sitting = false;
	bool selling = false;
	bool enableCameraFlash = false;
	int flashLightIndex = 31;
	float sitOffsetY = -0.4f;
	float sitOffsetZ = 0.25f;
};

inline void drawNPC(Shader& shader,
					unsigned int cubeVAO,
					unsigned int cylVAO,
					unsigned int sphereVAO,
					int sphereCount,
					const NPC& npc,
					const DrawParams& params,
					int cylSegments = 16) {
	if (cubeVAO == 0 || cylVAO == 0 || sphereVAO == 0 || sphereCount <= 0) return;

	glm::vec3 position = npc.position;
	float rotationY = npc.rotationY;
	float cycle = npc.walkCycleTime;
	bool isFemale = (npc.gender == FEMALE);

	glm::vec3 skinColor(0.85f, 0.65f, 0.5f);
	glm::vec3 shirtColor = npc.clothingColor;
	glm::vec3 pantsColor(0.1f, 0.1f, 0.15f);
	glm::vec3 shoeColor(0.05f, 0.05f, 0.05f);

	if (params.selling) {
		shirtColor = glm::mix(npc.clothingColor, glm::vec3(0.85f, 0.15f, 0.15f), 0.5f);
		pantsColor = glm::vec3(0.9f, 0.9f, 0.9f);
	}

	glm::mat4 root = glm::mat4(1.0f);
	root = glm::translate(root, position);
	root = glm::rotate(root, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));

	float swing = std::sin(cycle);
	float swingCos = std::cos(cycle);

	float hipL = swing * 30.0f;
	float hipR = -swing * 30.0f;
	float kneeL = std::max(0.0f, -swing) * 40.0f;
	float kneeR = std::max(0.0f, swing) * 40.0f;

	float shoulderL = -swing * 25.0f;
	float shoulderR = swing * 25.0f;
	float elbowL = std::max(0.0f, -swing) * 20.0f + 5.0f;
	float elbowR = std::max(0.0f, swing) * 20.0f + 5.0f;

	float pelvicBounce = std::abs(swingCos) * 0.05f;

	if (params.selling) {
		hipL = 0; hipR = 0; kneeL = 0; kneeR = 0; pelvicBounce = 0;
		shoulderL = 10.0f + std::sin(cycle * 0.5f) * 15.0f;
		shoulderR = 25.0f + std::cos(cycle) * 20.0f;
		elbowL = 40.0f + std::sin(cycle * 0.5f) * 5.0f;
		elbowR = 30.0f + std::cos(cycle) * 10.0f;
	}

	auto sOff = [&](float x, float y, float z) {
		float sRy = glm::radians(rotationY);
		return glm::vec3(std::cos(sRy) * x + std::sin(sRy) * z, y,
						 -std::sin(sRy) * x + std::cos(sRy) * z);
	};

	if (params.sitting) {
		hipL = -90.0f; hipR = -90.0f;
		kneeL = 90.0f;  kneeR = 90.0f;
		shoulderL = -15.0f; shoulderR = -15.0f;
		elbowL = 60.0f;  elbowR = 60.0f;
		pelvicBounce = 0.0f;
		root = glm::translate(root, sOff(0.0f, params.sitOffsetY, params.sitOffsetZ));
	}

	float pelvisY = 0.9f + pelvicBounce;
	glm::mat4 pelvisM = glm::translate(root, glm::vec3(0.0f, pelvisY, 0.0f));

	auto drawLeg = [&](int side, float hipFlex, float kneeFlex) {
		float dirMultiplier = (side == 0) ? -1.0f : 1.0f;

		glm::mat4 thighM = glm::translate(pelvisM, glm::vec3(0.12f * dirMultiplier, -0.05f, 0.0f));
		thighM = glm::rotate(thighM, glm::radians(hipFlex), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 drawThigh = glm::translate(thighM, glm::vec3(0.0f, -0.2f, 0.0f));
		drawThigh = glm::scale(drawThigh, glm::vec3(0.16f, 0.4f, 0.16f));
		shader.setMat4("model", drawThigh);
		shader.setVec3("objectColor", pantsColor);
		glBindVertexArray(cylVAO);
		glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);

		glm::mat4 calfM = glm::translate(thighM, glm::vec3(0.0f, -0.4f, 0.0f));
		calfM = glm::rotate(calfM, glm::radians(kneeFlex), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 drawCalf = glm::translate(calfM, glm::vec3(0.0f, -0.2f, 0.0f));
		drawCalf = glm::scale(drawCalf, glm::vec3(0.12f, 0.4f, 0.12f));
		shader.setMat4("model", drawCalf);
		glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);

		glm::mat4 shoeM = glm::translate(calfM, glm::vec3(0.0f, -0.42f, 0.06f));
		shoeM = glm::scale(shoeM, glm::vec3(0.14f, 0.08f, 0.25f));
		shader.setMat4("model", shoeM);
		shader.setVec3("objectColor", shoeColor);
		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	};

	if (!isFemale) {
		drawLeg(0, hipL, kneeL);
		drawLeg(1, hipR, kneeR);
	} else {
		drawLeg(0, hipL * 0.8f, kneeL * 0.6f);
		drawLeg(1, hipR * 0.8f, kneeR * 0.6f);
	}

	glm::mat4 torsoM = glm::translate(pelvisM, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 drawTorso = glm::translate(torsoM, glm::vec3(0.0f, 0.35f, 0.0f));
	drawTorso = glm::scale(drawTorso, isFemale ? glm::vec3(0.30f, 0.65f, 0.19f)
											   : glm::vec3(0.35f, 0.7f, 0.2f));
	shader.setMat4("model", drawTorso);
	shader.setVec3("objectColor", shirtColor);
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	if (isFemale) {
		for (int seg = 0; seg < 3; ++seg) {
			float t = seg / 2.0f;
			float segY = 0.20f - t * 0.28f;
			float radius = 0.20f + t * 0.12f;
			glm::mat4 dressM = glm::translate(torsoM, glm::vec3(0.0f, segY, 0.0f));
			dressM = glm::scale(dressM, glm::vec3(radius, 0.24f, radius));
			shader.setMat4("model", dressM);
			shader.setVec3("objectColor", glm::mix(shirtColor, glm::vec3(0.95f), 0.08f));
			glBindVertexArray(cylVAO);
			glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);
		}
	}

	auto drawArm = [&](int side, float shoulderFlex, float elbowFlex) {
		float dirMultiplier = (side == 0) ? -1.0f : 1.0f;
		float shoulderWidth = isFemale ? 0.20f : 0.24f;

		glm::mat4 uArmM = glm::translate(torsoM, glm::vec3(shoulderWidth * dirMultiplier, 0.58f, 0.0f));
		uArmM = glm::rotate(uArmM, glm::radians(shoulderFlex), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 drawUArm = glm::translate(uArmM, glm::vec3(0.0f, -0.15f, 0.0f));
		drawUArm = glm::scale(drawUArm, isFemale ? glm::vec3(0.10f, 0.28f, 0.10f)
												 : glm::vec3(0.12f, 0.3f, 0.12f));
		shader.setMat4("model", drawUArm);
		shader.setVec3("objectColor", shirtColor);
		glBindVertexArray(cylVAO);
		glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);

		glm::mat4 lArmM = glm::translate(uArmM, glm::vec3(0.0f, -0.3f, 0.0f));
		lArmM = glm::rotate(lArmM, glm::radians(-elbowFlex), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 drawLArm = glm::translate(lArmM, glm::vec3(0.0f, -0.15f, 0.0f));
		drawLArm = glm::scale(drawLArm, glm::vec3(0.1f, 0.3f, 0.1f));
		shader.setMat4("model", drawLArm);
		shader.setVec3("objectColor", skinColor);
		glBindVertexArray(cylVAO);
		glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);

		glm::mat4 handM = glm::translate(lArmM, glm::vec3(0.0f, -0.35f, 0.0f));
		handM = glm::scale(handM, glm::vec3(0.08f, 0.12f, 0.12f));
		shader.setMat4("model", handM);
		shader.setVec3("objectColor", skinColor);
		glBindVertexArray(sphereVAO);
		glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, 0);
	};

	drawArm(0, shoulderL, elbowL);
	drawArm(1, shoulderR, elbowR);

	glm::mat4 neckM = glm::translate(torsoM, glm::vec3(0.0f, 0.75f, 0.0f));
	glm::mat4 drawNeck = glm::scale(neckM, glm::vec3(0.1f, 0.1f, 0.1f));
	shader.setMat4("model", drawNeck);
	shader.setVec3("objectColor", skinColor);
	glBindVertexArray(cylVAO);
	glDrawArrays(GL_TRIANGLES, 0, cylSegments * 12);

	glm::mat4 headM = glm::translate(neckM, glm::vec3(0.0f, 0.15f, 0.0f));
	headM = glm::scale(headM, glm::vec3(0.22f, 0.25f, 0.22f));
	shader.setMat4("model", headM);
	shader.setVec3("objectColor", skinColor);
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	if (isFemale) {
		glm::mat4 hairM = glm::translate(neckM, glm::vec3(0.0f, 0.12f, -0.15f));
		hairM = glm::scale(hairM, glm::vec3(0.24f, 0.36f, 0.18f));
		shader.setMat4("model", hairM);
		shader.setVec3("objectColor", glm::vec3(0.16f, 0.10f, 0.05f));
		glBindVertexArray(sphereVAO);
		glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, 0);
	}

	if (params.enableCameraFlash && params.flashLightIndex >= 0) {
		const char* base = "pointLights[";
		std::string idx = std::to_string(params.flashLightIndex);
		shader.setVec3((std::string(base) + idx + "].position").c_str(), position + glm::vec3(0.0f, 1.5f, 0.0f));
		shader.setVec3((std::string(base) + idx + "].ambient").c_str(), glm::vec3(0.5f));
		shader.setVec3((std::string(base) + idx + "].diffuse").c_str(), glm::vec3(5.0f, 5.0f, 6.0f));
		shader.setVec3((std::string(base) + idx + "].specular").c_str(), glm::vec3(4.0f));
	}
}

} // namespace NPCShared

#endif
