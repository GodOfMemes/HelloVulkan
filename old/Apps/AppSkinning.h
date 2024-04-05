#ifndef APP_SKINNING
#define APP_SKINNING

#include "AppBase.h"
#include "Scene/Scene.h"
#include "Resources/ResourcesLight.h"
#include "Resources/ResourcesShadow.h"
#include "Pipelines/PipelineImGui.h"
#include "Pipelines/PipelineShadow.h"
#include "Pipelines/PipelinePBRShadow.h"
#include "Pipelines/PipelineLightRender.h"
#include "Pipelines/PipelineInfiniteGrid.h"

#include <memory>

class AppSkinning final : AppBase
{
public:
	AppSkinning();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitScene();
	void InitLights();

private:
	PipelineImGui* imguiPtr_ = nullptr;
	PipelineShadow* shadowPtr_ = nullptr;
	PipelinePBRShadow* pbrOpaquePtr_ = nullptr;
	PipelinePBRShadow* pbrTransparentPtr_ = nullptr;
	PipelineLightRender* lightPtr_ = nullptr;

	std::unique_ptr<Scene> scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
	ResourcesShadow* resourcesShadow_ = nullptr;
};

#endif