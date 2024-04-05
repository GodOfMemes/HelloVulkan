#pragma once


#ifdef _NDEBUG
#define RELEASE
#else
#define DEBUG
#endif

#define MODEL_DIR std::string("Assets/Models/")
#define TEXTURE_DIR std::string("Assets/Textures/")
#define SHADER_DIR std::string("Assets/Shaders/")
#define FONT_DIR std::string("Assets/Fonts/")