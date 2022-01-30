#pragma once

#include <VEngine.hpp>
#include <modules/gpu/VGPUContext.hpp>

class VGPUModule : public VModule {
  public:
	VGPUModule(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule);

	virtual void onCreate(VEngine& engine) override;
	virtual void onActivate(VEngine& engine) override;
	virtual void onExecute(VEngine& engine) override;
	virtual void onDeactivate(VEngine& engine) override;
	virtual void onDestroy(VEngine& engine) override;

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy);

  private:
	VGPUContext m_context;
	VWindowModule* m_windowModule;

	bool m_wasSwapchainInvalid = false;
	AcquireResult m_invalidAcquiredResult = {.imageIndex = -1U};

	VkSemaphore m_signalSemaphores[frameInFlightCount];
};