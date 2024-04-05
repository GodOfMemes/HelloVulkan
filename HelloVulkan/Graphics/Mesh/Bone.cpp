#include "Bone.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

#include <iostream>

inline glm::vec3 CastToGLMVec3(const aiVector3D& vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}

inline glm::quat CastToGLMQuat(const aiQuaternion& pOrientation)
{
	return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel) :
	_name(name),
	_localTransform(1.0f),
	_id(id)
{
	_positionCount = channel->mNumPositionKeys;

	for (uint32_t positionIndex = 0; positionIndex < _positionCount; ++positionIndex)
	{
		aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
		float timeStamp = static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
		KeyPosition data =
		{
			.position_ = CastToGLMVec3(aiPosition),
			.timeStamp_ = timeStamp
		};
		_positions.push_back(data);
	}

	_rotationCount = channel->mNumRotationKeys;
	for (uint32_t rotationIndex = 0; rotationIndex < _rotationCount; ++rotationIndex)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
		float timeStamp = static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
		KeyRotation data =
		{
			.orientation_ = CastToGLMQuat(aiOrientation),
			.timeStamp_ = timeStamp
		};
		_rotation.push_back(data);
	}

	_scalingCount = channel->mNumScalingKeys;
	for (uint32_t keyIndex = 0; keyIndex < _scalingCount; ++keyIndex)
	{
		aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
		float timeStamp = static_cast<float>(channel->mScalingKeys[keyIndex].mTime);
		KeyScale data =
		{
			.scale_ = CastToGLMVec3(scale),
			.timeStamp_ = timeStamp
		};
		_scales.push_back(data);
	}
}

void Bone::Update(float animationTime)
{
	const glm::mat4 translation = InterpolatePosition(animationTime);
	const glm::mat4 rotation = InterpolateRotation(animationTime);
	const glm::mat4 scale = InterpolateScaling(animationTime);
	_localTransform = translation * rotation * scale;
}

int Bone::GetPositionIndex(float animationTime) const
{
	for (uint32_t index = 0; index < _positionCount - 1; ++index)
	{
		if (animationTime < _positions[index + 1].timeStamp_)
		{
			return static_cast<int>(index);
		}
	}
	std::cerr << "Position index is -1\n";
	return -1;
}

int Bone::GetRotationIndex(float animationTime) const
{
	for (uint32_t index = 0; index < _rotationCount - 1; ++index)
	{
		if (animationTime < _rotation[index + 1].timeStamp_)
		{
			return static_cast<int>(index);
		}
	}
	std::cerr << "Rotation index is -1\n";
	return -1;
}

int Bone::GetScaleIndex(float animationTime) const
{
	for (uint32_t index = 0; index < _scalingCount - 1; ++index)
	{
		if (animationTime < _scales[index + 1].timeStamp_)
		{
			return static_cast<int>(index);
		}
	}
	std::cerr << "Scale index is -1\n";
	return -1;
}

float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	const float midWayLength = animationTime - lastTimeStamp;
	const float framesDiff = nextTimeStamp - lastTimeStamp;
	return midWayLength / framesDiff;
}

glm::mat4 Bone::InterpolatePosition(float animationTime)
{
	if (_positionCount == 1)
	{
		return glm::translate(glm::mat4(1.0f), _positions[0].position_);
	}

	const int p0Index = GetPositionIndex(animationTime);
	const int p1Index = p0Index + 1;
	const float scaleFactor = GetScaleFactor(_positions[p0Index].timeStamp_,
		_positions[p1Index].timeStamp_, animationTime);
	const glm::vec3 finalPosition =
		glm::mix(_positions[p0Index].position_,
			_positions[p1Index].position_,
			scaleFactor);
	return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::InterpolateRotation(float animationTime)
{
	if (_rotationCount == 1)
	{
		const auto rotation = glm::normalize(_rotation[0].orientation_);
		return glm::toMat4(rotation);
	}

	const int p0Index = GetRotationIndex(animationTime);
	const int p1Index = p0Index + 1;
	const float scaleFactor = GetScaleFactor(_rotation[p0Index].timeStamp_,
		_rotation[p1Index].timeStamp_, animationTime);
	glm::quat finalRotation =
		glm::slerp(_rotation[p0Index].orientation_,
			_rotation[p1Index].orientation_,
			scaleFactor);
	finalRotation = glm::normalize(finalRotation);
	return glm::toMat4(finalRotation);

}

glm::mat4 Bone::InterpolateScaling(float animationTime)
{
	if (_scalingCount == 1)
	{
		return glm::scale(glm::mat4(1.0f), _scales[0].scale_);
	}

	const int p0Index = GetScaleIndex(animationTime);
	const int p1Index = p0Index + 1;
	const float scaleFactor = GetScaleFactor(_scales[p0Index].timeStamp_,
		_scales[p1Index].timeStamp_, animationTime);
	const glm::vec3 finalScale =
		glm::mix(_scales[p0Index].scale_,
			_scales[p1Index].scale_,
			scaleFactor);
	return glm::scale(glm::mat4(1.0f), finalScale);
}