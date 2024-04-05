#pragma once

/*
Adapted from learnopengl.com/Guest-Articles/2020/Skeletal-Animation
*/

#include "Animation.hpp"
#include <glm/glm.hpp>
#include <vector>

class Animator
{
public:
	Animator();
	void Reset();

	void UpdateAnimation(
		Animation* animation,
		std::vector<glm::mat4>& skinningMatrices,
		float dt);
	
private:
	void CalculateBoneTransform(
		Animation* animation,
		const AnimationNode* node,
		const glm::mat4& parentTransform,
		std::vector<glm::mat4>& skinningMatrices);

private:
	float _currentTime = 0.0f;
	float _deltaTime = 0.0f;
};