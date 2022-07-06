#include "generate.hpp"
#include "parsing_utils.hpp"
#include <iostream>

using namespace tinyxml2;
using namespace std::string_literals;
// defined for parsing_utils.hpp
unsigned int indentationLevel = 0;

void generateFromDocument(const XMLDocument& document, std::ostream& outStream) {
	const XMLElement* registry = document.FirstChildElement("registry");
	if (!registry) {
		std::cout << "Error: Unable to find registry root node, is vk.xml valid?\n";
		return;
	}

	EnumMap enums = parseBasicEnums(registry);
	includeFeatureEnums(registry, enums);
	removeExtensionTypes(registry->FirstChildElement("extensions"), enums);
	writeEnums(enums, outStream);
}

EnumMap parseBasicEnums(const XMLElement* registry) {
	EnumMap enumMap;

	const XMLElement* enumsNode = registry->FirstChildElement("enums");
	if (!enumsNode) {
		std::cout << "Error: Unable to find enums nodes, is vk.xml valid?\n";
		return enumMap;
	}
	if (enumsNode->Attribute("name") && !strcmp(enumsNode->Attribute("name"), "API Constants")) {
		enumsNode = enumsNode->NextSiblingElement("enums");
	}
	while (enumsNode) {
		std::string name;
		const char* nameC = enumsNode->Attribute("name");
		if (nameC) {
			name = nameC;
		}
		vanadium::SimpleVector<std::string> blocklist = { "VkPipelineLayoutCreateFlagBits", "VkSemaphoreCreateFlagBits",
														  "VkImageFormatConstraintsFlagBitsFUCHSIA",
														  "VkShaderModuleCreateFlagBits", "VkPrivateDataSlotCreateFlagBits" };

		bool isNameAllowed = true;
		for (auto& entry : blocklist) {
			isNameAllowed &= name.find(entry, 0) == std::string::npos;
		}

		if (isNameAllowed)
			enumMap.insert(std::pair<std::string, VulkanEnum>(name, parseBasicEnumNode(enumsNode)));
		enumsNode = enumsNode->NextSiblingElement("enums");
	}
	return enumMap;
}

void includeFeatureEnums(const XMLElement* registry, EnumMap& enumMap) {
	const XMLElement* featureNode = registry->FirstChildElement("feature");
	if (!featureNode) {
		std::cout << "Error: Unable to find feature nodes, is vk.xml valid?\n";
		return;
	}
	while (featureNode) {
		parseExtensionEnumNode(featureNode, enumMap);
		featureNode = featureNode->NextSiblingElement("feature");
	}
}
void writeEnums(const EnumMap& enumMap, std::ostream& outStream) {
	addLine(outStream, "#pragma once");

	addLine(outStream, "#include <cstdint>");
	addLine(outStream, "#include <unordered_map>");
	addLine(outStream, "#include <string>");
	addLine(outStream, "#include <vulkan/vulkan.h>");

	for (auto& pair : enumMap) {

		addLine(outStream, "");
		const VulkanEnum& enumValue = pair.second;

		addLine(outStream, "inline "s + pair.first + " "s + pair.first + "FromString(const std::string_view& name) {"s);
		++indentationLevel;
		size_t valueIndex = 0;
		for (const auto& value : enumValue.values()) {
			addLine(outStream, (valueIndex != 0 ? "else if "s : "if "s) + "(name == \""s + value.name + "\") {");
			++indentationLevel;
			addLine(outStream, "return " + value.name + ";");
			--indentationLevel;
			addLine(outStream, "}");
			++valueIndex;
		}
		if (valueIndex != 0) {
			addLine(outStream, "else {");
			++indentationLevel;
		}
		addLine(outStream, "return static_cast<"s + pair.first + ">(~0U);");
		if (valueIndex != 0) {
			--indentationLevel;
			addLine(outStream, "}");
		}
		--indentationLevel;
		addLine(outStream, "}");
	}
}

VulkanEnum parseBasicEnumNode(const XMLElement* node) {
	VulkanEnum result;

	const char* nameC = node->Attribute("name");
	if (nameC) {
		result.name = nameC;
		result.originalName = nameC;
	}

	const char* type = node->Attribute("type");
	if (type && !strcmp(type, "bitmask")) {
		const char* bitwidth = node->Attribute("bitwidth");
		if (bitwidth && !strcmp(bitwidth, "64")) {
			result.type = VulkanEnumType::Bitmask64;
		} else {
			result.type = VulkanEnumType::Bitmask;
		}
	}

	const XMLElement* valueNode = node->FirstChildElement("enum");
	while (valueNode) {
		parseValueNode(valueNode, result);
		valueNode = valueNode->NextSiblingElement("enum");
	}
	return result;
}

void parseExtensionEnumNode(const XMLElement* node, EnumMap& enums) {
	const XMLElement* requireNode = node->FirstChildElement("require");
	while (requireNode) {
		const XMLElement* enumNode = requireNode->FirstChildElement("enum");
		while (enumNode) {
			const char* extendsName = enumNode->Attribute("extends");
			if (extendsName) {
				parseValueNode(enumNode, enums[extendsName]);
			}
			enumNode = enumNode->NextSiblingElement("enum");
		}
		const XMLElement* typeNode = requireNode->FirstChildElement("type");
		while (typeNode) {
			const char* referencedName = typeNode->Attribute("name");
			if (referencedName) {
				auto enumIterator = enums.find(referencedName);
				if (enumIterator != enums.end()) {
					enumIterator->second.isIncluded = true;
				}
			}
			typeNode = typeNode->NextSiblingElement("type");
		}
		requireNode = requireNode->NextSiblingElement("require");
	}
}

void parseValueNode(const XMLElement* valueNode, VulkanEnum& vulkanEnum) {
	VulkanEnumValue enumValue;
	const XMLNode* parent = valueNode->Parent();
	const XMLNode* grandparent = parent ? parent->Parent() : nullptr;

	std::string valueName;
	const char* valueNameC = valueNode->Attribute("name");
	const char* valueC = valueNode->Attribute("value");
	const char* bitposC = valueNode->Attribute("bitpos");
	const char* aliasC = valueNode->Attribute("alias");
	const char* offsetC = valueNode->Attribute("offset");
	const char* extNumberC = grandparent ? grandparent->ToElement()->Attribute("number") : nullptr;
	const char* commentC = valueNode->Attribute("comment");

	if (commentC) {
		enumValue.comment = commentC;
	}

	if (valueNameC) {
		valueName = valueNameC;
		enumValue.name = valueName;
	}

	if (offsetC && extNumberC) {
		int offset = atoi(offsetC);
		int extNumber = atoi(extNumberC);
		enumValue.value = std::to_string(1000000000 + (extNumber - 1) * 1000 + offset);
	} else if (aliasC) {
		enumValue.value = aliasC;
		vulkanEnum.addAliasEnumValue(valueName, enumValue);
	} else if (bitposC) {
		enumValue.value = bitposC;
		enumValue.isBitpos = true;
	} else if (valueC) {
		enumValue.value = valueC;
	}

	vulkanEnum.addEnumValue(valueName, enumValue);
}

void removeExtensionTypes(const XMLElement* extensionsNode, EnumMap& enumMap) {
	const XMLElement* extensionNode = extensionsNode->FirstChildElement("extension");
	while (extensionNode) {
		const XMLElement* requireNode = extensionNode->FirstChildElement("require");
		if (requireNode) {
			const XMLElement* typeNode = requireNode->FirstChildElement("type");
			while (typeNode) {
				const char* name = typeNode->Attribute("name");
				if (name) {
					auto iterator = enumMap.find(name);
					if (iterator != enumMap.end()) {
						enumMap.erase(iterator);
					}
				}
				typeNode = typeNode->NextSiblingElement("type");
			}
		}
		extensionNode = extensionNode->NextSiblingElement("extension");
	}
}