#include "Animation.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

inline glm::mat4 CastToGLMMat4(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

void Animation::Init(std::string const& path, Model* model) 
{
	_model = model;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
	assert(scene && scene->mRootNode);

	const aiAnimation* animation = scene->mAnimations[0]; // TODO Currently can only support one animation
	_duration = static_cast<float>(animation->mDuration);
	_ticksPerSecond = static_cast<float>(animation->mTicksPerSecond);
	aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
	globalTransformation = globalTransformation.Inverse();
	CreateHierarchy(rootNode, scene->mRootNode);
	AddBones(animation);
}

Bone* Animation::GetBone(const std::string& name)
{
	if (_boneMap.contains(name))
	{
		return &_boneMap[name];
	}
	return nullptr;
}

void Animation::AddBones(const aiAnimation* animation)
{
	int boneCounter = _model->GetBoneCounter();

	// Reading channels (bones engaged in an animation and their keyframes)
	const uint32_t size = animation->mNumChannels;
	for (uint32_t i = 0; i < size; ++i)
	{
		auto channel = animation->mChannels[i];
		const std::string boneName = channel->mNodeName.data;

		// Add additional bones if not found
		if (!_model->boneInfoMap_.contains(boneName))
		{
			_model->boneInfoMap_[boneName].id_ = boneCounter;
			boneCounter++;
		}

		// Add the bone to map
		_boneMap[boneName] = Bone(boneName, _model->boneInfoMap_[boneName].id_, channel);
	}
}

void Animation::CreateHierarchy(AnimationNode& dest, const aiNode* src)
{
	assert(src);

	dest.name_ = src->mName.data;
	dest.transformation_ = CastToGLMMat4(src->mTransformation);
	dest.childrenCount_ = src->mNumChildren;

	for (uint32_t i = 0; i < src->mNumChildren; i++)
	{
		AnimationNode newData;
		CreateHierarchy(newData, src->mChildren[i]);
		dest.children_.push_back(newData);
	}
}

bool Animation::GetIndexAndOffsetMatrix(const std::string& name, int& index, glm::mat4& offsetMatrix) const
{
	if (_model->boneInfoMap_.contains(name))
	{
		index = _model->boneInfoMap_[name].id_;
		offsetMatrix = _model->boneInfoMap_[name].offsetMatrix_;
		return true;
	}
	return false;
}