#pragma once

#include <VModule.hpp>
#include <modules/gpu/VGPUModule.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>

constexpr size_t pointsPerLatitudeSegment = 256;
constexpr size_t pointsPerLongitudeSegment = 256;
constexpr size_t totalPointCount = pointsPerLatitudeSegment * pointsPerLongitudeSegment + 2; //add 2 singularity points at the top of the sphere and at the bottom
constexpr size_t totalIndexCount = pointsPerLongitudeSegment * 2 * (pointsPerLatitudeSegment + 1);

struct Vector3 {
	float x, y, z;
	Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

class DataGeneratorModule : public VModule {
  public:
	DataGeneratorModule(VGPUModule* gpuModule);

	void onCreate(VEngine& engine);
	void onActivate(VEngine& engine);

	void onExecute(VEngine& engine);

	void onDeactivate(VEngine& engine);
	void onDestroy(VEngine& engine);

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {}

	VBufferResourceHandle vertexBufferHandle() { return m_vertexBufferHandle; }

  private:
	VGPUModule* m_gpuModule;

	Vector3* m_pointBuffer;
	uint32_t* m_indexBuffer;
	VBufferResourceHandle m_vertexBufferHandle;
	VBufferResourceHandle m_indexBufferHandle;
};