#ifndef MODEL
#define MODEL

#include "Graphics/Mesh/MeshScenePODs.hpp"
#include "Mesh.hpp"
#include "TextureMapper.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Texture2D.hpp"

#include <string>
#include <vector>
#include <unordered_map>

#include <assimp/scene.h>

class Model
{
public:
	std::string filepath_ = {};
	std::vector<Mesh> meshes_ = {};

	// NOTE Textures are stored in Model regardless of bindless textures or Slot-Based
	std::vector<Texture2D> textureList_ = {};

	// Optional per-frame buffers for model matrix
	// TODO Maybe can be moved to pipelines
	std::vector<Buffer> modelBuffers_ = {};

	// This is used to store the filename and to activate instancing in bindless setup
	ModelCreateInfo modelInfo_ = {};

	// Skinning
	std::unordered_map<std::string, BoneInfo> boneInfoMap_ = {};

private:
	const aiScene* scene_ = nullptr;
	bool bindlessTexture_ = false;
	VkDevice device_ = nullptr;
	std::string directory_ = {};

	// Skinning
	int boneCounter_ = 0;
	bool processAnimation_ = false;

	// string key is the filename, int value points to elements in textureList_
	std::unordered_map<std::string, uint32_t> textureMap_ = {};

	GraphicsDevice* gd;

public:
	Model(GraphicsDevice* gd)
		: gd(gd) {};
	
	~Model() = default;

	void Destroy();

	void LoadSlotBased(const std::string& path);
	void LoadBindless(const ModelCreateInfo& modelInfo,MeshSceneData& sceneData);

	[[nodiscard]] const aiScene* GetAssimpScene() const { return scene_; }
	[[nodiscard]] Texture2D* GetTexture(uint32_t textureIndex);
	[[nodiscard]] uint32_t GetTextureCount() const { return static_cast<uint32_t>(textureList_.size()); }
	[[nodiscard]] uint32_t GetMeshCount() const { return static_cast<uint32_t>(meshes_.size()); }
	[[nodiscard]] int GetBoneCounter() const { return boneCounter_; }
	[[nodiscard]] int ProcessAnimation() const { return processAnimation_; }

	void CreateModelUBOBuffers();
	void SetModelUBO(ModelUBO ubo);

private:
	void AddTexture(const std::string& textureFilename);
	void AddTexture(const std::string& textureName, void* data, int width, int height);

	// Entry point
	void LoadModel(std::string const& path,MeshSceneData& sceneData);

	// Processes a node recursively
	void ProcessNode(
		MeshSceneData& sceneData,
		const aiNode* node,
		const glm::mat4& parentTransform);

	void ProcessMesh(
		MeshSceneData& sceneData,
		const aiMesh* mesh,
		const glm::mat4& transform);

	void SetBoneToDefault(
		std::vector<uint32_t>& skinningIndices,
		std::vector<iSVec>& boneIDArray,
		std::vector<fSVec>& boneWeightArray,
		uint32_t vertexCount,
		uint32_t prevVertexOffset);

	void ExtractBoneWeight(
		std::vector<iSVec>& boneIDs,
		std::vector<fSVec>& boneWeights,
		const aiMesh* mesh);

	[[nodiscard]] std::vector<DefaultVertexData> GetMeshVertices(const aiMesh* mesh, const glm::mat4& transform);
	[[nodiscard]] std::vector<uint32_t> GetMeshIndices(const aiMesh* mesh);
	[[nodiscard]] std::unordered_map<TextureType, uint32_t> GetMeshTextures(const aiMesh* mesh);
};

#endif
