#pragma once 

#include <vector>
#include <fstream>
#include <Log.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include <json/json.h>
#include <VCPFormat.hpp>

#include <ParsingUtils.hpp>
#include <spirv_reflect.h>

#include <PipelineArchetypeRecord.hpp> //for ReflectedShader

class PipelineInstanceRecord {
  public:
  	PipelineInstanceRecord();
	PipelineInstanceRecord(PipelineType type, const std::string_view& srcPath, const Json::Value& instanceNode);

	void serialize(std::ofstream& outStream) const;

	void verifyInstance(const std::string_view& srcPath, const std::vector<ReflectedShader>& shaderModules);

	bool isValid() const { return m_isValid; }

  private:
	bool m_isValid = true;

	void deserializeVertexInput(const std::string_view& srcPath, const Json::Value& node);
	void deserializeInputAssembly(const std::string_view& srcPath, const Json::Value& node);
	void deserializeRasterization(const std::string_view& srcPath, const Json::Value& node);
	void deserializeViewportScissor(const std::string_view& srcPath, const Json::Value& node);
	void deserializeMultisample(const std::string_view& srcPath, const Json::Value& node);
	void deserializeDepthStencil(const std::string_view& srcPath, const Json::Value& node);
	VkStencilOpState deserializeStencilState(const std::string_view& srcPath, const Json::Value& node);
	void deserializeDynamicState(const std::string_view& srcPath, const Json::Value& node);
	void deserializeColorBlend(const std::string_view& srcPath, const Json::Value& node);
	void deserializeColorAttachmentBlend(const std::string_view& srcPath, const Json::Value& node);
	void deserializeSpecializationConfigs(const std::string_view& srcPath, const Json::Value& node);

	void verifyVertexShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader);
	void verifyFragmentShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader);
	
	PipelineType m_type;
	PipelineInstanceData m_data;
};

template<> inline void serialize(const PipelineInstanceRecord& record, std::ofstream& outStream) {
	record.serialize(outStream);
}