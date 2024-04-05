#include "SharedResource.hpp"
#include <Graphics/SwapChain.hpp>

SharedResource::SharedResource(GraphicsDevice* gd)
	: ResourceBase(gd,true)
{
}

SharedResource::~SharedResource()
{
	Destroy();
}

void SharedResource::Create()
{
	depthImage_.Destroy();
	multiSampledColorImage_.Destroy();
	singleSampledColorImage_.Destroy();

	VkSampleCountFlagBits msaaSamples = gd->GetSampleCount();
	auto extent = gd->GetSwapChain()->GetExtent();

	// Depth attachment (OnScreen and offscreen)
	depthImage_.CreateDepthResources(
		extent.width,
		extent.height,
		1u, // layerCount
		msaaSamples);
	depthImage_.SetDebugName("Depth_Image");

	// Color attachments
	// Multi-sampled (MSAA)
	multiSampledColorImage_.CreateColorResources(
		extent.width,
		extent.height,
		msaaSamples);
	multiSampledColorImage_.SetDebugName("Multisampled_Color_Image");

	// Single-sampled
	singleSampledColorImage_.CreateColorResources(
		extent.width,
		extent.height);
	singleSampledColorImage_.SetDebugName("Singlesampled_Color_Image");
}

void SharedResource::Destroy()
{
	depthImage_.Destroy();
	multiSampledColorImage_.Destroy();
	singleSampledColorImage_.Destroy();
}