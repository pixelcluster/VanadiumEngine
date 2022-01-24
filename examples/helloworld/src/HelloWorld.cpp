#include <modules/gpu/VGPUModule.hpp>
#include <modules/window/VGLFWWindowModule.hpp>

int main() {
	VEngine engine;
	VWindowModule* windowModule = engine.createModule<VGLFWWindowModule>(640, 480, "Vanadium Hello World");
	VGPUModule* gpuModule = engine.createModule<VGPUModule>("Vanadium Hello World", 0, windowModule);

	engine.activateModule(windowModule);
	engine.activateModule(gpuModule);

	engine.addModuleDependency(windowModule, gpuModule);

	engine.run();
}