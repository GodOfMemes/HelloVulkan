#include "Mesh.hpp"

// Constructor
void Mesh::InitSlotBased(
	const std::string& meshName,
	const uint32_t vertexOffset,
	const uint32_t indexOffset,
	// Currently only support r-values
	std::vector<DefaultVertexData>&& vertices,
	std::vector<uint32_t>&& indices,
	std::unordered_map<TextureType, uint32_t>&& textureIndices)
{
	_meshName = meshName;
	ToLower(_meshName);

	_bindlessTexture = false;
	_vertexOffset = vertexOffset;
	_indexOffset = indexOffset; 
	_vertices = std::move(vertices);
	_indices = std::move(indices);
	_textureIndices = std::move(textureIndices);
	_vertexCount = static_cast<uint32_t>(_vertices.size());
	_indexCount = static_cast<uint32_t>(_indices.size());

	SetupSlotBased();
}

void Mesh::InitBindless(
	const std::string& meshName,
	const uint32_t vertexOffset,
	const uint32_t indexOffset,
	const uint32_t vertexCount,
	const uint32_t indexCount,
	std::unordered_map<TextureType, uint32_t>&& textureIndices)
{
	// Set the mesh name to lowercase, important for material detection
	_meshName = meshName;
	ToLower(_meshName);

	_bindlessTexture = true;
	_vertexOffset = vertexOffset;
	_indexOffset = indexOffset;
	_vertexCount = vertexCount;
	_indexCount = indexCount;
	_textureIndices = std::move(textureIndices);
}

void Mesh::SetupSlotBased()
{
	if (_bindlessTexture)
	{
		return;
	}

	const VkDeviceSize vertexBufferSize = static_cast< VkDeviceSize>(sizeof(DefaultVertexData) * _vertices.size());
	_vertexBuffer.CreateGPUOnlyBuffer(
		vertexBufferSize, 
		_vertices.data(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);

	const VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * _indices.size());
	_indexBuffer.CreateGPUOnlyBuffer(
		indexBufferSize, 
		_indices.data(), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
	_indexCount = static_cast<uint32_t>(_indices.size());
}

void Mesh::Destroy()
{	
	_vertexBuffer.Destroy();
	_indexBuffer.Destroy();
}
