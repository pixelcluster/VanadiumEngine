#include <EnumMatchTable.hpp>
#include <ParsingUtils.hpp>
#include <PipelineInstanceRecord.hpp>
#include <cfloat>
#include <iostream>
#include <algorithm>

PipelineInstanceRecord::PipelineInstanceRecord(PipelineType type, const std::string_view& srcPath,
											   const Json::Value& instanceNode)
	: m_type(type) {
	m_name = asStringOr(instanceNode, "name", "");
	if (type == PipelineType::Graphics) {
		deserializeVertexInput(srcPath, instanceNode["vertex-input"]);
		deserializeInputAssembly(srcPath, instanceNode["input-assembly"]);
		deserializeRasterization(srcPath, instanceNode["rasterization"]);
		deserializeViewportScissor(srcPath, instanceNode["viewport-scissor"]);
		deserializeMultisample(srcPath, instanceNode["multisample"]);
		deserializeDepthStencil(srcPath, instanceNode["depth-stencil"]);
		deserializeDynamicState(srcPath, instanceNode["dynamic-states"]);
		deserializeColorBlend(srcPath, instanceNode["color-blend"]);
		deserializeColorAttachmentBlend(srcPath, instanceNode["attachment-blend"]);
	}
	deserializeSpecializationConfigs(srcPath, instanceNode["specialization"]);
}

void PipelineInstanceRecord::verifyInstance(const std::string_view& srcPath,
											const vanadium::SimpleVector<ReflectedShader>& shaderModules) {
	for (auto& shader : shaderModules) {
		if (shader.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			verifyVertexShader(srcPath, shader.shader);
		} else if (shader.stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
			verifyFragmentShader(srcPath, shader.shader);
		}
	}
}

void PipelineInstanceRecord::verifyVertexShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader) {
	SpvReflectResult result;
	uint32_t inputVariableCount;
	result = spvReflectEnumerateInputVariables(&shader, &inputVariableCount, nullptr);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cout << srcPath << ": Error: Internal error when verifying vertex shader.\n";
		return;
	}

	auto inputVariables = vanadium::SimpleVector<SpvReflectInterfaceVariable*>(inputVariableCount);

	result = spvReflectEnumerateInputVariables(&shader, &inputVariableCount, inputVariables.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cout << srcPath << ": Error: Internal error when verifying vertex shader.\n";
		return;
	}

	for (auto& variable : inputVariables) {
		auto var =
			std::find_if(m_instanceVertexInputConfig.attributes.begin(), m_instanceVertexInputConfig.attributes.end(),
						 [variable](const auto& attrib) { return attrib.location == variable->location; });
		if (var == m_instanceVertexInputConfig.attributes.end() && variable->location != ~0U) {
			std::cout << srcPath << ": Error: Unbound vertex attribute at location " << variable->location << ".\n";
			m_isValid = false;
		}
	}
}

void PipelineInstanceRecord::verifyFragmentShader(const std::string_view& srcPath,
												  const SpvReflectShaderModule& shader) {
	SpvReflectResult result;
	uint32_t outputVariableCount;
	result = spvReflectEnumerateOutputVariables(&shader, &outputVariableCount, nullptr);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cout << srcPath << ": Error: Internal error when verifying fragment shader.\n";
		return;
	}

	auto outputVariables = vanadium::SimpleVector<SpvReflectInterfaceVariable*>(outputVariableCount);

	result = spvReflectEnumerateOutputVariables(&shader, &outputVariableCount, outputVariables.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cout << srcPath << ": Error: Internal error when verifying fragment shader.\n";
		return;
	}

	for (auto& variable : outputVariables) {
		if (variable->location >= m_instanceColorAttachmentBlendConfigs.size()) {
			std::cout << srcPath << ": Error: Unbound color attachment output at location " << variable->location
					  << ".\n";
			m_isValid = false;
		}
	}
}

size_t PipelineInstanceRecord::serializedSize() const {
	size_t totalSize = 0;

	// packed sizes of pipeline create structs

	constexpr size_t inputAttribSize = sizeof(uint32_t) * 4;
	constexpr size_t inputBindingSize = sizeof(uint32_t) * 3;
	constexpr size_t inputAssemblySize = sizeof(uint32_t) + sizeof(bool);
	constexpr size_t rasterSize = sizeof(bool) * 3 + sizeof(uint32_t) * 3 + sizeof(float) * 4;
	constexpr size_t viewportSize = 6 * sizeof(float);
	constexpr size_t scissorSize = 2 * sizeof(int32_t) + 2 * sizeof(uint32_t);
	constexpr size_t multisampleSize = sizeof(uint32_t);
	constexpr size_t stencilStateSize = sizeof(uint32_t) * 7;
	constexpr size_t depthStencilSize = sizeof(bool) * 4 + sizeof(uint32_t) + sizeof(float) * 2 + stencilStateSize * 2;
	constexpr size_t dynamicStateSize = sizeof(uint32_t);
	constexpr size_t colorBlendSize = sizeof(bool) + sizeof(uint32_t) + sizeof(float) * 4;
	constexpr size_t colorAttachmentSize = 7 * sizeof(uint32_t) + sizeof(bool);
	constexpr size_t specializationMapEntrySize = 3 * sizeof(uint32_t);

	totalSize += sizeof(uint32_t);
	totalSize += m_name.size();
	if (m_type == PipelineType::Graphics) {
		totalSize += sizeof(uint32_t);
		totalSize += m_instanceVertexInputConfig.attributes.size() * inputAttribSize;
		totalSize += sizeof(uint32_t);
		totalSize += m_instanceVertexInputConfig.bindings.size() * inputBindingSize;
		totalSize += inputAssemblySize;
		totalSize += rasterSize;
		totalSize += sizeof(uint32_t);
		totalSize += m_instanceViewportScissorConfig.viewports.size() * viewportSize;
		totalSize += sizeof(uint32_t);
		totalSize += m_instanceViewportScissorConfig.scissorRects.size() * scissorSize;
		totalSize += multisampleSize;
		totalSize += depthStencilSize;
		totalSize += colorBlendSize;
		totalSize += sizeof(uint32_t);
		totalSize += m_instanceDynamicStateConfig.dynamicStates.size() * dynamicStateSize;
		totalSize += sizeof(uint32_t);
		totalSize += m_instanceColorAttachmentBlendConfigs.size() * colorAttachmentSize;
	}
	totalSize += sizeof(uint32_t);
	for (auto& config : m_instanceSpecializationConfigs) {
		totalSize += sizeof(uint32_t);
		totalSize += config.configs.size() * specializationMapEntrySize;
		totalSize += sizeof(uint32_t);
		totalSize += config.specializationDataSize;
	}

	return totalSize;
}

void PipelineInstanceRecord::serialize(void* data) {
	uint32_t nameSize = static_cast<uint32_t>(m_name.size());
	std::memcpy(data, &nameSize, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, m_name.c_str(), m_name.size());
	data = offsetVoidPtr(data, m_name.size());

	if (m_type == PipelineType::Graphics) {
		uint32_t attribCount = static_cast<uint32_t>(m_instanceVertexInputConfig.attributes.size());
		std::memcpy(data, &attribCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		for (auto& attrib : m_instanceVertexInputConfig.attributes) {
			std::memcpy(data, &attrib.location, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attrib.binding, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attrib.format, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attrib.offset, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
		}

		uint32_t bindingCount = static_cast<uint32_t>(m_instanceVertexInputConfig.bindings.size());
		std::memcpy(data, &bindingCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		for (auto& binding : m_instanceVertexInputConfig.bindings) {
			std::memcpy(data, &binding.binding, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &binding.stride, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &binding.inputRate, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
		}

		std::memcpy(data, &m_instanceInputAssemblyConfig.topology, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &m_instanceInputAssemblyConfig.primitiveRestart, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));

		std::memcpy(data, &m_instanceRasterizationConfig.depthClampEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceRasterizationConfig.rasterizerDiscardEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceRasterizationConfig.polygonMode, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &m_instanceRasterizationConfig.cullMode, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &m_instanceRasterizationConfig.frontFace, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &m_instanceRasterizationConfig.depthBiasEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceRasterizationConfig.depthBiasConstantFactor, sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceRasterizationConfig.depthBiasClamp, sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceRasterizationConfig.depthBiasSlopeFactor, sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceRasterizationConfig.lineWidth, sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));

		uint32_t viewportCount = m_instanceViewportScissorConfig.viewports.size();
		std::memcpy(data, &viewportCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		for (auto& viewport : m_instanceViewportScissorConfig.viewports) {
			std::memcpy(data, &viewport.x, sizeof(float));
			data = offsetVoidPtr(data, sizeof(float));
			std::memcpy(data, &viewport.y, sizeof(float));
			data = offsetVoidPtr(data, sizeof(float));
			std::memcpy(data, &viewport.width, sizeof(float));
			data = offsetVoidPtr(data, sizeof(float));
			std::memcpy(data, &viewport.height, sizeof(float));
			data = offsetVoidPtr(data, sizeof(float));
			std::memcpy(data, &viewport.minDepth, sizeof(float));
			data = offsetVoidPtr(data, sizeof(float));
			std::memcpy(data, &viewport.maxDepth, sizeof(float));
			data = offsetVoidPtr(data, sizeof(float));
		}

		uint32_t scissorCount = m_instanceViewportScissorConfig.scissorRects.size();
		std::memcpy(data, &scissorCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		for (auto& scissor : m_instanceViewportScissorConfig.scissorRects) {
			std::memcpy(data, &scissor.offset.x, sizeof(int32_t));
			data = offsetVoidPtr(data, sizeof(int32_t));
			std::memcpy(data, &scissor.offset.y, sizeof(int32_t));
			data = offsetVoidPtr(data, sizeof(int32_t));
			std::memcpy(data, &scissor.extent.width, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &scissor.extent.height, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
		}

		std::memcpy(data, &m_instanceMultisampleConfig, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		std::memcpy(data, &m_instanceDepthStencilConfig.depthTestEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceDepthStencilConfig.depthWriteEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceDepthStencilConfig.depthCompareOp, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &m_instanceDepthStencilConfig.depthBoundsTestEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceDepthStencilConfig.stencilTestEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		data = serializeStencilState(m_instanceDepthStencilConfig.front, data);
		data = serializeStencilState(m_instanceDepthStencilConfig.back, data);
		std::memcpy(data, &m_instanceDepthStencilConfig.minDepthBounds, sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceDepthStencilConfig.maxDepthBounds, sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));

		uint32_t dynamicStateCount = m_instanceDynamicStateConfig.dynamicStates.size();
		std::memcpy(data, &dynamicStateCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		for (auto& state : m_instanceDynamicStateConfig.dynamicStates) {
			std::memcpy(data, &state, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
		}

		std::memcpy(data, &m_instanceColorBlendConfig.logicOpEnable, sizeof(bool));
		data = offsetVoidPtr(data, sizeof(bool));
		std::memcpy(data, &m_instanceColorBlendConfig.logicOp, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &m_instanceColorBlendConfig.blendConstants[0], sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceColorBlendConfig.blendConstants[1], sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceColorBlendConfig.blendConstants[2], sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));
		std::memcpy(data, &m_instanceColorBlendConfig.blendConstants[3], sizeof(float));
		data = offsetVoidPtr(data, sizeof(float));

		uint32_t attachmentCount = static_cast<uint32_t>(m_instanceColorAttachmentBlendConfigs.size());
		std::memcpy(data, &attachmentCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		for (auto& attachment : m_instanceColorAttachmentBlendConfigs) {
			bool blendEnable = attachment.blendEnable;
			std::memcpy(data, &blendEnable, sizeof(bool));
			data = offsetVoidPtr(data, sizeof(bool));
			std::memcpy(data, &attachment.srcColorBlendFactor, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attachment.dstColorBlendFactor, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attachment.colorBlendOp, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attachment.srcAlphaBlendFactor, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attachment.dstAlphaBlendFactor, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attachment.alphaBlendOp, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &attachment.colorWriteMask, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
		}
	}

	uint32_t stageConfigCount = m_instanceSpecializationConfigs.size();
	std::memcpy(data, &stageConfigCount, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));

	for (auto& stageConfig : m_instanceSpecializationConfigs) {
		uint32_t specializationConfigCount = stageConfig.configs.size();
		std::memcpy(data, &specializationConfigCount, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		for (auto& config : stageConfig.configs) {
			std::memcpy(data, &config.mapEntry.constantID, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &config.mapEntry.offset, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
			std::memcpy(data, &config.mapEntry.size, sizeof(uint32_t));
			data = offsetVoidPtr(data, sizeof(uint32_t));
		}

		uint32_t specializationDataSize = stageConfig.specializationDataSize;
		std::memcpy(data, &specializationDataSize, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		for (auto& config : stageConfig.configs) {
			switch (config.data.index()) {
				case 1:
					std::memcpy(data, &std::get<bool>(config.data), sizeof(bool));
					data = offsetVoidPtr(data, sizeof(bool));
					break;
				case 2:
					std::memcpy(data, &std::get<uint32_t>(config.data), sizeof(uint32_t));
					data = offsetVoidPtr(data, sizeof(uint32_t));
					break;
				case 3:
					std::memcpy(data, &std::get<float>(config.data), sizeof(float));
					data = offsetVoidPtr(data, sizeof(float));
					break;
				default:
					break;
			}
		}
	}
}

void* PipelineInstanceRecord::serializeStencilState(const VkStencilOpState& state, void* data) {
	std::memcpy(data, &state.failOp, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, &state.passOp, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, &state.depthFailOp, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, &state.compareOp, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, &state.compareMask, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, &state.writeMask, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	std::memcpy(data, &state.reference, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	return data;
}

void PipelineInstanceRecord::deserializeVertexInput(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid vertex input structure value.\n";
		m_isValid = false;
		return;
	}

	auto& attribNode = config["attributes"];
	auto& bindingNode = config["bindings"];

	m_instanceVertexInputConfig.attributes.reserve(attribNode.size());
	m_instanceVertexInputConfig.bindings.reserve(bindingNode.size());

	for (auto& attrib : attribNode) {
		VkFormat format = VkFormatFromString(asCStringOr(attrib, "format", "VK_FORMAT_R32G32B32A32_SFLOAT"));

		if (format == static_cast<VkFormat>(~0U)) {
			std::cout << srcPath << ": Warning: Invalid Format! Choosing R32G32B32A32_SFLOAT, might cause errors...\n";
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		VkVertexInputAttributeDescription attribDescription = { .location = asUIntOr(attrib, "location", 0),
																.binding = asUIntOr(attrib, "binding", 0),
																.format = format,
																.offset = asUIntOr(attrib, "offset", 0) };
		if (attribDescription.binding >= bindingNode.size()) {
			std::cout << srcPath << ": Error: Invalid binding index for attribute at location "
					  << attribDescription.location << ".\n";
			m_isValid = false;
		}
		m_instanceVertexInputConfig.attributes.push_back(attribDescription);
	}

	for (auto& binding : bindingNode) {
		VkVertexInputRate inputRate =
			VkVertexInputRateFromString(asCStringOr(binding, "input-rate", "VK_VERTEX_INPUT_RATE_VERTEX"));

		if (inputRate == static_cast<VkVertexInputRate>(~0U)) {
			std::cout << srcPath
					  << ": Warning: Invalid Input Rate! Choosing VERTEX, might cause unintended behaviour...\n";
			inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		VkVertexInputBindingDescription bindingDescription = { .binding = asUIntOr(binding, "binding", 0),
															   .stride = asUIntOr(binding, "stride", 0),
															   .inputRate = inputRate };
		m_instanceVertexInputConfig.bindings.push_back(bindingDescription);
	}
}

void PipelineInstanceRecord::deserializeInputAssembly(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid input assembly structure value.\n";
		m_isValid = false;
		return;
	}

	auto topology = VkPrimitiveTopologyFromString(config["topology"].asCString());

	if (topology == static_cast<VkPrimitiveTopology>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Primitive Topology! Choosing TRIANGLE_LIST, might cause errors...\n";
		topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	m_instanceInputAssemblyConfig = { .topology = topology,
									  .primitiveRestart = asBoolOr(config, "primitive-restart", false) };
}

void PipelineInstanceRecord::deserializeRasterization(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid rasterization structure value.\n";
		m_isValid = false;
		return;
	}

	VkCullModeFlags cullMode = 0;
	std::string cullModeFlags = asStringOr(config, "cull-mode", "VK_CULL_MODE_BACK_BIT");
	auto flags = splitString(cullModeFlags);
	for (auto& flag : flags) {
		auto flagBit = VkCullModeFlagBitsFromString(flag);
		if (flagBit == static_cast<VkCullModeFlagBits>(~0U)) {
			std::cout << srcPath
					  << ": Warning: Invalid Cull Mode flag bit specified. Ignoring bit. If flags are 0, "
						 "VK_CULL_MODE_BACK_BIT is used.\n";
		} else {
			cullMode |= flagBit;
		}
	}

	auto polygonMode = VkPolygonModeFromString(asCStringOr(config, "polygon-mode", "VK_POLYGON_MODE_FILL"));
	if (polygonMode == static_cast<VkPolygonMode>(~0U)) {
		std::cout << srcPath << ": Warning: Invalid Polygon Mode specified. Choosing FILL, may cause errors...\n";
		polygonMode = VK_POLYGON_MODE_FILL;
	}

	auto frontFace = VkFrontFaceFromString(asCStringOr(config, "front-face", "VK_FRONT_FACE_COUNTER_CLOCKWISE"));
	if (frontFace == static_cast<VkFrontFace>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Front Face specified. Choosing COUNTER_CLOCKWISE, may cause unintended "
					 "behaviour...\n";
		frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	m_instanceRasterizationConfig = { .depthClampEnable = asBoolOr(config, "depth-clamp", false),
									  .rasterizerDiscardEnable = asBoolOr(config, "rasterizer-discard", false),
									  .polygonMode = polygonMode,
									  .cullMode = cullMode,
									  .frontFace = frontFace,
									  .depthBiasEnable = asBoolOr(config, "depth-bias-enable", false),
									  .depthBiasConstantFactor = asFloatOr(config, "depth-bias-constant-factor", 0.0f),
									  .depthBiasClamp = asFloatOr(config, "depth-bias-clamp", 0.0),
									  .depthBiasSlopeFactor = asFloatOr(config, "depth-bias-slope-factor", 1.0f),
									  .lineWidth = asFloatOr(config, "line-width", 1.0f) };
}

void PipelineInstanceRecord::deserializeViewportScissor(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() == Json::nullValue) {
		m_instanceViewportScissorConfig.viewports.push_back(
			{ .x = 0.0f, .y = 0.0f, .width = 999.9f, .height = 999.9f, .minDepth = 0.0f, .maxDepth = 1.0f });
		m_instanceViewportScissorConfig.scissorRects.push_back(
			{ .offset = {}, .extent = { .width = INT32_MAX, .height = INT32_MAX } });
		return;
	}
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.\n";
		m_isValid = false;
		return;
	}

	if (config["viewports"].type() != Json::arrayValue) {
		std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.\n";
		m_isValid = false;
		return;
	}
	if (config["scissor-rects"].type() != Json::arrayValue) {
		std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.\n";
		m_isValid = false;
		return;
	}
	for (auto& viewport : config["viewports"]) {
		if (viewport.type() != Json::objectValue) {
			std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.\n";
			m_isValid = false;
			return;
		}
		m_instanceViewportScissorConfig.viewports.push_back({ .x = asFloatOr(viewport, "x", 0.0f),
															  .y = asFloatOr(viewport, "y", 0.0f),
															  .width = asFloatOr(viewport, "width", 999.9f),
															  .height = asFloatOr(viewport, "height", 999.9f),
															  .minDepth = asFloatOr(viewport, "min-depth", 0.0f),
															  .maxDepth = asFloatOr(viewport, "max-depth", 1.0f) });
	}
	for (auto& scissor : config["scissor-rects"]) {
		if (scissor.type() != Json::objectValue) {
			std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.\n";
			m_isValid = false;
			return;
		}
		m_instanceViewportScissorConfig.scissorRects.push_back(
			{ .offset = { .x = asIntOr(scissor, "offset-x", 0), .y = asIntOr(scissor, "offset-y", 0) },
			  .extent = { .width = asUIntOr(scissor, "width", INT32_MAX),
						  .height = asUIntOr(scissor, "height", INT32_MAX) } });
	}
}

void PipelineInstanceRecord::deserializeMultisample(const std::string_view& srcPath, const Json::Value& config) {
	m_instanceMultisampleConfig = 0;

	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid multisample structure value.\n";
		m_isValid = false;
		return;
	}

	std::string sampleCountFlags = asStringOr(config, "sample-count", "VK_SAMPLE_COUNT_1_BIT");
	auto flags = splitString(sampleCountFlags);

	for (auto& flag : flags) {
		auto flagBit = VkSampleCountFlagBitsFromString(flag);
		if (flagBit == static_cast<VkSampleCountFlagBits>(~0U)) {
			std::cout << srcPath
					  << ": Warning: Invalid Sample Count Flag bit specified! Ignoring bit. If flags are 0, "
						 "1_BIT is chosen.\n";
		} else {
			m_instanceMultisampleConfig |= flagBit;
		}
	}

	if (m_instanceMultisampleConfig == 0) {
		m_instanceMultisampleConfig = VK_SAMPLE_COUNT_1_BIT;
	}
}

void PipelineInstanceRecord::deserializeDepthStencil(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid depth/stencil structure value.\n";
		m_isValid = false;
		return;
	}

	auto depthCompareOp = VkCompareOpFromString(asCStringOr(config, "depth-compare-op", "VK_COMPARE_OP_LESS"));
	if (depthCompareOp == static_cast<VkCompareOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Depth Compare Op specified. Choosing LESS, may cause "
					 "unintended behaviour...\n";
		depthCompareOp = VK_COMPARE_OP_LESS;
	}

	m_instanceDepthStencilConfig = { .depthTestEnable = asBoolOr(config, "depth-test", true),
									 .depthWriteEnable = asBoolOr(config, "depth-writes", true),
									 .depthCompareOp = depthCompareOp,
									 .depthBoundsTestEnable = asBoolOr(config, "depth-bounds-test", false),
									 .stencilTestEnable = asBoolOr(config, "stencil-test", false),
									 .front = deserializeStencilState(srcPath, config["front"]),
									 .back = deserializeStencilState(srcPath, config["back"]),
									 .minDepthBounds = asFloatOr(config, "depth-bounds-min", 0.0f),
									 .maxDepthBounds = asFloatOr(config, "depth-bounds-max", 1.0f) };
}

VkStencilOpState PipelineInstanceRecord::deserializeStencilState(const std::string_view& srcPath,
																 const Json::Value& config) {
	if (config.type() == Json::nullValue) {
		return VkStencilOpState{};
	}

	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid stencil state structure value.\n";
		m_isValid = false;
		return {};
	}

	auto failOp = VkStencilOpFromString(asCStringOr(config, "fail", "VK_STENCIL_OP_KEEP"));
	if (failOp == static_cast<VkStencilOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Fail Op specified. Choosing KEEP, may cause unintended behaviour...\n";
		failOp = VK_STENCIL_OP_KEEP;
	}

	auto passOp = VkStencilOpFromString(asCStringOr(config, "pass", "VK_STENCIL_OP_KEEP"));
	if (passOp == static_cast<VkStencilOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Pass Op specified. Choosing KEEP, may cause unintended behaviour...\n";
		passOp = VK_STENCIL_OP_KEEP;
	}

	auto depthFailOp = VkStencilOpFromString(asCStringOr(config, "depth-fail", "VK_STENCIL_OP_KEEP"));
	if (depthFailOp == static_cast<VkStencilOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Pass/Depth Fail Op specified. Choosing KEEP, may cause unintended "
					 "behaviour...\n";
		depthFailOp = VK_STENCIL_OP_KEEP;
	}

	VkCompareOp stencilCompareOp;

	auto compareOp = VkCompareOpFromString(asCStringOr(config, "compare-op", "VK_COMPARE_OP_LESS"));
	if (compareOp == static_cast<VkCompareOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Compare Op specified. Choosing LESS, may cause "
					 "unintended behaviour...\n";
		stencilCompareOp = VK_COMPARE_OP_LESS;
	}

	return { .failOp = failOp,
			 .passOp = passOp,
			 .depthFailOp = depthFailOp,
			 .compareOp = stencilCompareOp,
			 .compareMask = asUIntOr(config, "compare-mask", 0),
			 .writeMask = asUIntOr(config, "write-mask", 0),
			 .reference = asUIntOr(config, "reference", 0) };
}

void PipelineInstanceRecord::deserializeDynamicState(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() == Json::nullValue)
		return;
	if (config.type() != Json::arrayValue) {
		std::cout << srcPath << ": Error: Invalid dynamic state structure value.\n";
		m_isValid = false;
		return;
	}
	for (auto& state : config) {
		VkDynamicState dynamicState = VkDynamicStateFromString(state.asCString());
		if (dynamicState == static_cast<VkDynamicState>(~0U)) {
			std::cout << srcPath << ": Warning: Invalid Dynamic state specified, ignoring state...\n";
		} else {
			m_instanceDynamicStateConfig.dynamicStates.push_back(dynamicState);
		}
	}
}

void PipelineInstanceRecord::deserializeColorBlend(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Warning: Invalid color blend structure value, ignoring value.\n";
		return;
	}

	auto logicOp = VkLogicOpFromString(asCStringOr(config, "logic-op", "VK_LOGIC_OP_NO_OP"));
	if (logicOp == static_cast<VkLogicOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Logic Op specified. Choosing NO_OP, may cause unintended behaviour...\n";
		logicOp = VK_LOGIC_OP_NO_OP;
	}

	float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (config["blend-constants"].size() > 4) {
		std::cout << srcPath << ": Warning: More than 4 blend constants specified. Ignoring superfluous constants.\n";
	}
	for (uint32_t i = 0; i < config["blend-constants"].size() && i < 4; ++i) {
		if (config["blend-constants"][i].isNumeric()) {
			blendConstants[i] = config["blend-constants"][i].asFloat();
		} else {
			std::cout << srcPath << ": Warning: Blend constant " << i << " is not numeric, treating value like 0.0.\n";
		}
	}

	m_instanceColorBlendConfig = { .logicOpEnable = asBoolOr(config, "logic-op-enable", false),
								   .logicOp = logicOp,
								   .blendConstants = { blendConstants[0], blendConstants[1], blendConstants[2],
													   blendConstants[3] } };
}

void PipelineInstanceRecord::deserializeColorAttachmentBlend(const std::string_view& srcPath, const Json::Value& node) {
	m_instanceColorAttachmentBlendConfigs.reserve(node.size());

	if (!node.isArray()) {
		std::cout << srcPath << ": Error: Invalid color attachment blend array.\n";
		m_isValid = false;
		return;
	}

	for (auto& config : node) {
		VkColorComponentFlags componentFlags = 0;

		if (!config.isObject()) {
			std::cout << srcPath << ": Error: Invalid color attachment blend structure.\n";
			continue;
		}

		auto srcColorFactor =
			VkBlendFactorFromString(asCStringOr(config, "src-color-factor", "VK_BLEND_FACTOR_SRC_ALPHA"));
		if (srcColorFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Src Color Blend Factor specified. Choosing SRC_ALPHA, might lead to unintended "
				   "behaviour...\n";
			srcColorFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		}

		auto dstColorFactor =
			VkBlendFactorFromString(asCStringOr(config, "dst-color-factor", "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"));
		if (dstColorFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Dst Color Blend Factor specified. Choosing ONE_MINUS_SRC_ALPHA, might lead "
				   "to unintended "
				   "behaviour...\n";
			dstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		}

		auto srcAlphaFactor =
			VkBlendFactorFromString(asCStringOr(config, "src-alpha-factor", "VK_BLEND_FACTOR_SRC_ALPHA"));
		if (srcAlphaFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Src Alpha Blend Factor specified. Choosing SRC_ALPHA, might lead to unintended "
				   "behaviour...\n";
			srcAlphaFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		}

		auto dstAlphaFactor =
			VkBlendFactorFromString(asCStringOr(config, "dst-alpha-factor", "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"));
		if (dstAlphaFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Dst Alpha Blend Factor specified. Choosing ONE_MINUS_SRC_ALPHA, might lead to "
				   "unintended "
				   "behaviour...\n";
			dstAlphaFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		}

		auto colorBlendOp = VkBlendOpFromString(asCStringOr(config, "color-blend-op", "VK_BLEND_OP_ADD"));
		if (colorBlendOp == static_cast<VkBlendOp>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Color Blend Op specified. Choosing ADD, might lead to unintended behaviour...\n";
			colorBlendOp = VK_BLEND_OP_ADD;
		}

		auto alphaBlendOp = VkBlendOpFromString(asCStringOr(config, "alpha-blend-op", "VK_BLEND_OP_ADD"));
		if (alphaBlendOp == static_cast<VkBlendOp>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Alpha Blend Op specified. Choosing ADD, might lead to unintended behaviour...\n";
			alphaBlendOp = VK_BLEND_OP_ADD;
		}

		auto componentFlagBits = splitString(asStringOr(config, "components",
														"VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | "
														"VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"));
		for (auto& flagBit : componentFlagBits) {
			auto bit = VkColorComponentFlagBitsFromString(flagBit);
			if (bit == static_cast<VkColorComponentFlagBits>(~0U)) {
				std::cout << "Invalid Color Component Flag Bit specified, ignoring bit. If the flag is zero, all "
							 "components will be set.\n";
			} else {
				componentFlags |= bit;
			}
		}
		if (componentFlags == 0) {
			componentFlags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
							 VK_COLOR_COMPONENT_A_BIT;
		}

		InstanceColorAttachmentBlendConfig blendConfig = { .blendEnable = asBoolOr(config, "blend-enable", false),
														   .srcColorBlendFactor = srcColorFactor,
														   .dstColorBlendFactor = dstColorFactor,
														   .colorBlendOp = colorBlendOp,
														   .srcAlphaBlendFactor = srcAlphaFactor,
														   .dstAlphaBlendFactor = dstAlphaFactor,
														   .alphaBlendOp = alphaBlendOp,
														   .colorWriteMask = componentFlags };
		m_instanceColorAttachmentBlendConfigs.push_back(blendConfig);
	}
}

void PipelineInstanceRecord::deserializeSpecializationConfigs(const std::string_view& srcPath,
															  const Json::Value& node) {
	if (node.isNull())
		return;

	if (!node.isArray()) {
		std::cout << srcPath << ": Error: Invalid specialization constant stage array.\n";
		m_isValid = false;
		return;
	}

	for (auto& stage : node) {
		if (!stage.isArray()) {
			std::cout << srcPath << ": Error: Invalid specialization constant array.\n";
			m_isValid = false;
			return;
		}
		m_instanceSpecializationConfigs.reserve(stage.size());

		InstanceStageSpecializationConfig newConfig;

		auto stageBit = VkShaderStageFlagBitsFromString(asCStringOr(stage, "stage", " "));
		if (stage == static_cast<VkShaderStageFlagBits>(~0U)) {
			std::cout << srcPath << ": Error: Invalid stage flags for specialization constant.\n";
			m_isValid = false;
			return;
		} else {
			switch (m_type) {
				case PipelineType::Graphics:
					if (stageBit != VK_SHADER_STAGE_VERTEX_BIT && stageBit != VK_SHADER_STAGE_FRAGMENT_BIT) {
						std::cout << srcPath << ": Error: Invalid stage flags for specialization constant.\n";
						m_isValid = false;
						return;
					}
					break;
				case PipelineType::Compute:
					if (stageBit != VK_SHADER_STAGE_COMPUTE_BIT) {
						std::cout << srcPath << ": Error: Invalid stage flags for specialization constant.\n";
						m_isValid = false;
						return;
					}
				default:
					break;
			}
		}

		auto& configArray = stage["values"];

		if (!configArray.isArray()) {
			std::cout << srcPath << ": Error: Invalid specialization constant array.\n";
			m_isValid = false;
			return;
		}

		for (auto& config : stage) {
			if (!config.isObject()) {
				std::cout << srcPath << ": Error: Invalid specialization constant structure.\n";
				m_isValid = false;
				return;
			}

			InstanceSpecializationConfig resultConfig = { .mapEntry = { .constantID = node["id"].asUInt(),
																		.offset = static_cast<uint32_t>(
																			newConfig.specializationDataSize) } };

			if (node["value"].isInt()) {
				uint32_t value;
				int32_t intValue = node["value"].asInt();
				std::memcpy(&value, &intValue, sizeof(uint32_t));

				resultConfig.data = value;
				resultConfig.mapEntry.size = sizeof(uint32_t);
			} else if (node["value"].isUInt()) {
				resultConfig.data = static_cast<uint32_t>(node["value"].asUInt());
				resultConfig.mapEntry.size = sizeof(uint32_t);
			} else if (node["value"].isNumeric()) {
				resultConfig.data = static_cast<float>(node["value"].asFloat());
				resultConfig.mapEntry.size = sizeof(float);
			} else if (node["value"].isBool()) {
				resultConfig.data = static_cast<bool>(node["value"].asBool());
				resultConfig.mapEntry.size = sizeof(bool);
			}

			newConfig.specializationDataSize += resultConfig.mapEntry.size;
			newConfig.configs.push_back(resultConfig);
		}
		m_instanceSpecializationConfigs.push_back(std::move(newConfig));
	}
}