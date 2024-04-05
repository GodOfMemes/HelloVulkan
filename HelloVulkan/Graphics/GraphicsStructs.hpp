#pragma once

#include <cmath>
#include <volk.h> //<vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <glm/matrix.hpp>

namespace ClusterForwardConfig
{
	// Parameters similar to Doom 2016
	constexpr uint32_t SliceCountX = 16;
	constexpr uint32_t SliceCountY = 9;
	constexpr uint32_t SliceCountZ = 24;
	constexpr uint32_t ClusterCount = SliceCountX * SliceCountY * SliceCountZ;

	// Note that this also has to be set inside the compute shader
	constexpr uint32_t maxLightPerCluster = 150;
}

namespace IBLConfig
{
	constexpr uint32_t OutputDiffuseSampleCount = 1024;
	constexpr uint32_t InputCubeSideLength = 1024;
	constexpr uint32_t OutputDiffuseSideLength = 32;
	constexpr uint32_t OutputSpecularSideLength = 128;
	constexpr uint32_t LayerCount = 6;
	constexpr VkFormat CubeFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

	// BRDF LUT
	constexpr uint32_t LUTSampleCount = 1024;
	constexpr uint32_t LUTWidth = 256;
	constexpr uint32_t LUTHeight = 256;
	constexpr uint32_t LUTBufferSize = 2 * sizeof(float) * LUTWidth * LUTHeight;
}

namespace ShadowConfig
{
	constexpr uint32_t DepthSize = 4096;
}

///////////////////////// PC  //////////////////////////////////

// For IBL Lookup Table
struct BRDFLUTPushConst
{
	uint32_t width;
	uint32_t height;
	uint32_t sampleCount;
};

// For generating specular and diffuse maps
struct CubeFilterPushConst
{
	float roughness = 0.f;
	uint32_t outputDiffuseSampleCount = 1u;
};

// Additional customization for PBR
struct PBRPushConst
{
	float lightIntensity = 1.f;
	float baseReflectivity = 0.04f;
	float maxReflectionLod = 4.f;
	float lightFalloff = 1.0f; // Small --> slower falloff, Big --> faster falloff
	float albedoMultipler = 0.0f; // Show albedo color if the scene is too dark, default value should be zero
};


///////////////////////// UBO //////////////////////////////////

// General-purpose
struct CameraUBO
{
	alignas(16)
	glm::mat4 projection;
	alignas(16)
	glm::mat4 view;
	alignas(16)
	glm::vec4 position;
};

struct ShadowMapUBO
{
	alignas(16)
	glm::mat4 lightSpaceMatrix;
	alignas(16)
	glm::vec4 lightPosition;
	alignas(4)
	float shadowMinBias;
	alignas(4)
	float shadowMaxBias;
};

// Per model transformation matrix
struct ModelUBO
{
	alignas(16)
	glm::mat4 model;
};

struct ClusterForwardUBO
{
	alignas(16)
	glm::mat4 cameraInverseProjection;
	alignas(16)
	glm::mat4 cameraView;
	alignas(8)
	glm::vec2 screenSize;
	alignas(4)
	float sliceScaling;
	alignas(4)
	float sliceBias;
	alignas(4)
	float cameraNear;
	alignas(4)
	float cameraFar;
	alignas(4)
	uint32_t sliceCountX = ClusterForwardConfig::SliceCountX;
	alignas(4)
	uint32_t sliceCountY = ClusterForwardConfig::SliceCountY;
	alignas(4)
	uint32_t sliceCountZ = ClusterForwardConfig::SliceCountZ;

	ClusterForwardUBO(glm::mat4 invProj, glm::mat4 view, glm::vec2 screenSize, float near, float far)
		: cameraInverseProjection(invProj), cameraView(view), screenSize(screenSize), cameraNear(near), cameraFar(far)
	{
		float log2FarDivNear = std::log2(near / far);
		float log2Near = std::log2(near);

		sliceScaling = sliceCountZ / log2FarDivNear;
		sliceBias = -(sliceCountZ * log2Near / log2FarDivNear);
	}
};

// For compute-based frustum culling
struct FrustumUBO
{
	alignas(16)
	glm::vec4 planes[6];
	alignas(16)
	glm::vec4 corners[8];

	FrustumUBO(glm::mat4 proj, glm::mat4 view)
	{
		glm::mat4 projView = proj * view;
		glm::mat4 invProjView = glm::inverse(projView);
		glm::mat4 t = glm::transpose(projView);

		/*planes =
		{
			glm::vec4(t[3] + t[0]), // left
			glm::vec4(t[3] - t[0]), // right
			glm::vec4(t[3] + t[1]), // bottom
			glm::vec4(t[3] - t[1]), // top
			glm::vec4(t[3] + t[2]), // near
			glm::vec4(t[3] - t[2])  // far
		};*/

		for (int i = 0; i < 3; ++i) 
		{
			planes[i * 2] = glm::vec4(t[3] + t[i]);
			planes[i * 2 + 1] = glm::vec4(t[3] - t[i]);
		}

		glm::vec4 corners[8] =
		{
			glm::vec4(-1, -1, -1, 1), glm::vec4( 1, -1, -1, 1),
			glm::vec4( 1,  1, -1, 1), glm::vec4(-1,  1, -1, 1),
			glm::vec4(-1, -1,  1, 1), glm::vec4( 1, -1,  1, 1),
			glm::vec4( 1,  1,  1, 1), glm::vec4(-1,  1,  1, 1)
		};

		for (int i = 0; i < 8; i++)
		{
			const glm::vec4 q = invProjView * corners[i];
			this->corners[i] = q / q.w;
		}
	}
};