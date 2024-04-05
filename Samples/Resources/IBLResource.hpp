#pragma once

#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/Texture2D.hpp"
#include <Graphics/Resource.hpp>
#include <Graphics/Buffer.hpp>

struct IBLResource : ResourceBase
{
public:
	IBLResource(GraphicsDevice* ctx, const std::string& hdrFile);
	~IBLResource();
	void Destroy() override;

private:
	void Create(const std::string& hdrFile);
	void SetDebugNames();

public:
	float cubemapMipmapCount_;
	Texture2D environmentCubemap_;
	Texture2D diffuseCubemap_;
	Texture2D specularCubemap_;
	Texture2D brdfLut_;

};