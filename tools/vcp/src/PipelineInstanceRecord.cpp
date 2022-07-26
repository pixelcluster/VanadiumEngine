/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <EnumMatchTable.hpp>
#include <ParsingUtils.hpp>
#include <PipelineInstanceRecord.hpp>
#include <algorithm>
#include <cfloat>
#include <iostream>

PipelineInstanceRecord::PipelineInstanceRecord(PipelineType type, const std::string_view& srcPath,
											   const Json::Value& instanceNode)
	: m_type(type) {
	m_data.name = asStringOr(instanceNode, "name", "");
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
											const std::vector<ReflectedShader>& shaderModules) {
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
		std::cout << srcPath << ": Error: Internal error when verifying vertex shader.
";
		return;
	}

	auto inputVariables = std::vector<SpvReflectInterfaceVariable*>(inputVariableCount);

	result = spvReflectEnumerateInputVariables(&shader, &inputVariableCount, inputVariables.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cout << srcPath << ": Error: Internal error when verifying vertex shader.
";
		return;
	}

	for (auto& variable : inputVariables) {
		auto var = std::find_if(m_data.instanceVertexInputConfig.attributes.begin(),
								m_data.instanceVertexInputConfig.attributes.end(),
								[variable](const auto& attrib) { return attrib.location == variable->location; });
		if (var == m_data.instanceVertexInputConfig.attributes.end() && variable->location != ~0U) {
			std::cout << srcPath << ": Error: Unbound vertex attribute at location " << variable->location << ".
";
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
		std::cout << srcPath << ": Error: Internal error when verifying fragment shader.
";
		return;
	}

	auto outputVariables = std::vector<SpvReflectInterfaceVariable*>(outputVariableCount);

	result = spvReflectEnumerateOutputVariables(&shader, &outputVariableCount, outputVariables.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cout << srcPath << ": Error: Internal error when verifying fragment shader.
";
		return;
	}

	for (auto& variable : outputVariables) {
		if (variable->location >= m_data.instanceColorAttachmentBlendConfigs.size()) {
			std::cout << srcPath << ": Error: Unbound color attachment output at location " << variable->location
					  << ".
";
			m_isValid = false;
		}
	}
}

void PipelineInstanceRecord::serialize(std::ofstream& outStream) const { ::serialize(m_data, outStream); }

void PipelineInstanceRecord::deserializeVertexInput(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid vertex input structure value.
";
		m_isValid = false;
		return;
	}

	auto& attribNode = config["attributes"];
	auto& bindingNode = config["bindings"];

	m_data.instanceVertexInputConfig.attributes.reserve(attribNode.size());
	m_data.instanceVertexInputConfig.bindings.reserve(bindingNode.size());

	for (auto& attrib : attribNode) {
		VkFormat format = VkFormatFromString(asCStringOr(attrib, "format", "VK_FORMAT_R32G32B32A32_SFLOAT"));

		if (format == static_cast<VkFormat>(~0U)) {
			std::cout << srcPath << ": Warning: Invalid Format! Choosing R32G32B32A32_SFLOAT, might cause errors...
";
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		VkVertexInputAttributeDescription attribDescription = { .location = asUIntOr(attrib, "location", 0),
																.binding = asUIntOr(attrib, "binding", 0),
																.format = format,
																.offset = asUIntOr(attrib, "offset", 0) };
		if (attribDescription.binding >= bindingNode.size()) {
			std::cout << srcPath << ": Error: Invalid binding index for attribute at location "
					  << attribDescription.location << ".
";
			m_isValid = false;
		}
		m_data.instanceVertexInputConfig.attributes.push_back(attribDescription);
	}

	for (auto& binding : bindingNode) {
		VkVertexInputRate inputRate =
			VkVertexInputRateFromString(asCStringOr(binding, "input-rate", "VK_VERTEX_INPUT_RATE_VERTEX"));

		if (inputRate == static_cast<VkVertexInputRate>(~0U)) {
			std::cout << srcPath
					  << ": Warning: Invalid Input Rate! Choosing VERTEX, might cause unintended behaviour...
";
			inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		VkVertexInputBindingDescription bindingDescription = { .binding = asUIntOr(binding, "binding", 0),
															   .stride = asUIntOr(binding, "stride", 0),
															   .inputRate = inputRate };
		m_data.instanceVertexInputConfig.bindings.push_back(bindingDescription);
	}
}

void PipelineInstanceRecord::deserializeInputAssembly(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid input assembly structure value.
";
		m_isValid = false;
		return;
	}

	auto topology = VkPrimitiveTopologyFromString(config["topology"].asCString());

	if (topology == static_cast<VkPrimitiveTopology>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Primitive Topology! Choosing TRIANGLE_LIST, might cause errors...
";
		topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	m_data.instanceInputAssemblyConfig = { .topology = topology,
										   .primitiveRestart = asBoolOr(config, "primitive-restart", false) };
}

void PipelineInstanceRecord::deserializeRasterization(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid rasterization structure value.
";
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
						 "VK_CULL_MODE_BACK_BIT is used.
";
		} else {
			cullMode |= flagBit;
		}
	}

	auto polygonMode = VkPolygonModeFromString(asCStringOr(config, "polygon-mode", "VK_POLYGON_MODE_FILL"));
	if (polygonMode == static_cast<VkPolygonMode>(~0U)) {
		std::cout << srcPath << ": Warning: Invalid Polygon Mode specified. Choosing FILL, may cause errors...
";
		polygonMode = VK_POLYGON_MODE_FILL;
	}

	auto frontFace = VkFrontFaceFromString(asCStringOr(config, "front-face", "VK_FRONT_FACE_COUNTER_CLOCKWISE"));
	if (frontFace == static_cast<VkFrontFace>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Front Face specified. Choosing COUNTER_CLOCKWISE, may cause unintended "
					 "behaviour...
";
		frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}

	m_data.instanceRasterizationConfig = { .depthClampEnable = asBoolOr(config, "depth-clamp", false),
										   .rasterizerDiscardEnable = asBoolOr(config, "rasterizer-discard", false),
										   .polygonMode = polygonMode,
										   .cullMode = cullMode,
										   .frontFace = frontFace,
										   .depthBiasEnable = asBoolOr(config, "depth-bias-enable", false),
										   .depthBiasConstantFactor =
											   asFloatOr(config, "depth-bias-constant-factor", 0.0f),
										   .depthBiasClamp = asFloatOr(config, "depth-bias-clamp", 0.0),
										   .depthBiasSlopeFactor = asFloatOr(config, "depth-bias-slope-factor", 1.0f),
										   .lineWidth = asFloatOr(config, "line-width", 1.0f) };
}

void PipelineInstanceRecord::deserializeViewportScissor(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() == Json::nullValue) {
		m_data.instanceViewportScissorConfig.viewports.push_back(
			{ .x = 0.0f, .y = 0.0f, .width = 999.9f, .height = 999.9f, .minDepth = 0.0f, .maxDepth = 1.0f });
		m_data.instanceViewportScissorConfig.scissorRects.push_back(
			{ .offset = {}, .extent = { .width = INT32_MAX, .height = INT32_MAX } });
		return;
	}
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.
";
		m_isValid = false;
		return;
	}

	if (config["viewports"].type() != Json::arrayValue) {
		std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.
";
		m_isValid = false;
		return;
	}
	if (config["scissor-rects"].type() != Json::arrayValue) {
		std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.
";
		m_isValid = false;
		return;
	}
	for (auto& viewport : config["viewports"]) {
		if (viewport.type() != Json::objectValue) {
			std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.
";
			m_isValid = false;
			return;
		}
		m_data.instanceViewportScissorConfig.viewports.push_back(
			{ .x = asFloatOr(viewport, "x", 0.0f),
			  .y = asFloatOr(viewport, "y", 0.0f),
			  .width = asFloatOr(viewport, "width", 999.9f),
			  .height = asFloatOr(viewport, "height", 999.9f),
			  .minDepth = asFloatOr(viewport, "min-depth", 0.0f),
			  .maxDepth = asFloatOr(viewport, "max-depth", 1.0f) });
	}
	for (auto& scissor : config["scissor-rects"]) {
		if (scissor.type() != Json::objectValue) {
			std::cout << srcPath << ": Error: Invalid viewport/scissor structure value.
";
			m_isValid = false;
			return;
		}
		m_data.instanceViewportScissorConfig.scissorRects.push_back(
			{ .offset = { .x = asIntOr(scissor, "offset-x", 0), .y = asIntOr(scissor, "offset-y", 0) },
			  .extent = { .width = asUIntOr(scissor, "width", INT32_MAX),
						  .height = asUIntOr(scissor, "height", INT32_MAX) } });
	}
}

void PipelineInstanceRecord::deserializeMultisample(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid multisample structure value.
";
		m_isValid = false;
		return;
	}

	m_data.instanceMultisampleConfig =
		VkSampleCountFlagBitsFromString(asStringOr(config, "sample-count", "VK_SAMPLE_COUNT_1_BIT"));

	if (m_data.instanceMultisampleConfig == static_cast<VkSampleCountFlagBits>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Sample count specified. Choosing 1, may cause unintended "
					 "behaviour...
";
		m_data.instanceMultisampleConfig = VK_SAMPLE_COUNT_1_BIT;
	}
}

void PipelineInstanceRecord::deserializeDepthStencil(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Error: Invalid depth/stencil structure value.
";
		m_isValid = false;
		return;
	}

	auto depthCompareOp = VkCompareOpFromString(asCStringOr(config, "depth-compare-op", "VK_COMPARE_OP_LESS"));
	if (depthCompareOp == static_cast<VkCompareOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Depth Compare Op specified. Choosing LESS, may cause "
					 "unintended behaviour...
";
		depthCompareOp = VK_COMPARE_OP_LESS;
	}

	m_data.instanceDepthStencilConfig = { .depthTestEnable = asBoolOr(config, "depth-test", true),
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
		std::cout << srcPath << ": Error: Invalid stencil state structure value.
";
		m_isValid = false;
		return {};
	}

	auto failOp = VkStencilOpFromString(asCStringOr(config, "fail", "VK_STENCIL_OP_KEEP"));
	if (failOp == static_cast<VkStencilOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Fail Op specified. Choosing KEEP, may cause unintended behaviour...
";
		failOp = VK_STENCIL_OP_KEEP;
	}

	auto passOp = VkStencilOpFromString(asCStringOr(config, "pass", "VK_STENCIL_OP_KEEP"));
	if (passOp == static_cast<VkStencilOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Pass Op specified. Choosing KEEP, may cause unintended behaviour...
";
		passOp = VK_STENCIL_OP_KEEP;
	}

	auto depthFailOp = VkStencilOpFromString(asCStringOr(config, "depth-fail", "VK_STENCIL_OP_KEEP"));
	if (depthFailOp == static_cast<VkStencilOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Pass/Depth Fail Op specified. Choosing KEEP, may cause unintended "
					 "behaviour...
";
		depthFailOp = VK_STENCIL_OP_KEEP;
	}

	VkCompareOp stencilCompareOp = {};

	auto compareOp = VkCompareOpFromString(asCStringOr(config, "compare-op", "VK_COMPARE_OP_LESS"));
	if (compareOp == static_cast<VkCompareOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Stencil Compare Op specified. Choosing LESS, may cause "
					 "unintended behaviour...
";
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
		std::cout << srcPath << ": Error: Invalid dynamic state structure value.
";
		m_isValid = false;
		return;
	}
	for (auto& state : config) {
		VkDynamicState dynamicState = VkDynamicStateFromString(state.asCString());
		if (dynamicState == static_cast<VkDynamicState>(~0U)) {
			std::cout << srcPath << ": Warning: Invalid Dynamic state specified, ignoring state...
";
		} else {
			m_data.instanceDynamicStateConfig.dynamicStates.push_back(dynamicState);
		}
	}
}

void PipelineInstanceRecord::deserializeColorBlend(const std::string_view& srcPath, const Json::Value& config) {
	if (config.type() != Json::objectValue) {
		std::cout << srcPath << ": Warning: Invalid color blend structure value, ignoring value.
";
		return;
	}

	auto logicOp = VkLogicOpFromString(asCStringOr(config, "logic-op", "VK_LOGIC_OP_NO_OP"));
	if (logicOp == static_cast<VkLogicOp>(~0U)) {
		std::cout << srcPath
				  << ": Warning: Invalid Logic Op specified. Choosing NO_OP, may cause unintended behaviour...
";
		logicOp = VK_LOGIC_OP_NO_OP;
	}

	float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (config["blend-constants"].size() > 4) {
		std::cout << srcPath << ": Warning: More than 4 blend constants specified. Ignoring superfluous constants.
";
	}
	for (uint32_t i = 0; i < config["blend-constants"].size() && i < 4; ++i) {
		if (config["blend-constants"][i].isNumeric()) {
			blendConstants[i] = config["blend-constants"][i].asFloat();
		} else {
			std::cout << srcPath << ": Warning: Blend constant " << i << " is not numeric, treating value like 0.0.
";
		}
	}

	m_data.instanceColorBlendConfig = { .logicOpEnable = asBoolOr(config, "logic-op-enable", false),
										.logicOp = logicOp,
										.blendConstants = { blendConstants[0], blendConstants[1], blendConstants[2],
															blendConstants[3] } };
}

void PipelineInstanceRecord::deserializeColorAttachmentBlend(const std::string_view& srcPath, const Json::Value& node) {
	m_data.instanceColorAttachmentBlendConfigs.reserve(node.size());

	if (!node.isArray()) {
		std::cout << srcPath << ": Error: Invalid color attachment blend array.
";
		m_isValid = false;
		return;
	}

	for (auto& config : node) {
		VkColorComponentFlags componentFlags = 0;

		if (!config.isObject()) {
			std::cout << srcPath << ": Error: Invalid color attachment blend structure.
";
			continue;
		}

		auto srcColorFactor =
			VkBlendFactorFromString(asCStringOr(config, "src-color-factor", "VK_BLEND_FACTOR_SRC_ALPHA"));
		if (srcColorFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Src Color Blend Factor specified. Choosing SRC_ALPHA, might lead to unintended "
				   "behaviour...
";
			srcColorFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		}

		auto dstColorFactor =
			VkBlendFactorFromString(asCStringOr(config, "dst-color-factor", "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"));
		if (dstColorFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Dst Color Blend Factor specified. Choosing ONE_MINUS_SRC_ALPHA, might lead "
				   "to unintended "
				   "behaviour...
";
			dstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		}

		auto srcAlphaFactor =
			VkBlendFactorFromString(asCStringOr(config, "src-alpha-factor", "VK_BLEND_FACTOR_SRC_ALPHA"));
		if (srcAlphaFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Src Alpha Blend Factor specified. Choosing SRC_ALPHA, might lead to unintended "
				   "behaviour...
";
			srcAlphaFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		}

		auto dstAlphaFactor =
			VkBlendFactorFromString(asCStringOr(config, "dst-alpha-factor", "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"));
		if (dstAlphaFactor == static_cast<VkBlendFactor>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Dst Alpha Blend Factor specified. Choosing ONE_MINUS_SRC_ALPHA, might lead to "
				   "unintended "
				   "behaviour...
";
			dstAlphaFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		}

		auto colorBlendOp = VkBlendOpFromString(asCStringOr(config, "color-blend-op", "VK_BLEND_OP_ADD"));
		if (colorBlendOp == static_cast<VkBlendOp>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Color Blend Op specified. Choosing ADD, might lead to unintended behaviour...
";
			colorBlendOp = VK_BLEND_OP_ADD;
		}

		auto alphaBlendOp = VkBlendOpFromString(asCStringOr(config, "alpha-blend-op", "VK_BLEND_OP_ADD"));
		if (alphaBlendOp == static_cast<VkBlendOp>(~0U)) {
			std::cout
				<< srcPath
				<< ": Warning: Invalid Alpha Blend Op specified. Choosing ADD, might lead to unintended behaviour...
";
			alphaBlendOp = VK_BLEND_OP_ADD;
		}

		auto componentFlagBits = splitString(asStringOr(config, "components",
														"VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | "
														"VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"));
		for (auto& flagBit : componentFlagBits) {
			auto bit = VkColorComponentFlagBitsFromString(flagBit);
			if (bit == static_cast<VkColorComponentFlagBits>(~0U)) {
				std::cout << "Invalid Color Component Flag Bit specified, ignoring bit. If the flag is zero, all "
							 "components will be set.
";
			} else {
				componentFlags |= bit;
			}
		}
		if (componentFlags == 0) {
			componentFlags = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
							 VK_COLOR_COMPONENT_A_BIT;
		}

		PipelineInstanceColorAttachmentBlendConfig blendConfig = { .blendEnable =
																	   asBoolOr(config, "blend-enable", false),
																   .srcColorBlendFactor = srcColorFactor,
																   .dstColorBlendFactor = dstColorFactor,
																   .colorBlendOp = colorBlendOp,
																   .srcAlphaBlendFactor = srcAlphaFactor,
																   .dstAlphaBlendFactor = dstAlphaFactor,
																   .alphaBlendOp = alphaBlendOp,
																   .colorWriteMask = componentFlags };
		m_data.instanceColorAttachmentBlendConfigs.push_back(blendConfig);
	}
}

void PipelineInstanceRecord::deserializeSpecializationConfigs(const std::string_view& srcPath,
															  const Json::Value& node) {
	if (node.isNull())
		return;

	if (!node.isArray()) {
		std::cout << srcPath << ": Error: Invalid specialization constant stage array.
";
		m_isValid = false;
		return;
	}

	for (auto& stage : node) {
		if (!stage.isArray()) {
			std::cout << srcPath << ": Error: Invalid specialization constant array.
";
			m_isValid = false;
			return;
		}
		m_data.instanceSpecializationConfigs.reserve(stage.size());

		PipelineInstanceStageSpecializationConfig newConfig;

		auto stageBit = VkShaderStageFlagBitsFromString(asCStringOr(stage, "stage", " "));
		if (stage == static_cast<VkShaderStageFlagBits>(~0U)) {
			std::cout << srcPath << ": Error: Invalid stage flags for specialization constant.
";
			m_isValid = false;
			return;
		} else {
			switch (m_type) {
				case PipelineType::Graphics:
					if (stageBit != VK_SHADER_STAGE_VERTEX_BIT && stageBit != VK_SHADER_STAGE_FRAGMENT_BIT) {
						std::cout << srcPath << ": Error: Invalid stage flags for specialization constant.
";
						m_isValid = false;
						return;
					}
					break;
				case PipelineType::Compute:
					if (stageBit != VK_SHADER_STAGE_COMPUTE_BIT) {
						std::cout << srcPath << ": Error: Invalid stage flags for specialization constant.
";
						m_isValid = false;
						return;
					}
				default:
					break;
			}
		}

		auto& configArray = stage["values"];

		if (!configArray.isArray()) {
			std::cout << srcPath << ": Error: Invalid specialization constant array.
";
			m_isValid = false;
			return;
		}

		for (auto& config : stage) {
			if (!config.isObject()) {
				std::cout << srcPath << ": Error: Invalid specialization constant structure.
";
				m_isValid = false;
				return;
			}

			PipelineInstanceSpecializationConfig resultConfig = {
				.mapEntry = { .constantID = node["id"].asUInt(),
							  .offset = static_cast<uint32_t>(newConfig.specializationDataSize) }
			};

			if (node["value"].isInt()) {
				uint32_t value;
				int32_t intValue = node["value"].asInt();
				std::memcpy(&value, &intValue, sizeof(uint32_t));

				resultConfig.dataUint32 = value;
				resultConfig.type = SpecializationDataType::UInt32;
				resultConfig.mapEntry.size = sizeof(uint32_t);
			} else if (node["value"].isUInt()) {
				resultConfig.dataUint32 = static_cast<uint32_t>(node["value"].asUInt());
				resultConfig.type = SpecializationDataType::UInt32;
				resultConfig.mapEntry.size = sizeof(uint32_t);
			} else if (node["value"].isNumeric()) {
				resultConfig.dataFloat = static_cast<float>(node["value"].asFloat());
				resultConfig.type = SpecializationDataType::Float;
				resultConfig.mapEntry.size = sizeof(float);
			} else if (node["value"].isBool()) {
				resultConfig.dataBool = static_cast<float>(node["value"].asBool());
				resultConfig.type = SpecializationDataType::Boolean;
				resultConfig.mapEntry.size = sizeof(bool);
			}

			newConfig.specializationDataSize += resultConfig.mapEntry.size;
			newConfig.configs.push_back(resultConfig);
		}
		m_data.instanceSpecializationConfigs.push_back(std::move(newConfig));
	}
}
