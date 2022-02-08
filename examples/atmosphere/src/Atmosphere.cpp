#include <VEngine.hpp>
#include <modules/gpu/VGPUModule.hpp>
#include <modules/window/VGLFWWindowModule.hpp>

#include <DataGeneratorModule.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>

int main() {
	VEngine engine;
	VWindowModule* windowModule = engine.createModule<VGLFWWindowModule>(1280, 720, "Planet renderer");
	VGPUModule* gpuModule = engine.createModule<VGPUModule>("Planet renderer", 0, windowModule);
	DataGeneratorModule* bufferModule = engine.createModule<DataGeneratorModule>(gpuModule, windowModule);

	gpuModule->framegraphContext().appendNode<PlanetRenderNode>(
		bufferModule->vertexBufferHandle(),
		bufferModule->indexBufferHandle(),
		gpuModule->transferManager().dstBufferHandle(bufferModule->uboTransferHandle()), bufferModule->textureHandle(),
		totalIndexCount);

	engine.activateModule(windowModule);
	engine.activateModule(gpuModule);
	engine.activateModule(bufferModule);

	engine.addModuleDependency(windowModule, bufferModule);
	engine.addModuleDependency(windowModule, gpuModule);
	engine.addModuleDependency(bufferModule, gpuModule);
	engine.run();
}