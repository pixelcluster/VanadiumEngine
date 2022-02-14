#pragma once

#include <modules/gpu/VGPUModule.hpp>

class ScatteringLUTPrecomputer {
  public:
	ScatteringLUTPrecomputer(VGPUModule* gpuModule);

	VImageResourceHandle lutImageHandle();
  private:
	VImageResourceHandle m_lutImageHandle;
};