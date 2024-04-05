#pragma once

#include "Graphics/Texture2D.hpp"
#include <Graphics/GraphicsDevice.hpp>
#include <Graphics/Pipeline.hpp>

/*
Compute pipeline to generate lookup table
*/
class BRDFLUTPipeline : PipelineBase
{
public:
	BRDFLUTPipeline(GraphicsDevice* gd);
	~BRDFLUTPipeline();

	void CreateLUT(Texture2D* outputLUT);
	void Execute();

private:
	Buffer outBuffer_{gd}; // This is the lookup table which has to be transferred to an image
	VkDescriptorSet descriptorSet_;

	void CreateDescriptor();
};