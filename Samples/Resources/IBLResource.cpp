#include "IBLResource.hpp"
#include "Pipelines/Equirect2CubePipeline.hpp"
#include "Pipelines/CubeFilterPipeline.hpp"
#include "Pipelines/BRDFLUTPipeline.hpp"
#include <Utility/Utility.hpp>

IBLResource::IBLResource(GraphicsDevice* gd, const std::string& hdrFile)
	: ResourceBase(gd),
	environmentCubemap_(gd), 
	diffuseCubemap_(gd),
	specularCubemap_(gd), 
	brdfLut_(gd)
{
	Create(hdrFile);
	SetDebugNames();
}

IBLResource::~IBLResource()
{
	Destroy();
}

void IBLResource::Create(const std::string& hdrFile)
{
	// Create a cubemap from the input HDR
	{
		Equirect2CubePipeline e2c(gd, hdrFile);
		e2c.OffscreenRender(&environmentCubemap_);
	}

	// Cube filtering
	{
		CubeFilterPipeline cubeFilter(gd,&(environmentCubemap_));
		cubeFilter.OffscreenRender(&diffuseCubemap_, CubeFilterType::Diffuse);
		cubeFilter.OffscreenRender(&specularCubemap_, CubeFilterType::Specular);
	}

	// BRDF look up table
	{
		BRDFLUTPipeline brdfLUTCompute(gd);
		brdfLUTCompute.CreateLUT(&brdfLut_);
	}

	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));
}

void IBLResource::Destroy()
{
	environmentCubemap_.Destroy();
	diffuseCubemap_.Destroy();
	specularCubemap_.Destroy();
	brdfLut_.Destroy();
}

void IBLResource::SetDebugNames()
{
	environmentCubemap_.SetDebugName("Environment_Cubemap");
	diffuseCubemap_.SetDebugName("Diffuse_Cubemap");
	specularCubemap_.SetDebugName("Specular_Cubemap");
	brdfLut_.SetDebugName("BRDF_LUT");
}