#pragma once

#include <VModule.hpp>
#include <modules/gpu/VGPUModule.hpp>
#include <modules/window/VWindowModule.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>
#include <glm/glm.hpp>
#include <numbers>

constexpr size_t pointsPerLatitudeSegment = 128;
constexpr size_t pointsPerLongitudeSegment = 128;
constexpr size_t individualPointsPerLongitudeSegment = pointsPerLongitudeSegment + 1;
constexpr size_t totalPointCount = pointsPerLatitudeSegment * individualPointsPerLongitudeSegment +
								   2; // add 2 singularity points at the top of the sphere and at the bottom
constexpr size_t totalIndexCount = individualPointsPerLongitudeSegment * 6 * pointsPerLongitudeSegment;

struct CameraSceneData {
	glm::mat4 viewProjection;
};

struct VertexData {
	glm::vec3 pos;
	glm::vec2 texCoord;
};

class DataGeneratorModule : public VModule {
  public:
	DataGeneratorModule(VGPUModule* gpuModule, VWindowModule* windowModule);

	void onCreate(VEngine& engine);
	void onActivate(VEngine& engine);

	void onExecute(VEngine& engine);

	void onDeactivate(VEngine& engine);
	void onDestroy(VEngine& engine);

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {}

	VBufferResourceHandle vertexBufferHandle() { return m_vertexBufferHandle; }
	VBufferResourceHandle indexBufferHandle() { return m_indexBufferHandle; }
	VGPUTransferHandle uboTransferHandle() { return m_sceneDataTransfer; }
	VImageResourceHandle textureHandle() { return m_texHandle; }

  private:
	VGPUModule* m_gpuModule;
	VWindowModule* m_windowModule;

	glm::vec3 m_camPos = glm::vec3(0.0f, 0.0f, -5.0f);
	float m_camPhi = -0.5 * std::numbers::pi_v<float>;
	float m_camTheta = 0.5 * std::numbers::pi_v<float>;

	float m_lastMouseX;
	float m_lastMouseY;
	bool m_lastMouseValid = false;

	VertexData* m_pointBuffer;
	uint32_t* m_indexBuffer;
	VBufferResourceHandle m_vertexBufferHandle;
	VBufferResourceHandle m_indexBufferHandle;
	VImageResourceHandle m_texHandle;
	VGPUTransferHandle m_sceneDataTransfer;
};