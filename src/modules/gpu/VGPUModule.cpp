#include <modules\gpu\VGPUModule.hpp>

VGPUModule::VGPUModule(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule) {
	m_context.create(appName, appVersion, windowModule);
	m_windowModule = windowModule;
}

void VGPUModule::onCreate(VEngine& engine) {}

void VGPUModule::onActivate(VEngine& engine) {}

void VGPUModule::onExecute(VEngine& engine) {
	if (m_wasSwapchainInvalid || m_windowModule->wasResized()) {
		m_context.recreateSwapchain(m_windowModule);
	}

	AcquireResult result;
	if (m_wasSwapchainInvalid && m_invalidAcquiredResult.imageIndex) {
		result = m_invalidAcquiredResult;
		result.swapchainState = SwapchainState::OK; //don't fail right away
	} else {
		result = m_context.acquireImage();
	}

	if (result.swapchainState == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		m_invalidAcquiredResult.imageIndex = -1U;
		return;
	}


	SwapchainState state = m_context.presentImage(result.imageIndex, result.imageSemaphore);
	if (state == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		m_invalidAcquiredResult = result;
		return;
	}

	m_wasSwapchainInvalid = false;
}

void VGPUModule::onDeactivate(VEngine& engine) {}

void VGPUModule::onDestroy(VEngine& engine) { m_context.destroy(); }

void VGPUModule::onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {
	if (moduleToDestroy == m_windowModule)
		engine.setExitFlag();
}
