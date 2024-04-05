#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Buffer.hpp"
#include "TextureMapper.hpp"
#include "MeshScenePODs.hpp"

// TODO: Move Buffer Creation to GraphicsDevice
class Mesh
{
//public:
private:
	// Slot based
	Buffer _vertexBuffer;
	Buffer _indexBuffer;

	std::unordered_map<TextureType, uint32_t> _textureIndices;

//private:
	GraphicsDevice* gd;
	bool _bindlessTexture = false;
	std::string _meshName;

	uint32_t _vertexOffset = 0;
	uint32_t _indexOffset = 0;

	// Slot-based rendering
	std::vector<DefaultVertexData> _vertices;
	std::vector<uint32_t> _indices;

	uint32_t _vertexCount = 0;
	uint32_t _indexCount = 0;

public:
	Mesh(GraphicsDevice* gd) 
		: gd(gd), _vertexBuffer(gd), _indexBuffer(gd) {}
	~Mesh() { Destroy(); }

	void InitSlotBased(
		const std::string& meshName,
		const uint32_t vertexOffset,
		const uint32_t indexOffset,
		std::vector<DefaultVertexData>&& _vertices,
		std::vector<uint32_t>&& _indices,
		std::unordered_map<TextureType, uint32_t>&& textureIndices
	);
	void InitBindless(
		const std::string& meshName,
		const uint32_t vertexOffset,
		const uint32_t indexOffset,
		const uint32_t vertexCount,
		const uint32_t indexCount,
		std::unordered_map<TextureType, uint32_t>&& textureIndices);
	void SetupSlotBased();
	void Destroy();

	[[nodiscard]] uint32_t GetIndexCount() const { return _indexCount; }
	[[nodiscard]] uint32_t GetVertexOffset() const { return _vertexOffset; }
	[[nodiscard]] uint32_t GetVertexCount() const { return _vertexCount; }

	[[nodiscard]] MeshData GetMeshData(uint32_t textureIndexOffset, uint32_t modelMatrixIndex)
	{
		return
		{
			.vertexOffset_ = _vertexOffset,
			.indexOffset_ = _indexOffset,
			.modelMatrixIndex_ = modelMatrixIndex,
			.albedo_ = _textureIndices[TextureType::Albedo] + textureIndexOffset,
			.normal_ = _textureIndices[TextureType::Normal] + textureIndexOffset,
			.metalness_ = _textureIndices[TextureType::Metalness] + textureIndexOffset,
			.roughness_ = _textureIndices[TextureType::Roughness] + textureIndexOffset,
			.ao_ = _textureIndices[TextureType::AmbientOcclusion] + textureIndexOffset,
			.emissive_ = _textureIndices[TextureType::Emissive] + textureIndexOffset,
			.material_ = GetMaterialType()
		};
	}

	[[nodiscard]] MaterialType GetMaterialType() const
	{
		if (_meshName.find("transparent") != _meshName.npos)
		{
			return MaterialType::Transparent;
		}
		return MaterialType::Opaque;
	}

	void ToLower(std::string& str) const
	{
		for (auto& c : str)
		{
			// TODO Narrowing conversion
			c = tolower(c);
		}
	}
};
