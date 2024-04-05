#pragma once

/*
Adapted from learnopengl.com/Guest-Articles/2020/Skeletal-Animation
*/

#include "Bone.hpp"
#include "Model.hpp"
#include "MeshScenePODs.hpp"
#include <string>

class Animation
{
	friend class Animator;
public:
	Animation() = default;
	~Animation() = default;

	void Init(std::string const& path, Model* model);

	[[nodiscard]] Bone* GetBone(const std::string& name);
	[[nodiscard]] float GetTicksPerSecond() const { return _ticksPerSecond; }
	[[nodiscard]] float GetDuration() const { return _duration; }
	[[nodiscard]] bool GetIndexAndOffsetMatrix(const std::string& name, int& index, glm::mat4& offsetMatrix) const;

private:
	void AddBones(const aiAnimation* animation);
	void CreateHierarchy(AnimationNode& dest, const aiNode* src);

//public:
	AnimationNode rootNode;
	float _duration = 0.0f;
	float _ticksPerSecond = 0.0f;
	Model* _model = nullptr;
	std::unordered_map<std::string, Bone> _boneMap;
};