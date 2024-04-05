#pragma once

#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Texture2D.hpp>
#include <Graphics/Resource.hpp>
#include "UIData.hpp"

struct ShadowResource : ResourceBase
{
public:
	ShadowResource(GraphicsDevice* gd) : ResourceBase(gd) {}
	~ShadowResource();

	void CreateSingleShadowMap();
	void Destroy() override;

	void UpdateFromUIData(UIData& uiData)
	{
		shadowUBO_.shadowMinBias = uiData.shadowMinBias_;
		shadowUBO_.shadowMaxBias = uiData.shadowMaxBias_;
		shadowNearPlane_ = uiData.shadowNearPlane_;
		shadowFarPlane_ = uiData.shadowFarPlane_;
		orthoSize_ = uiData.shadowOrthoSize_;
	}

public:
	Texture2D shadowMap_{gd};

	ShadowMapUBO shadowUBO_;
	float shadowNearPlane_;
	float shadowFarPlane_;
	float orthoSize_;
};