#include <EnumMatchTable.hpp>
#include <ProjectDatabase.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

std::vector<std::string> splitFlags(const std::string& list) {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	std::vector<std::string> splitList;

	while (std::getline(listStream, listEntry, '|')) {
		for (size_t i = 0; i < listEntry.size(); ++i) {
			if (listEntry[i] == ' ') {
				listEntry.erase(listEntry.begin() + i);
				--i;
			}
		}
		splitList.push_back(listEntry);
	}
	return splitList;
}

void ProjectDatabase::deserialize(const std::string_view& filename) {
	auto stream = std::ifstream(filename.data(), std::ios_base::binary);
	if (!stream.is_open()) {
		return;
	}

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	auto fileSize = stream.gcount();
	stream.clear();
	stream.seekg(0, std::ios_base::beg);

	char* data = reinterpret_cast<char*>(malloc(fileSize));
	stream.read(data, fileSize);

	Json::Value rootValue;

	Json::Reader reader;
	reader.parse(data, data + fileSize, rootValue);

	Json::Value nameValue = rootValue["name"].asString();

	deserializeVertexInput(rootValue["vertex-input"]);
	deserializeInputAssembly(rootValue["input-assembly"]);
	deserializeRasterization(rootValue["rasterization"]);
	deserializeMultisample(rootValue["multisample"]);
	deserializeDepthStencil(rootValue["depth-stencil"]);
	deserializeColorBlend(rootValue["color-blend"]);
	deserializeColorAttachmentBlend(rootValue["attachment-blend"]);

	free(data);
}

void ProjectDatabase::deserializeVertexInput(const Json::Value& node) {
	m_instanceVertexInputConfigs.reserve(node.size());
	for (auto& config : node) {
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
				VkVertexInputRateFromString.find(asCStringOr(binding, "inputRate", "VK_VERTEX_INPUT_RATE_VERTEX"));

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

		std::string sampleCountFlags = asStringOr(config, "sample-count", "VK_SAMPLE_COUNT_1_BIT");
		auto flags = splitFlags(sampleCountFlags);

		for (auto& flag : flags) {
			auto flagBitIterator = VkSampleCountFlagBitsFromString.find(flag);
			if (flagBitIterator == VkSampleCountFlagBitsFromString.end()) {
				std::cout << "Warning: Invalid Sample Count Flag bit specified! Ignoring bit. If flags are 0, "
							 "VK_SAMPLE_COUNT_1_BIT is chosen.\n";
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

void ProjectDatabase::deserializeDepthStencil(const Json::Value& node) {}

void ProjectDatabase::deserializeStencilState(const Json::Value& node) {}

void ProjectDatabase::deserializeColorBlend(const Json::Value& node) {}

void ProjectDatabase::deserializeColorAttachmentBlend(const Json::Value& node) {}

uint32_t ProjectDatabase::asUIntOr(const Json::Value& value, const std::string_view& name, uint32_t fallback) {
	if (value.isMember(name.data()) && value[name.data()].isUInt()) {
		return value[name.data()].asUInt();
	} else {
		return fallback;
	}
}

bool ProjectDatabase::asBoolOr(const Json::Value& value, const std::string_view& name, bool fallback) {
	if (value.isMember(name.data()) && value[name.data()].isBool()) {
		return value[name.data()].asBool();
	} else {
		return fallback;
	}
}

const char* ProjectDatabase::asCStringOr(const Json::Value& value, const std::string_view& name, const char* fallback) {
	if (value.isMember(name.data()) && value[name.data()].isString()) {
		return value[name.data()].asCString();
	} else {
		return fallback;
	}
}

std::string ProjectDatabase::asStringOr(const Json::Value& value, const std::string_view& name,
										const std::string& fallback) {
	if (value.isMember(name.data()) && value[name.data()].isString()) {
		return value[name.data()].asString();
	} else {
		return fallback;
	}
}

float ProjectDatabase::asFloatOr(const Json::Value& value, const std::string_view& name, float fallback) {
	if (value.isMember(name.data()) && value[name.data()].isNumeric()) {
		return value[name.data()].asFloat();
	} else {
		return fallback;
	}
}