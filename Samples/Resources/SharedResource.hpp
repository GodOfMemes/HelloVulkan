#pragma once

#include "Graphics/Texture2D.hpp"
#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Resource.hpp>
#include <Graphics/Buffer.hpp>

struct SharedResource : ResourceBase
{
public:
	SharedResource(GraphicsDevice* gd);
	~SharedResource();

	void Create() override;
	void Destroy() override;

public:
	Texture2D multiSampledColorImage_{gd};
	Texture2D singleSampledColorImage_{gd};
	Texture2D depthImage_{gd};
};