#include <VEngine.hpp>
#include <modules/gpu/VGPUModule.hpp>
#include <modules/window/VGLFWWindowModule.hpp>

#include <DataGeneratorModule.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <framegraph_nodes/ScatteringLUTNode.hpp>

int main() {
	VEngine engine;
	VWindowModule* windowModule = engine.createModule<VGLFWWindowModule>(1280, 720, "Planet renderer");
	VGPUModule* gpuModule = engine.createModule<VGPUModule>("Planet renderer", 0, windowModule);

	ScatteringLUTNode* lutNode = gpuModule->framegraphContext().appendNode<ScatteringLUTNode>();
	PlanetRenderNode* renderNode = gpuModule->framegraphContext().appendNode<PlanetRenderNode>();

	DataGeneratorModule* bufferModule = engine.createModule<DataGeneratorModule>(gpuModule, renderNode, windowModule);

	engine.activateModule(windowModule);
	engine.activateModule(gpuModule);
	engine.activateModule(bufferModule);

	engine.addModuleDependency(windowModule, bufferModule);
	engine.addModuleDependency(windowModule, gpuModule);
	engine.addModuleDependency(bufferModule, gpuModule);
	engine.run();
}