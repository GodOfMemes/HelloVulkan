#include "ShadowResource.hpp"

ShadowResource::~ShadowResource()
{
	Destroy();
}

void ShadowResource::Destroy()
{
	shadowMap_.Destroy();
}

void ShadowResource::CreateSingleShadowMap()
{
	// Init shadow map
	shadowMap_.CreateDepthResources(
		ShadowConfig::DepthSize,
		ShadowConfig::DepthSize,
		1u,// layerCount
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT);
	shadowMap_.CreateDefaultSampler(
		0.f,
		1.f,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	shadowMap_.SetDebugName("Single_Layer_Shadow_Map");
}