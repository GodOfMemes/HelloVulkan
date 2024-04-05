#include "Apps/PBRShadowApp.hpp"

int main(int argc, char* argv[])
{
    try
	{
		PBRShadowApp app;
		app.Run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
    return 0;
}