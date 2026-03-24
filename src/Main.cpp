#include "Application.hpp"

#ifdef _WIN32
// Force systems with hybrid graphics to use discrete GPU
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main() {
	Application app;
	app.run();
}