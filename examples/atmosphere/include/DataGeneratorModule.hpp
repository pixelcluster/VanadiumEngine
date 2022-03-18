#pragma once

#include <VModule.hpp>
#include <modules/gpu/VGPUModule.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <modules/window/VWindowModule.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <glm/glm.hpp>
#include <numbers>

constexpr size_t pointsPerLatitudeSegment = 256;
constexpr size_t pointsPerLongitudeSegment = 256;
constexpr size_t individualPointsPerLongitudeSegment = pointsPerLongitudeSegment + 1;
constexpr size_t totalPointCount = pointsPerLatitudeSegment * individualPointsPerLongitudeSegment +
								   2; // add 2 singularity points at the top of the sphere and at the bottom
constexpr size_t totalIndexCount = individualPointsPerLongitudeSegment * 6 * pointsPerLongitudeSegment;

struct CameraSceneData {
	glm::mat4 viewProjection;
	glm::vec4 camPos;
};

struct VertexData {
	glm::vec3 pos;
	glm::vec2 texCoord;
};

class DataGeneratorModule : public VModule {
  public:
	DataGeneratorModule(VGPUModule* gpuModule, PlanetRenderNode* renderNode, VWindowModule* windowModule);

	void onCreate(VEngine& engine);
	void onActivate(VEngine& engine);

	void onExecute(VEngine& engine);

	void onDeactivate(VEngine& engine);
	void onDestroy(VEngine& engine);

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {}

  private:
	VGPUModule* m_gpuModule;
	PlanetRenderNode* m_renderNode;
	VWindowModule* m_windowModule;

	glm::vec3 m_camPos = glm::vec3(0.0f, 0.0f, -5.0f);
	float m_camPhi = -0.5 * std::numbers::pi_v<float>;
	float m_camTheta = 0.5 * std::numbers::pi_v<float>;

	float m_lastMouseX;
	float m_lastMouseY;
	bool m_lastMouseValid = false;

	VertexData* m_pointBuffer;
	uint32_t* m_indexBuffer;

	BufferResourceHandle m_vertexBufferHandle;
	BufferResourceHandle m_indexBufferHandle;
	ImageResourceHandle m_texHandle;
	ImageResourceHandle m_seaMaskTexHandle;
	GPUTransferHandle m_sceneDataTransfer;

	VkDescriptorSetLayout m_sceneDataSetLayout;
	VkDescriptorSetLayout m_textureSetLayout;
	VkSampler m_textureSampler;
};