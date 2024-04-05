#include "MeshScene.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Mesh/MeshVertex.hpp"
#include "Utility/Utility.hpp"
#include <algorithm>
#include <execution>
#include <glm/glm.hpp>
#include <iostream>

MeshScene::MeshScene(GraphicsDevice* ctx,
	const std::span<ModelCreateInfo> modelDataArray,
	const bool supportDeviceAddress) :
	ctx(ctx),
	supportDeviceAddress_(supportDeviceAddress),
	boneIDBuffer_(ctx),
	boneWeightBuffer_(ctx),
	skinningIndicesBuffer_(ctx),
	preSkinningVertexBuffer_(ctx),
	vertexBuffer_(ctx),
	indexBuffer_(ctx),
	indirectBuffer_(ctx),
	meshDataBuffer_(ctx),
	transformedBoundingBoxBuffer_(ctx)
{
	boneMatricesBuffers_.resize(MAX_FRAMES_IN_FLIGHT,{ctx});
	modelSSBOBuffers_.resize(MAX_FRAMES_IN_FLIGHT,{ctx});

	uint32_t vertexOffset = 0u;
	uint32_t indexOffset = 0u;
	for (const ModelCreateInfo& mData : modelDataArray)
	{
		Model m{ctx};
		m.LoadBindless(
			mData,
			MeshSceneData_);
		models_.push_back(m);
	}
	CreateBindlessResources();
	CreateAnimationResources();
}

MeshScene::~MeshScene()
{
	boneIDBuffer_.Destroy();
	boneWeightBuffer_.Destroy();
	skinningIndicesBuffer_.Destroy();
	preSkinningVertexBuffer_.Destroy();
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
	indirectBuffer_.Destroy();
	meshDataBuffer_.Destroy();
	transformedBoundingBoxBuffer_.Destroy();
	for (auto& buffer : modelSSBOBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& buffer : boneMatricesBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& model : models_)
	{
		model.Destroy();
	}
}

BDA MeshScene::GetBDA() const
{
	return
	{
		.vertexBufferAddress = vertexBuffer_.GetAddress(),
		.indexBufferAddress = indexBuffer_.GetAddress(),
		.meshDataBufferAddress = meshDataBuffer_.GetAddress()
	};
}

void MeshScene::GetOffsetAndDrawCount(MaterialType matType, VkDeviceSize& offset, uint32_t& drawCount) const
{
	offset = 0;
	drawCount = 0;

	const int meshDataSize = static_cast<int>(meshDataArray_.size());
	
	// Calculate left
	int left = 0;
	for (; left < meshDataSize; ++left)
	{
		if (meshDataArray_[left].material_ == matType)
		{
			break;
		}
	}

	// Calculate right
	int right = meshDataSize - 1;
	for (; right >= 0 && right > left; --right)
	{
		if (meshDataArray_[right].material_ == matType)
		{
			break;
		}
	}

	// Edge case
	if (left > right)
	{
		offset = 0;
		drawCount = 0;
	}

	// if left == right, then drawCount == 1

	drawCount = right - left + 1;
	offset = left * sizeof(VkDrawIndirectCommand);
}

void MeshScene::CreateBindlessResources()
{
	CreateDataStructures();

	// Support for bindless rendering
	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (supportDeviceAddress_) { bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; }
	
	// Vertices
	// NOTE This may contain post-skinning vertices
	const VkDeviceSize vertexBufferSize = sizeof(DefaultVertexData) * MeshSceneData_.vertices_.size();
	vertexBuffer_.CreateGPUOnlyBuffer(
		vertexBufferSize,
		MeshSceneData_.vertices_.data(),
		bufferUsage);

	// Indices
	const VkDeviceSize indexBufferSize = sizeof(uint32_t) * MeshSceneData_.indices_.size();
	indexBuffer_.CreateGPUOnlyBuffer(
		indexBufferSize,
		MeshSceneData_.indices_.data(),
		bufferUsage);
	triangleCount_ = static_cast<uint32_t>(MeshSceneData_.indices_.size()) / 3u; // TODO This somehow can be wrong

	// Mesh Data
	const VkDeviceSize meshDataBufferSize = sizeof(MeshData) * meshDataArray_.size();
	meshDataBuffer_.CreateGPUOnlyBuffer(
		meshDataBufferSize,
		meshDataArray_.data(), 
		bufferUsage);

	// Transform matrices
	const VkDeviceSize modelSSBOBufferSize = sizeof(ModelUBO) * modelSSBOs_.size();
	constexpr uint32_t frameCount = MAX_FRAMES_IN_FLIGHT;
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelSSBOBuffers_[i].CreateBuffer(modelSSBOBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
		modelSSBOBuffers_[i].UploadBufferData(modelSSBOs_.data(), modelSSBOBufferSize);
	}

	// Bounding boxes
	const VkDeviceSize bbBufferSize = transformedBoundingBoxes_.size() * sizeof(BoundingBox);
	transformedBoundingBoxBuffer_.CreateBuffer(
		bbBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	transformedBoundingBoxBuffer_.UploadBufferData(transformedBoundingBoxes_.data(), bbBufferSize);

	// Indirect buffers
	CreateIndirectBuffer(indirectBuffer_);
}

void MeshScene::UpdateAnimation(float deltaTime)
{
	if (!HasAnimation())
	{
		return;
	}

	{
		ZoneScopedNC("UpdateAnimation", tracy::Color::GreenYellow);

		for (uint32_t i = 0; i < animators_.size(); ++i)
		{
			if (models_[i].ProcessAnimation())
			{
				animators_[i].UpdateAnimation(&(animations_[i]), skinningMatrices_, deltaTime);
			}
		}
	}
	{
		ZoneScopedNC("Update boneMatricesBuffers_", tracy::Color::Orange);

		const uint32_t frameIndex = ctx->GetCurrentFrameIdx();
		const VkDeviceSize matrixBufferSize = sizeof(glm::mat4) * skinningMatrices_.size();
		boneMatricesBuffers_[frameIndex].UploadBufferData(skinningMatrices_.data(), matrixBufferSize);
	}
}

void MeshScene::CreateAnimationResources()
{
	if (!HasAnimation())
	{
		return;
	}

	skinningMatrices_.reserve(MeshSceneData_.boneMatrixCount_ + 1);
	for (uint32_t i = 0; i < MeshSceneData_.boneMatrixCount_; i++)
	{
		skinningMatrices_.emplace_back(1.0f);
	}

	animations_.resize(models_.size());
	animators_.resize(models_.size());
	for (uint32_t i = 0; i < models_.size(); ++i)
	{
		if (models_[i].ProcessAnimation())
		{
			animations_[i].Init(models_[i].filepath_, &(models_[i]));
		}
	}

	// Support for bindless rendering
	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (supportDeviceAddress_) { bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; }

	// Bone IDs
	const VkDeviceSize boneIDBufferSize = sizeof(iSVec) * MeshSceneData_.boneIDArray_.size();
	boneIDBuffer_.CreateGPUOnlyBuffer(
		boneIDBufferSize,
		MeshSceneData_.boneIDArray_.data(),
		bufferUsage);
	
	// Bone weights
	const VkDeviceSize boneWeightBufferSize = sizeof(fSVec) * MeshSceneData_.boneWeightArray_.size();
	boneWeightBuffer_.CreateGPUOnlyBuffer(
		boneWeightBufferSize,
		MeshSceneData_.boneWeightArray_.data(),
		bufferUsage);

	// Map from preSkinningVertices to vertices
	const VkDeviceSize skinningIndicesBufferSize = sizeof(uint32_t) * MeshSceneData_.skinningIndices_.size();
	skinningIndicesBuffer_.CreateGPUOnlyBuffer(
		skinningIndicesBufferSize,
		MeshSceneData_.skinningIndices_.data(),
		bufferUsage);

	// Pre-skinned vertex buffer
	const VkDeviceSize vertexBufferSize = sizeof(DefaultVertexData) * MeshSceneData_.preSkinningVertices_.size();
	preSkinningVertexBuffer_.CreateGPUOnlyBuffer(
		vertexBufferSize,
		MeshSceneData_.preSkinningVertices_.data(),
		bufferUsage);

	// Bone matrices buffers
	const VkDeviceSize matrixBufferSize = sizeof(glm::mat4) * skinningMatrices_.size();
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		boneMatricesBuffers_[i].CreateBuffer(
			matrixBufferSize,
			bufferUsage,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void MeshScene::CreateDataStructures()
{
	uint32_t matrixCounter = 0u; // This will also be the length of modelSSBO_
	uint32_t textureCounter = 0u;
	uint32_t globalInstanceCounter = 0u;
	for (uint32_t m = 0; m < models_.size(); ++m)
	{
		const uint32_t meshCount = models_[m].GetMeshCount();
		const uint32_t instanceCount = models_[m].modelInfo_.instanceCount;

		// Create temporary bounding box array
		std::vector<BoundingBox> tempOriArray(meshCount);
		for (uint32_t i = 0; i < meshCount; ++i) // Per mesh
		{
			const uint32_t vertexStart = models_[m].meshes_[i].GetVertexOffset();
			const uint32_t vertexCount = models_[m].meshes_[i].GetVertexCount();

			auto vertices = Utility::Slide(std::span{ MeshSceneData_.vertices_ }, vertexStart, vertexCount);

			std::vector<glm::vec3> points{vertices.size()};
			std::for_each(std::execution::par,vertices.begin(),vertices.end(),[&](auto& v)
			{
				int idx = &v - &vertices[0];
				points[idx] = vertices[idx].position;
			});
			tempOriArray[i] = BoundingBox(points);
		}

		// Create the actual InstanceData
		for (uint32_t i = 0; i < instanceCount; ++i)
		{
			for (uint32_t j = 0; j < meshCount; ++j)
			{
				instanceDataArray_.push_back(
				{
					.modelIndex_ = m,
					.perModelInstanceIndex_ = i,
					.perModelMeshIndex_ = j,
					.meshData_ = models_[m].meshes_[j].GetMeshData(textureCounter, matrixCounter),
					.originalBoundingBox_ = tempOriArray[j] // Copy bounding box from temporary
				}
				);
				++globalInstanceCounter;
			}
			++matrixCounter;
		}
		textureCounter += models_[m].GetTextureCount();
	}

	// Sort based on material
	std::ranges::sort(
		std::begin(instanceDataArray_),
		std::end(instanceDataArray_),
		[](InstanceData a, InstanceData b)
		{
			return a.meshData_.material_ < b.meshData_.material_;
		});

	// Matrices
	modelSSBOs_ = std::vector<ModelUBO>(matrixCounter, { .model = glm::mat4(1.0f) }); // Identity matrices
	
	// Flat array for SSBO
	meshDataArray_.resize(globalInstanceCounter);
	for (uint32_t i = 0; i < instanceDataArray_.size(); ++i)
	{
		meshDataArray_[i] = instanceDataArray_[i].meshData_;
	}

	// Prepare bounding boxes for frustum culling
	transformedBoundingBoxes_.resize(globalInstanceCounter);
	for (uint32_t i = 0; i < globalInstanceCounter; ++i)
	{
		// Equal the original bb because at this point modelSSBOs_ only has identity matrix
		transformedBoundingBoxes_[i] = instanceDataArray_[i].originalBoundingBox_;
	}

	// Create a map
	matrixCounter = 0u;
	instanceMapArray_.resize(models_.size());
	for (size_t i = 0; i < models_.size(); ++i)
	{
		const uint32_t instanceCount = models_[i].modelInfo_.instanceCount;
		instanceMapArray_[i].resize(instanceCount);
		for (uint32_t j = 0; j < instanceCount; ++j)
		{
			instanceMapArray_[i][j].modelMatrixIndex_ = matrixCounter++;
		}
	}
	for (uint32_t i = 0; i < globalInstanceCounter; ++i)
	{
		const uint32_t modelIndex = instanceDataArray_[i].modelIndex_;
		const uint32_t perModelInstanceIndex = instanceDataArray_[i].perModelInstanceIndex_;
		instanceMapArray_[modelIndex][perModelInstanceIndex].instanceDataIndices_.push_back(i);
	}
}

void MeshScene::UpdateModelMatrix(
	const ModelUBO& modelUBO,
	const uint32_t modelIndex,
	const uint32_t perModelInstanceIndex)
{
	if (modelIndex < 0 || modelIndex >= models_.size())
	{
		std::cerr << "Cannot update ModelUBO because of invalid modelIndex " << modelIndex << "\n";
		return;
	}

	const uint32_t instanceCount = models_[modelIndex].modelInfo_.instanceCount;
	if (perModelInstanceIndex < 0 || perModelInstanceIndex >= instanceCount)
	{
		std::cerr << "Cannot update ModelUBO because of invalid instanceIndex " << perModelInstanceIndex << "\n";
		return;
	}

	const uint32_t matrixIndex = instanceMapArray_[modelIndex][perModelInstanceIndex].modelMatrixIndex_;

	// Update transformation matrix
	modelSSBOs_[matrixIndex] = modelUBO;

	// Update the buffer
	UpdateModelMatrixBuffer(modelIndex, perModelInstanceIndex);
}

void MeshScene::UpdateModelMatrixBuffer(
	const uint32_t modelIndex,
	const uint32_t perModelInstanceIndex)
{
	const uint32_t matrixIndex = instanceMapArray_[modelIndex][perModelInstanceIndex].modelMatrixIndex_;
	const ModelUBO& modelUBO = modelSSBOs_[matrixIndex];

	// Update SSBO
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		modelSSBOBuffers_[i].UploadOffsetBufferData(
			&(modelSSBOs_[matrixIndex]),
			sizeof(ModelUBO) * matrixIndex,
			sizeof(ModelUBO));
	}

	// Update bounding box buffer
	const std::vector<uint32_t>& mappedIndices = instanceMapArray_[modelIndex][perModelInstanceIndex].instanceDataIndices_;
	if (!mappedIndices.empty())
	{
		for (uint32_t i : mappedIndices)
		{
			transformedBoundingBoxes_[i] = instanceDataArray_[i].originalBoundingBox_.GetTransformed(modelUBO.model);
		}
		// Update bounding box buffers
		const uint32_t firstIndex = mappedIndices[0];
		transformedBoundingBoxBuffer_.UploadOffsetBufferData(
			transformedBoundingBoxes_.data() + firstIndex,
			sizeof(BoundingBox) * firstIndex,
			sizeof(BoundingBox) * mappedIndices.size());
	}
}

void MeshScene::CreateIndirectBuffer(Buffer& indirectBuffer)
{
	const uint32_t instanceCount = static_cast<uint32_t>(meshDataArray_.size());
	const uint32_t indirectDataSize = instanceCount * sizeof(VkDrawIndirectCommand);

	std::vector<VkDrawIndirectCommand> iCommands(instanceCount);
	for (uint32_t i = 0; i < instanceCount; ++i)
	{
		const uint32_t modelIndex = instanceDataArray_[i].modelIndex_;
		const uint32_t perModelMeshIndex = instanceDataArray_[i].perModelMeshIndex_;

		iCommands[i] =
		{
			.vertexCount = models_[modelIndex].meshes_[perModelMeshIndex].GetIndexCount(),
			.instanceCount = 1u,
			.firstVertex = 0,
			.firstInstance = i
		};
	}

	// This type of buffer is not accessible from CPU
	indirectBuffer.CreateGPUOnlyIndirectBuffer(iCommands.data(), indirectDataSize);
}

// This is for descriptor indexing
std::vector<VkDescriptorImageInfo> MeshScene::GetImageInfos() const
{
	std::vector<VkDescriptorImageInfo> textureInfoArray;
	for (auto& model : models_)
	{
		for (auto& texture : model.textureList_)
		{
			textureInfoArray.emplace_back(texture.GetDescriptorImageInfo());
		}
	}
	return textureInfoArray;
}

// This is currently a brute force but can be improved with BVH or pixel perfect technique
int MeshScene::GetClickedInstanceIndex(const Ray& ray)
{
	float tMin = std::numeric_limits<float>::max();
	//int modelIndex = -1; // Debug
	int instanceIndex = -1;

	for (size_t i = 0; i < transformedBoundingBoxes_.size(); ++i)
	{
		InstanceData& iData = instanceDataArray_[i];
		Model& m = models_[iData.modelIndex_];
		if (!m.modelInfo_.clickable) { continue; }

		float t;
		if (transformedBoundingBoxes_[i].Hit(ray, t))
		{
			if (t < tMin)
			{
				tMin = t;
				//modelIndex = iData.modelIndex;
				instanceIndex = static_cast<int>(i);
			}
		}
	}

	// Debug 
	/*if (modelIndex >= 0)
	{
		Model& m = models_[modelIndex];
		std::cout << m.filepath_ << '\n';
	}*/

	return instanceIndex;
}
