#include <EnumMatchTable.hpp>
#include <ParsingUtils.hpp>
#include <PipelineArchetypeRecord.hpp>
#include <iostream>

PipelineArchetypeRecord::PipelineArchetypeRecord(const Json::Value& archetypeRoot) {
	if (!archetypeRoot.isObject()) {
		std::cout << "Error: Archetype node is invalid." << std::endl;
		std::exit(EXIT_FAILURE);
	}

	if (!archetypeRoot["type"].isString()) {
		std::cout << "Error: Invalid type for pipeline archetype." << std::endl;
		std::exit(EXIT_FAILURE);
	} else {
		if (archetypeRoot["type"].asString() == "Graphics") {
			m_pipelineType = PipelineType::Graphics;
		} else if (archetypeRoot["type"].asString() == "Compute") {
			m_pipelineType = PipelineType::Compute;
		} else {
			std::cout << "Error: Unrecognized type for pipeline archetype." << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}

	switch (m_pipelineType) {
		case PipelineType::Graphics:
			m_files.reserve(2);
			if (!archetypeRoot["vert"].isString()) {
				std::cout << "Error: Vertex shader node has invalid type." << std::endl;
				std::exit(EXIT_FAILURE);
			} else {
				m_files.push_back({ .stage = VK_SHADER_STAGE_VERTEX_BIT, .path = archetypeRoot["vert"].asCString() });
			}
			if (!archetypeRoot["frag"].isString()) {
				std::cout << "Error: Vertex shader node has invalid type." << std::endl;
				std::exit(EXIT_FAILURE);
			} else {
				m_files.push_back({ .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .path = archetypeRoot["frag"].asCString() });
			}
			break;
		case PipelineType::Compute:
			m_files.reserve(1);
			if (!archetypeRoot["comp"].isString()) {
				std::cout << "Error: Vertex shader node has invalid type." << std::endl;
				std::exit(EXIT_FAILURE);
			} else {
				m_files.push_back({ .stage = VK_SHADER_STAGE_COMPUTE_BIT, .path = archetypeRoot["comp"].asCString() });
			}
			break;
	}

	if (!archetypeRoot["sets"].isArray()) {
		std::cout << "Error: Invalid descriptor sets for pipeline archetype." << std::endl;
		std::exit(EXIT_FAILURE);
	} else {
		uint32_t setIndex = 0;
		uint32_t bindingCount = 0;

		m_setBindingOffsets.reserve(archetypeRoot["sets"].size());

		for (auto& set : archetypeRoot["sets"]) {
			if (!set["bindings"].isArray()) {
				std::cout << "Error: Invalid descriptor bindings for set " << setIndex << "." << std::endl;
				std::exit(EXIT_FAILURE);
			} else {
				m_setBindingOffsets.push_back(bindingCount);
				uint32_t bindingIndex = 0;
				for (auto& bindingNode : set["bindings"]) {
					if (!(bindingNode["binding"].isUInt() && bindingNode["type"].isString() &&
						  bindingNode["count"].isUInt() && bindingNode["stages"].isString() &&
						  (bindingNode["immutable-samplers"].isBool() || bindingNode["immutable-samplers"].isNull()))) {
						std::cout << "Error: Invalid descriptor binding at index " << bindingIndex << " for set "
								  << setIndex << "." << std::endl;
						std::exit(EXIT_FAILURE);
					}

					auto typeIterator = VkDescriptorTypeFromString.find(bindingNode["stages"].asString());
					if (typeIterator == VkDescriptorTypeFromString.end()) {
						std::cout << "Error: Invalid descriptor type at binding index " << bindingIndex << " for set "
								  << setIndex << "." << std::endl;
						std::exit(EXIT_FAILURE);
					}

					VkShaderStageFlags shaderStageFlags = 0;
					auto stageFlagsString = splitFlags(bindingNode["stages"].asString());

					bool usesImmutableSamplers = false;
					if (bindingNode["immutable-samplers"].isBool()) {
						usesImmutableSamplers = bindingNode["immutable-samplers"].asBool();
					}

					VkDescriptorSetLayoutBinding binding = { .binding = bindingNode["binding"].asUInt(),
															 .descriptorType = typeIterator->second,
															 .descriptorCount = bindingNode["count"].asUInt(),
															 .stageFlags = shaderStageFlags,
															 .pImmutableSamplers = nullptr };
					m_bindings.push_back({ .binding = binding, .usesImmutableSamplers = usesImmutableSamplers });
				}
			}
			++setIndex;
		}
	}
}