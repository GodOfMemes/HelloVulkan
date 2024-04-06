#include "Apps/PBRBindlessApp.hpp" // Bindless, using draw indirect, buffer device address, and descriptor indexing
#include "Apps/PBRShadowApp.hpp" // Shadow demo, using draw indirect, buffer device address, and descriptor indexing
#include "Apps/FrustumCullingApp.hpp"
#include "Apps/PBRClusterForwardApp.hpp"
#include "Apps/SkinningSample.hpp"


int main(int argc, char* argv[])
{
    try
	{
		FrustumCullingApp app;
		app.Run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
    return 0;
}