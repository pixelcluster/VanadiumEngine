#pragma once

#include <VEngine.hpp>
#include <modules/gpu/VGPUContext.hpp>
#include <modules/gpu/framegraph/VFramegraphContext.hpp>
#include <modules/gpu/VGPUResourceAllocator.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>

class VGPUModule : public VModule {
  public:
	VGPUModule(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule);

	virtual void onCreate(VEngine& engine) override;
	virtual void onActivate(VEngine& engine) override;
	virtual void onExecute(VEngine& engine) override;
	virtual void onDeactivate(VEngine& engine) override;
	virtual void onDestroy(VEngine& engine) override;

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy);

	//No more nodes can be added after the module is activated once
	VFramegraphContext& framegraphContext() { return m_framegraphContext; }

	VGPUTransferManager& transferManager() { return m_transferManager; }

  private:
	VGPUContext m_context;

	VFramegraphContext m_framegraphContext;
	VGPUResourceAllocator m_resourceAllocator;

	VWindowModule* m_windowModule;

	bool m_wasSwapchainInvalid = true;
	AcquireResult m_invalidAcquiredResult = {.imageIndex = -1U};

	VkSemaphore m_signalSemaphores[frameInFlightCount];

	VGPUTransferManager m_transferManager;

	double m_memoryBudgetTimer = 0.0f;
};