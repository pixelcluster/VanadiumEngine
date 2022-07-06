#pragma once

#include <util/Vector.hpp>
#include <unordered_map>
#include <string>
#include <ostream>
#include <tinyxml2.h>
#include <unordered_set>

//Main header generation logic.
//Generates a C++ header based on the Vulkan vk.xml specification file.

//User-defined options on what exactly to parse and how to generate the header
struct VulkanEnumValue
{
	std::string name, value, comment;
	bool isBitpos = false;
};

enum class VulkanEnumType {
	Enum, Bitmask, Bitmask64
};

class VulkanEnum
{
public:
	std::string name;
	std::string originalName;
	bool isIncluded = false;
	VulkanEnumType type = VulkanEnumType::Enum;

	//Enum value definitions are sometimes duplicated across vk.xml, this makes sure the same value name won't occur twice
	void addEnumValue(std::string originalValueName, VulkanEnumValue value) {
		if (m_originalValueNames.find(originalValueName) == m_originalValueNames.end()) {
			m_values.push_back(value);
			m_originalValueNames.insert(originalValueName);
		}
	}

	void addAliasEnumValue(std::string originalValueName, VulkanEnumValue value) {
		if (m_originalValueNames.find(originalValueName) == m_originalValueNames.end()) {
			m_aliasValues.push_back(value);
			m_originalValueNames.insert(originalValueName);
		}
	}

	const vanadium::SimpleVector<VulkanEnumValue>& values() const {
		return m_values;
	}

	const vanadium::SimpleVector<VulkanEnumValue>& aliasValues() const {
		return m_aliasValues;
	}
private:
	vanadium::SimpleVector<VulkanEnumValue> m_values;
	//Alias values need to be put last in order to ensure the aliases are already defined
	vanadium::SimpleVector<VulkanEnumValue> m_aliasValues;
	std::unordered_set<std::string> m_originalValueNames;
};

namespace std {
	template<>
	struct hash<VulkanEnum> {
		size_t operator()(const VulkanEnum& enumValue) const {
			return hash<string>()(enumValue.originalName);
		}
	};
}

using EnumMap = std::unordered_map<std::string, ::VulkanEnum>;

//Generates a full header file from document according to the parsing options and writes it to outStream.
void generateFromDocument(const tinyxml2::XMLDocument& document, std::ostream& outStream);

//Document/Structure helpers

//Look up all defined enums
EnumMap parseBasicEnums(const tinyxml2::XMLElement* registry);
//Finds and includes all feature enums (=core enums that were added in later versions) and applies additions to existing enums.
void includeFeatureEnums(const tinyxml2::XMLElement* registry, EnumMap& enums);
//Finds and includes all extension enums that are specified in the parsing options and applies additions to existing enums.
void includeExtensionEnums(const tinyxml2::XMLElement* registry, EnumMap& enums);
//Writes the included enums to a stream
void writeEnums(const EnumMap& enums, std::ostream& outStream);
//Writes one enum value to a stream
void writeEnumValue(const VulkanEnumValue& enumValue, std::ostream& outStream, bool isLastValue);

//Node helpers

//Parses a basic enum node which isn't an extension to other existing nodes
VulkanEnum parseBasicEnumNode(const tinyxml2::XMLElement* node);
//Parses an extension node. Includes referenced enums and applies additions.
void parseExtensionEnumNode(const tinyxml2::XMLElement* node, EnumMap& enums);

void removeExtensionTypes(const tinyxml2::XMLElement* extensionsNode, EnumMap& enumMap);

//Name/Value helpers

//Parses an XML node containing an enum value
void parseValueNode(const tinyxml2::XMLElement* node, VulkanEnum& vulkanEnum);