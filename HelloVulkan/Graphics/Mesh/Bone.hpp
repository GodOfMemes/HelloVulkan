#pragma once

/*
Adapted from learnopengl.com/Guest-Articles/2020/Skeletal-Animation
*/

#include "MeshScenePODs.hpp"
#include <assimp/scene.h>

class Bone
{
public:
	Bone() = default;
	Bone(const std::string& name, int id, const aiNodeAnim* channel);

	void Update(float animationTime);

	[[nodiscard]] glm::mat4 GetLocalTransform() const { return _localTransform; }
	[[nodiscard]] std::string GetBoneName() const { return _name; }

private:
	[[nodiscard]] int GetPositionIndex(float animationTime) const;
	[[nodiscard]] int GetRotationIndex(float animationTime) const;
	[[nodiscard]] int GetScaleIndex(float animationTime) const;
	[[nodiscard]] float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
	[[nodiscard]] glm::mat4 InterpolatePosition(float animationTime);
	[[nodiscard]] glm::mat4 InterpolateRotation(float animationTime);
	[[nodiscard]] glm::mat4 InterpolateScaling(float animationTime);

private:
	std::vector<KeyPosition> _positions;
	std::vector<KeyRotation> _rotation;
	std::vector<KeyScale> _scales;
	std::string _name;
	glm::mat4 _localTransform ;
	uint32_t _positionCount = 0;
	uint32_t _rotationCount = 0;
	uint32_t _scalingCount = 0;
	int32_t _id = -1;
};