#pragma once

#include <modules/gpu/VGPUModule.hpp>

class VertexBufferUpdateModule : public VModule {
  public:
	VertexBufferUpdateModule(VGPUModule* gpuModule, VWindowModule* windowModule);

	virtual void onCreate(VEngine& engine) override;
	virtual void onActivate(VEngine& engine) override;
	virtual void onExecute(VEngine& engine) override;
	virtual void onDeactivate(VEngine& engine) override;
	virtual void onDestroy(VEngine& engine) override;

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) override;

	BufferResourceHandle vertexBufferHandle() const {
		return m_gpuModule->transferManager().dstBufferHandle(m_vertexDataTransfer);
	}
  private:
	VGPUModule* m_gpuModule;
	VWindowModule* m_windowModule;

	double m_timeCounter = 0.0f;

	GPUTransferHandle m_vertexDataTransfer;
};