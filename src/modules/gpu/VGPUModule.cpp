#include <modules\gpu\VGPUModule.hpp>

VGPUModule::VGPUModule(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule) {
	m_context.create(appName, appVersion, windowModule);
	m_windowModule = windowModule;
}

void VGPUModule::onCreate(VEngine& engine) {}

void VGPUModule::onActivate(VEngine& engine) {}

void VGPUModule::onExecute(VEngine& engine) {
	AcquireResult result = m_context.acquireImage();

	if (result.swapchainState == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		return;
	}


	SwapchainState state = m_context.presentImage(result.imageIndex, result.imageSemaphore);
	if (state == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
	}
}

void VGPUModule::onDeactivate(VEngine& engine) {}

void VGPUModule::onDestroy(VEngine& engine) { m_context.destroy(); }

void VGPUModule::onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {
	if (moduleToDestroy == m_windowModule)
		engine.setExitFlag();
}
