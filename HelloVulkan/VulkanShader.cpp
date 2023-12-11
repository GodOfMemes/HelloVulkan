#include "VulkanShader.h"

VkResult VulkanShader::Create(VkDevice device, const char* fileName)
{
	if (CompileShaderFile(fileName) < 1)
		return VK_NOT_READY;

	const VkShaderModuleCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = SPIRV.size() * sizeof(unsigned int),
		.pCode = SPIRV.data(),
	};

	return vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
}

void VulkanShader::Destroy(VkDevice device)
{
	vkDestroyShaderModule(device, shaderModule, nullptr);
}

VkPipelineShaderStageCreateInfo VulkanShader::GetShaderStageInfo(
	VkShaderStageFlagBits shaderStage,
	const char* entryPoint)
{
	return VkPipelineShaderStageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStage,
		.module = shaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = nullptr
	};
}

size_t VulkanShader::CompileShaderFile(const char* file)
{
	if (auto shaderSource = ReadShaderFile(file); !shaderSource.empty())
		return CompileShader(GLSLangShaderStageFromFileName(file), shaderSource.c_str());

	return 0;
}

std::string VulkanShader::ReadShaderFile(const char* fileName)
{
	FILE* file = fopen(fileName, "r");

	if (!file)
	{
		printf("I/O error. Cannot open shader file '%s'\n", fileName);
		return std::string();
	}

	fseek(file, 0L, SEEK_END);
	const auto bytesinfile = ftell(file);
	fseek(file, 0L, SEEK_SET);

	char* buffer = (char*)alloca(bytesinfile + 1);
	const size_t bytesread = fread(buffer, 1, bytesinfile, file);
	fclose(file);

	buffer[bytesread] = 0;

	static constexpr unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };

	if (bytesread > 3)
	{
		if (!memcmp(buffer, BOM, 3))
			memset(buffer, ' ', 3);
	}

	std::string code(buffer);

	while (code.find("#include ") != code.npos)
	{
		const auto pos = code.find("#include ");
		const auto p1 = code.find('<', pos);
		const auto p2 = code.find('>', pos);
		if (p1 == code.npos || p2 == code.npos || p2 <= p1)
		{
			printf("Error while loading shader program: %s\n", code.c_str());
			return std::string();
		}
		const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
		const std::string include = ReadShaderFile(name.c_str());
		code.replace(pos, p2 - pos + 1, include.c_str());
	}

	return code;
}

size_t VulkanShader::CompileShader(glslang_stage_t stage, const char* shaderSource)
{
	const glslang_input_t input =
	{
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_1,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_3,
		.code = shaderSource,
		.default_version = 100,
		.default_profile = GLSLANG_NO_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = glslang_default_resource(),
	};

	glslang_shader_t* shader = glslang_shader_create(&input);

	if (!glslang_shader_preprocess(shader, &input))
	{
		fprintf(stderr, "GLSL preprocessing failed\n");
		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
		PrintShaderSource(input.code);
		return 0;
	}

	if (!glslang_shader_parse(shader, &input))
	{
		fprintf(stderr, "GLSL parsing failed\n");
		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
		PrintShaderSource(glslang_shader_get_preprocessed_code(shader));
		return 0;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);

	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
	{
		fprintf(stderr, "GLSL linking failed\n");
		fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
		fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
		return 0;
	}

	glslang_program_SPIRV_generate(program, stage);

	SPIRV.resize(glslang_program_SPIRV_get_size(program));
	glslang_program_SPIRV_get(program, SPIRV.data());

	{
		const char* spirv_messages =
			glslang_program_SPIRV_get_messages(program);

		if (spirv_messages)
			fprintf(stderr, "%s", spirv_messages);
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return SPIRV.size();
}

void VulkanShader::PrintShaderSource(const char* text)
{
	int line = 1;

	printf("\n(%3i) ", line);

	while (text && *text++)
	{
		if (*text == '\n')
		{
			printf("\n(%3i) ", ++line);
		}
		else if (*text == '\r')
		{
		}
		else
		{
			printf("%c", *text);
		}
	}

	printf("\n");
}

VkShaderStageFlagBits GLSLangShaderStageToVulkan(glslang_stage_t sh)
{
	switch (sh)
	{
	case GLSLANG_STAGE_VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case GLSLANG_STAGE_FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case GLSLANG_STAGE_GEOMETRY:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case GLSLANG_STAGE_TESSCONTROL:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case GLSLANG_STAGE_TESSEVALUATION:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case GLSLANG_STAGE_COMPUTE:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}

	return VK_SHADER_STAGE_VERTEX_BIT;
}

glslang_stage_t GLSLangShaderStageFromFileName(const char* fileName)
{
	if (EndsWith(fileName, ".vert"))
		return GLSLANG_STAGE_VERTEX;

	if (EndsWith(fileName, ".frag"))
		return GLSLANG_STAGE_FRAGMENT;

	if (EndsWith(fileName, ".geom"))
		return GLSLANG_STAGE_GEOMETRY;

	if (EndsWith(fileName, ".comp"))
		return GLSLANG_STAGE_COMPUTE;

	if (EndsWith(fileName, ".tesc"))
		return GLSLANG_STAGE_TESSCONTROL;

	if (EndsWith(fileName, ".tese"))
		return GLSLANG_STAGE_TESSEVALUATION;

	return GLSLANG_STAGE_VERTEX;
}

inline int EndsWith(const char* s, const char* part)
{
	return (strstr(s, part) - s) == (strlen(s) - strlen(part));
}