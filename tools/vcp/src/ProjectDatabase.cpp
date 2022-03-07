#include <EnumMatchTable.hpp>
#include <ProjectDatabase.hpp>
#include <fstream>

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
			VkVertexInputAttributeDescription attribDescription = { .location = attrib["location"].asUInt(),
																	.binding = attrib["binding"].asUInt(),
																	.format = VkFormatFromString[attrib["format"].asString()],
																	.offset = attrib["offset"].asUInt() };
			resultConfig.attributes.push_back(attribDescription);
		}

		for (auto& binding : bindingNode) {
			VkVertexInputBindingDescription bindingDescription = { .binding = binding["binding"].asUInt(),
																   .stride = binding["stride"].asUInt(),
																   .inputRate =
																	   binding["inputRate"].asString() == "instance"
																		   ? VK_VERTEX_INPUT_RATE_INSTANCE
																		   : VK_VERTEX_INPUT_RATE_VERTEX };
			resultConfig.bindings.push_back(bindingDescription);
		}

		m_instanceVertexInputConfigs.push_back(std::move(resultConfig));
	}
}

void ProjectDatabase::deserializeInputAssembly(const Json::Value& node) {

}

void ProjectDatabase::deserializeRasterization(const Json::Value& node) {}

void ProjectDatabase::deserializeMultisample(const Json::Value& node) {}

void ProjectDatabase::deserializeDepthStencil(const Json::Value& node) {}

void ProjectDatabase::deserializeColorBlend(const Json::Value& node) {}

void ProjectDatabase::deserializeColorAttachmentBlend(const Json::Value& node) {}