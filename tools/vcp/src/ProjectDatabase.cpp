#include <EnumMatchTable.hpp>
#include <ProjectDatabase.hpp>
#include <iostream>
#include <cfloat>
#include <ParsingUtils.hpp>

ProjectDatabase::ProjectDatabase(const Json::Value& projectRoot) {
	deserializeVertexInput(projectRoot["vertex-input"]);
	deserializeInputAssembly(projectRoot["input-assembly"]);
	deserializeRasterization(projectRoot["rasterization"]);
	deserializeMultisample(projectRoot["multisample"]);
	deserializeDepthStencil(projectRoot["depth-stencil"]);
	deserializeColorBlend(projectRoot["color-blend"]);
	deserializeColorAttachmentBlend(projectRoot["attachment-blend"]);
}

void ProjectDatabase::deserializeVertexInput(const Json::Value& node) {
	m_instanceVertexInputConfigs.reserve(node.size());
	for (auto& config : node) {
		if (config.type() != Json::objectValue) {
			std::cout << "Warning: Invalid color blend structure value, ignoring value.\n";
			continue;
		}

		auto& attribNode = config["attributes"];
		auto& bindingNode = config["bindings"];

		InstanceVertexInputConfig resultConfig;
		resultConfig.attributes.reserve(attribNode.size());
		resultConfig.bindings.reserve(bindingNode.size());

		for (auto& attrib : attribNode) {
			VkFormat format;
			auto formatIterator =
				VkFormatFromString.find(asCStringOr(attrib, "format", "VK_FORMAT_R32G32B32A32_SFLOAT"));

			if (formatIterator == VkFormatFromString.end()) {
				std::cout << "Warning: Invalid Format! Choosing R32G32B32A32_SFLOAT, might cause errors...\n";
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
			} else {
				format = formatIterator->second;
			}

			VkVertexInputAttributeDescription attribDescription = { .location = asUIntOr(attrib, "location", 0),
																	.binding = asUIntOr(attrib, "binding", 0),
																	.format = format,
																	.offset = asUIntOr(attrib, "offset", 0) };
			resultConfig.attributes.push_back(attribDescription);
		}

		for (auto& binding : bindingNode) {
			VkVertexInputRate inputRate;
			auto rateIterator =
				VkVertexInputRateFromString.find(asCStringOr(binding, "input-rate", "VK_VERTEX_INPUT_RATE_VERTEX"));

			if (rateIterator == VkVertexInputRateFromString.end()) {
				std::cout << "Warning: Invalid Input Rate! Choosing VERTEX, might cause unintended behaviour...\n";
				inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			} else {
				inputRate = rateIterator->second;
			}

			VkVertexInputBindingDescription bindingDescription = { .binding = asUIntOr(binding, "binding", 0),
																   .stride = asUIntOr(binding, "stride", 0),
																   .inputRate = inputRate };
			resultConfig.bindings.push_back(bindingDescription);
		}

		m_instanceVertexInputConfigs.push_back(std::move(resultConfig));
	}
}

void ProjectDatabase::deserializeInputAssembly(const Json::Value& node) {
	m_instanceInputAssemblyConfigs.reserve(node.size());
	for (auto& config : node) {
		VkPrimitiveTopology topology;

		if (config.type() != Json::objectValue) {
			std::cout << "Warning: Invalid color blend structure value, ignoring value.\n";
			continue;
		}

		auto topologyIterator = VkPrimitiveTopologyFromString.find(config["topology"].asCString());

		if (topologyIterator == VkPrimitiveTopologyFromString.end()) {
			std::cout << "Warning: Invalid Primitive Topology! Choosing TRIANGLE_LIST, might cause errors...\n";
			topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		} else {
			topology = topologyIterator->second;
		}

		InstanceInputAssemblyConfig resultConfig = { .topology = topology,
													 .primitiveRestart = asBoolOr(config, "primitive-restart", false) };

		m_instanceInputAssemblyConfigs.push_back(resultConfig);
	}
}

void ProjectDatabase::deserializeRasterization(const Json::Value& node) {
	m_instanceRasterizationConfigs.reserve(node.size());
	for (auto& config : node) {
		VkPolygonMode polygonMode;
		VkCullModeFlags cullMode = 0;
		VkFrontFace frontFace;

		if (config.type() != Json::objectValue) {
			std::cout << "Warning: Invalid color blend structure value, ignoring value.\n";
			continue;
		}

		std::string cullModeFlags = asStringOr(config, "cull-mode", "VK_CULL_MODE_BACK_BIT");
		auto flags = splitFlags(cullModeFlags);
		for (auto& flag : flags) {
			auto flagIterator = VkCullModeFlagBitsFromString.find(flag);
			if (flagIterator == VkCullModeFlagBitsFromString.end()) {
				std::cout << "Warning: Invalid Cull Mode flag bit specified. Ignoring bit. If flags are 0, "
							 "VK_CULL_MODE_BACK_BIT is used.\n";
			} else {
				cullMode |= flagIterator->second;
			}
		}

		auto polygonModeIterator =
			VkPolygonModeFromString.find(asCStringOr(config, "polygon-mode", "VK_POLYGON_MODE_FILL"));
		if (polygonModeIterator == VkPolygonModeFromString.end()) {
			std::cout << "Warning: Invalid Polygon Mode specified. Choosing FILL, may cause errors...\n";
			polygonMode = VK_POLYGON_MODE_FILL;
		} else {
			polygonMode = polygonModeIterator->second;
		}

		auto frontFaceIterator =
			VkFrontFaceFromString.find(asCStringOr(config, "front-face", "VK_FRONT_FACE_COUNTER_CLOCKWISE"));
		if (frontFaceIterator == VkFrontFaceFromString.end()) {
			std::cout << "Warning: Invalid Front Face specified. Choosing COUNTER_CLOCKWISE, may cause unintended "
						 "behaviour...\n";
			frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		} else {
			frontFace = frontFaceIterator->second;
		}

		InstanceRasterizationConfig rasterConfig = {
			.depthClampEnable = asBoolOr(config, "depth-clamp", false),
			.rasterizerDiscardEnable = asBoolOr(config, "rasterizer-discard", false),
			.polygonMode = polygonMode,
			.cullMode = cullMode,
			.frontFace = frontFace,
			.depthBiasEnable = asBoolOr(config, "depth-bias-enable", false),
			.depthBiasConstantFactor = asFloatOr(config, "depth-bias-constant-factor", 0.0f),
			.depthBiasClamp = asFloatOr(config, "depth-bias-clamp", FLT_MAX),
			.depthBiasSlopeFactor = asFloatOr(config, "depth-bias-slope-factor", 1.0f),
			.lineWidth = asFloatOr(config, "line-width", 1.0f)
		};
		m_instanceRasterizationConfigs.push_back(rasterConfig);
	}
}

void ProjectDatabase::deserializeMultisample(const Json::Value& node) {
	m_instanceMultisampleConfigs.reserve(node.size());
	for (auto& config : node) {
		VkSampleCountFlags sampleCount = 0;

		if (config.type() != Json::objectValue) {
			std::cout << "Warning: Invalid color blend structure value, ignoring value.\n";
			continue;
		}

		std::string sampleCountFlags = asStringOr(config, "sample-count", "VK_SAMPLE_COUNT_1_BIT");
		auto flags = splitFlags(sampleCountFlags);

		for (auto& flag : flags) {
			auto flagBitIterator = VkSampleCountFlagBitsFromString.find(flag);
			if (flagBitIterator == VkSampleCountFlagBitsFromString.end()) {
				std::cout << "Warning: Invalid Sample Count Flag bit specified! Ignoring bit. If flags are 0, "
							 "1_BIT is chosen.\n";
			} else {
				sampleCount |= flagBitIterator->second;
			}
		}

		if (sampleCount == 0) {
			sampleCount = VK_SAMPLE_COUNT_1_BIT;
		}

		m_instanceMultisampleConfigs.push_back(sampleCount);
	}
}

void ProjectDatabase::deserializeDepthStencil(const Json::Value& node) {
	m_instanceDepthStencilConfigs.reserve(node.size());
	for (auto& config : node) {
		VkCompareOp depthCompareOp;

		if (config.type() != Json::objectValue) {
			std::cout << "Warning: Invalid color blend structure value, ignoring value.\n";
			continue;
		}

		auto compareOpIterator =
			VkCompareOpFromString.find(asCStringOr(config, "depth-compare-op", "VK_COMPARE_OP_LESS"));
		if (compareOpIterator == VkCompareOpFromString.end()) {
			std::cout << "Warning: Invalid Depth Compare Op specified. Choosing LESS, may cause "
						 "unintended behaviour...\n";
			depthCompareOp = VK_COMPARE_OP_LESS;
		} else {
			depthCompareOp = compareOpIterator->second;
		}

		InstanceDepthStencilConfig depthStencilConfig = { .depthTestEnable = asBoolOr(config, "depth-test", true),
														  .depthWriteEnable = asBoolOr(config, "depth-writes", true),
														  .depthCompareOp = depthCompareOp,
														  .depthBoundsTestEnable =
															  asBoolOr(config, "depth-bounds-test", false),
														  .stencilTestEnable = asBoolOr(config, "stencil-test", false),
														  .front = deserializeStencilState(config["front"]),
														  .back = deserializeStencilState(config["back"]),
														  .minDepthBounds = asFloatOr(config, "depth-bounds-min", 0.0f),
														  .maxDepthBounds =
															  asFloatOr(config, "depth-bounds-max", 1.0f) };
		m_instanceDepthStencilConfigs.push_back(depthStencilConfig);
	}
}

VkStencilOpState ProjectDatabase::deserializeStencilState(const Json::Value& node) {
	VkStencilOp failOp, passOp, depthFailOp;
	VkCompareOp compareOp;

	auto failOpIterator = VkStencilOpFromString.find(asCStringOr(node, "fail", "VK_STENCIL_OP_KEEP"));
	if (failOpIterator == VkStencilOpFromString.end()) {
		std::cout << "Warning: Invalid Stencil Fail Op specified. Choosing KEEP, may cause unintended behaviour...\n";
		failOp = VK_STENCIL_OP_KEEP;
	} else {
		failOp = failOpIterator->second;
	}
	auto passOpIterator = VkStencilOpFromString.find(asCStringOr(node, "pass", "VK_STENCIL_OP_KEEP"));
	if (passOpIterator == VkStencilOpFromString.end()) {
		std::cout << "Warning: Invalid Stencil Pass Op specified. Choosing KEEP, may cause unintended behaviour...\n";
		passOp = VK_STENCIL_OP_KEEP;
	} else {
		passOp = passOpIterator->second;
	}
	auto depthFailOpIterator = VkStencilOpFromString.find(asCStringOr(node, "depth-fail", "VK_STENCIL_OP_KEEP"));
	if (depthFailOpIterator == VkStencilOpFromString.end()) {
		std::cout << "Warning: Invalid Stencil Pass/Depth Fail Op specified. Choosing KEEP, may cause unintended "
					 "behaviour...\n";
		depthFailOp = VK_STENCIL_OP_KEEP;
	} else {
		depthFailOp = depthFailOpIterator->second;
	}

	VkCompareOp stencilCompareOp;

	auto compareOpIterator = VkCompareOpFromString.find(asCStringOr(node, "compare-op", "VK_COMPARE_OP_LESS"));
	if (compareOpIterator == VkCompareOpFromString.end()) {
		std::cout << "Warning: Invalid Stencil Compare Op specified. Choosing LESS, may cause "
					 "unintended behaviour...\n";
		stencilCompareOp = VK_COMPARE_OP_LESS;
	} else {
		stencilCompareOp = compareOpIterator->second;
	}

	return { .failOp = failOp,
			 .passOp = passOp,
			 .depthFailOp = depthFailOp,
			 .compareOp = stencilCompareOp,
			 .compareMask = asUIntOr(node, "compare-mask", 0),
			 .writeMask = asUIntOr(node, "write-mask", 0),
			 .reference = asUIntOr(node, "reference", 0) };
}

void ProjectDatabase::deserializeColorBlend(const Json::Value& node) {
	m_instanceColorBlendConfigs.reserve(node.size());
	for (auto& config : node) {
		VkLogicOp logicOp;

		if (config.type() != Json::objectValue) {
			std::cout << "Warning: Invalid color blend structure value, ignoring value.\n";
			continue;
		}

		auto logicOpIterator = VkLogicOpFromString.find(asCStringOr(config, "logic-op", "VK_LOGIC_OP_NO_OP"));
		if (logicOpIterator == VkLogicOpFromString.end()) {
			std::cout << "Warning: Invalid Logic Op specified. Choosing NO_OP, may cause unintended behaviour...\n";
			logicOp = VK_LOGIC_OP_NO_OP;
		} else {
			logicOp = logicOpIterator->second;
		}

		float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		if (config["blend-constants"].size() > 4) {
			std::cout << "Warning: More than 4 blend constants specified. Ignoring superfluous constants.\n";
		}
		for (uint32_t i = 0; i < config["blend-constants"].size() && i < 4; ++i) {
			if (config["blend-constants"][i].isNumeric()) {
				blendConstants[i] = config["blend-constants"][i].asFloat();
			} else {
				std::cout << "Warning: Blend constant " << i << " is not numeric, treating value like 0.0.\n";
			}
		}

		InstanceColorBlendConfig blendConfig = { .logicOpEnable = asBoolOr(config, "logic-op-enable", false),
												 .logicOp = logicOp,
												 .blendConstants = { blendConstants[0], blendConstants[1],
																	 blendConstants[2], blendConstants[3] } };

		m_instanceColorBlendConfigs.push_back(blendConfig);
	}
}

void ProjectDatabase::deserializeColorAttachmentBlend(const Json::Value& node) {
	m_instanceColorAttachmentBlendConfigs.reserve(node.size());
	for (auto& config : node) {
		VkBlendFactor srcColorFactor, srcAlphaFactor, dstColorFactor, dstAlphaFactor;
		VkBlendOp colorBlendOp, alphaBlendOp;
		VkColorComponentFlags componentFlags = 0;

		auto srcColorFactorIterator =
			VkBlendFactorFromString.find(asCStringOr(config, "src-color-factor", "VK_BLEND_FACTOR_SRC_ALPHA"));
		if (srcColorFactorIterator == VkBlendFactorFromString.end()) {
			std::cout
				<< "Warning: Invalid Src Color Blend Factor specified. Choosing SRC_ALPHA, might lead to unintended "
				   "behaviour...\n";
			srcColorFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		} else {
			srcColorFactor = srcColorFactorIterator->second;
		}

		auto dstColorFactorIterator = VkBlendFactorFromString.find(
			asCStringOr(config, "dst-color-factor", "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"));
		if (dstColorFactorIterator == VkBlendFactorFromString.end()) {
			std::cout << "Warning: Invalid Dst Color Blend Factor specified. Choosing ONE_MINUS_SRC_ALPHA, might lead "
						 "to unintended "
						 "behaviour...\n";
			dstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		} else {
			dstColorFactor = dstColorFactorIterator->second;
		}

		auto srcAlphaFactorIterator =
			VkBlendFactorFromString.find(asCStringOr(config, "src-alpha-factor", "VK_BLEND_FACTOR_SRC_ALPHA"));
		if (srcAlphaFactorIterator == VkBlendFactorFromString.end()) {
			std::cout
				<< "Warning: Invalid Src Alpha Blend Factor specified. Choosing SRC_ALPHA, might lead to unintended "
				   "behaviour...\n";
			srcAlphaFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		} else {
			srcAlphaFactor = srcAlphaFactorIterator->second;
		}

		auto dstAlphaFactorIterator = VkBlendFactorFromString.find(
			asCStringOr(config, "dst-alpha-factor", "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA"));
		if (dstAlphaFactorIterator == VkBlendFactorFromString.end()) {
			std::cout
				<< "Warning: Invalid Dst Alpha Blend Factor specified. Choosing ONE_MINUS_SRC_ALPHA, might lead to "
				   "unintended "
				   "behaviour...\n";
			dstAlphaFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		} else {
			dstAlphaFactor = dstAlphaFactorIterator->second;
		}

		auto colorBlendOpIterator = VkBlendOpFromString.find(asCStringOr(config, "color-blend-op", "VK_BLEND_OP_ADD"));
		if (colorBlendOpIterator == VkBlendOpFromString.end()) {
			std::cout
				<< "Warning: Invalid Color Blend Op specified. Choosing ADD, might lead to unintended behaviour...\n";
			colorBlendOp = VK_BLEND_OP_ADD;
		} else {
			colorBlendOp = colorBlendOpIterator->second;
		}

		auto alphaBlendOpIterator = VkBlendOpFromString.find(asCStringOr(config, "alpha-blend-op", "VK_BLEND_OP_ADD"));
		if (alphaBlendOpIterator == VkBlendOpFromString.end()) {
			std::cout
				<< "Warning: Invalid Alpha Blend Op specified. Choosing ADD, might lead to unintended behaviour...\n";
			alphaBlendOp = VK_BLEND_OP_ADD;
		} else {
			alphaBlendOp = alphaBlendOpIterator->second;
		}

		auto componentFlagBits = splitFlags(asStringOr(config, "components",
													   "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | "
													   "VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"));
		for (auto& flagBit : componentFlagBits) {
			auto bitIterator = VkColorComponentFlagBitsFromString.find(flagBit);
			if (bitIterator == VkColorComponentFlagBitsFromString.end()) {
				std::cout << "Invalid Color Component Flag Bit specified, ignoring bit. If the flag is zero, all "
							 "components will be set.\n";
			} else {
				componentFlags |= bitIterator->second;
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

uint32_t ProjectDatabase::asUIntOr(const Json::Value& value, const std::string_view& name, uint32_t fallback) {
	if (value[name.data()].isUInt()) {
		return value[name.data()].asUInt();
	} else {
		return fallback;
	}
}

bool ProjectDatabase::asBoolOr(const Json::Value& value, const std::string_view& name, bool fallback) {
	if (value[name.data()].isBool()) {
		return value[name.data()].asBool();
	} else {
		return fallback;
	}
}

const char* ProjectDatabase::asCStringOr(const Json::Value& value, const std::string_view& name, const char* fallback) {
	if (/*value.type() == Json::objectValue && */value[name.data()].isString()) {
		return value[name.data()].asCString();
	} else {
		return fallback;
	}
}

std::string ProjectDatabase::asStringOr(const Json::Value& value, const std::string_view& name,
										const std::string& fallback) {
	if (value[name.data()].isString()) {
		return value[name.data()].asString();
	} else {
		return fallback;
	}
}

float ProjectDatabase::asFloatOr(const Json::Value& value, const std::string_view& name, float fallback) {
	if (value[name.data()].isNumeric()) {
		return value[name.data()].asFloat();
	} else {
		return fallback;
	}
}