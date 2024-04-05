#pragma once

//#include "BDA.h"
#include "Graphics/GraphicsDevice.hpp"
#include "Model.hpp"
#include "Animation.hpp"
#include "Animator.hpp"
#include "MeshScenePODs.hpp"
#include <vector>
#include <span>

/*
A MeshScene used for indirect draw + bindless resources that contains SSBO buffers for vertices, indices, and mesh data.

Below is an example of a MeshScene structure:

MeshScene
|
|-- Model
|    |-- Mesh
|    `-- Mesh
|
|-- Model
|    |-- Mesh
|    |-- Mesh
|    `-- Mesh

A MeshScene has multiple models, each model is loaded from a file (fbx, glTF, etc.), and a model has multiple meshes.
A single mesh requires one draw call. For example, if the MeshScene has 10 meshes, 10 draw calls will be issued.
The MeshScene representation supports instances, each is defined as a copy of a mesh.
Instances only duplicate the draw call of a mesh, so this is different than hardware instancing.

*/
class MeshScene
{
public:
	MeshScene(
		GraphicsDevice* ctx,
		const std::span<ModelCreateInfo> modelDataArray,
		const bool supportDeviceAddress = false);
	~MeshScene();

	[[nodiscard]] uint32_t GetInstanceCount() const { return static_cast<uint32_t>(meshDataArray_.size()); }
	[[nodiscard]] std::vector<VkDescriptorImageInfo> GetImageInfos() const;
	[[nodiscard]] BDA GetBDA() const;
	[[nodiscard]] int GetClickedInstanceIndex(const Ray& ray);

	void GetOffsetAndDrawCount(MaterialType matType, VkDeviceSize& offset, uint32_t& drawCount) const;

	/*Update model matrix and update the buffer
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	void UpdateModelMatrix(
		const ModelUBO& modelUBO,
		const uint32_t modelIndex,
		const uint32_t perModelInstanceIndex);

	/*Only update the buffer of model matrix
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	void UpdateModelMatrixBuffer(
		const uint32_t modelIndex,
		const uint32_t perModelInstanceIndex);
	
	void CreateIndirectBuffer(Buffer& indirectBuffer);

	void UpdateAnimation(float deltaTime);

private:
	[[nodiscard]] bool HasAnimation() const { return !MeshSceneData_.boneIDArray_.empty(); }

	void CreateAnimationResources();
	void CreateBindlessResources();
	void CreateDataStructures();
	
public:
	uint32_t triangleCount_ = 0;
	MeshSceneData MeshSceneData_; // Containing vertices and indices
	std::vector<ModelUBO> modelSSBOs_;

	// These three have the same length
	std::vector<MeshData> meshDataArray_; // Content is sent to meshDataBuffer_
	std::vector<InstanceData> instanceDataArray_;
	std::vector<BoundingBox> transformedBoundingBoxes_; // Content is sent to transformedBoundingBoxBuffer_
	
	// Animation
	Buffer boneIDBuffer_;
	Buffer boneWeightBuffer_;
	Buffer skinningIndicesBuffer_;
	Buffer preSkinningVertexBuffer_; // This buffer contains a subset of vertexBuffer_
	std::vector<Buffer> boneMatricesBuffers_; // Frame-in-flight

	// Vertex pulling
	Buffer vertexBuffer_; 
	Buffer indexBuffer_;
	Buffer indirectBuffer_;
	Buffer meshDataBuffer_;
	std::vector<Buffer> modelSSBOBuffers_; // Frame-in-flight

	// Frustum culling
	Buffer transformedBoundingBoxBuffer_; // TODO No Frame-in-flight but somenow not giving error
	
private:
	GraphicsDevice* ctx;
	bool supportDeviceAddress_ = false;

	std::vector<Model> models_;

	/*Update model matrix and update the buffer
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	std::vector<std::vector<InstanceMap>> instanceMapArray_;

	// Animation
	std::vector<glm::mat4> skinningMatrices_;
	std::vector<Animation> animations_;
	std::vector<Animator> animators_;
};
