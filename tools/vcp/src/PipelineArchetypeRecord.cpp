#include <EnumMatchTable.hpp>
#include <ParsingUtils.hpp>
#include <PipelineArchetypeRecord.hpp>
#include <helper/WholeFileReader.hpp>
#include <iostream>
#include <optional>

// returns true if [offset1;size1] is inside [offset2;size2]
bool isFullyInRange(uint32_t offset1, uint32_t size1, uint32_t offset2, uint32_t size2) {
	return offset2 <= offset1 && (offset2 + size2) >= (offset1 + size1);
}

PipelineArchetypeRecord::PipelineArchetypeRecord(const std::string_view& srcPath, const std::string& projectDir,
												 std::vector<std::vector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
												 const Json::Value& archetypeRoot) {
	if (!archetypeRoot.isObject()) {
		std::cout << srcPath << ": Error: Archetype node is invalid." << std::endl;
		return;
	}

	if (!archetypeRoot["type"].isString()) {
		std::cout << srcPath << ": Error: Invalid type for pipeline archetype." << std::endl;
		return;
	} else {
		if (archetypeRoot["type"].asString() == "Graphics") {
			m_pipelineType = PipelineType::Graphics;
		} else if (archetypeRoot["type"].asString() == "Compute") {
			m_pipelineType = PipelineType::Compute;
		} else {
			std::cout << srcPath << ": Error: Unrecognized type for pipeline archetype." << std::endl;
			return;
		}
	}

	switch (m_pipelineType) {
		case PipelineType::Graphics:
			m_files.reserve(2);
			if (!archetypeRoot["vert"].isString()) {
				std::cout << srcPath << ": Error: Vertex shader node has invalid type." << std::endl;
				return;
			} else {
				m_files.push_back(
					{ .stage = VK_SHADER_STAGE_VERTEX_BIT, .path = projectDir + archetypeRoot["vert"].asCString() });
			}
			if (!archetypeRoot["frag"].isString()) {
				std::cout << srcPath << ": Error: Vertex shader node has invalid type." << std::endl;
				return;
			} else {
				m_files.push_back(
					{ .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .path = projectDir + archetypeRoot["frag"].asCString() });
			}
			break;
		case PipelineType::Compute:
			m_files.reserve(1);
			if (!archetypeRoot["comp"].isString()) {
				std::cout << srcPath << ": Error: Vertex shader node has invalid type." << std::endl;
				return;
			} else {
				m_files.push_back(
					{ .stage = VK_SHADER_STAGE_COMPUTE_BIT, .path = projectDir + archetypeRoot["comp"].asCString() });
			}
			break;
	}

	if (!archetypeRoot["sets"].isArray()) {
		std::cout << srcPath << ": Error: Invalid descriptor sets for pipeline archetype." << std::endl;
		return;
	}
	uint32_t setIndex = 0;

	auto bindingLayoutInfos = std::vector<std::vector<DescriptorBindingLayoutInfo>>(archetypeRoot["sets"].size());
	m_setLayoutIndices.reserve(archetypeRoot["sets"].size());

	for (auto& set : archetypeRoot["sets"]) {
		if (!set["bindings"].isArray()) {
			std::cout << srcPath << ": Error: Invalid descriptor bindings for set " << setIndex << "." << std::endl;
			return;
		} else {
			bindingLayoutInfos.reserve(set["bindings"].size());
			uint32_t bindingIndex = 0;
			for (auto& bindingNode : set["bindings"]) {
				if (!(bindingNode["binding"].isUInt() && bindingNode["type"].isString() &&
					  bindingNode["count"].isUInt() && bindingNode["stages"].isString() &&
					  (bindingNode["immutable-samplers"].isBool() || bindingNode["immutable-samplers"].isNull()))) {
					std::cout << srcPath << ": Error: Invalid descriptor binding at index " << bindingIndex
							  << " for set " << setIndex << "." << std::endl;
					return;
				}

				auto typeIterator = VkDescriptorTypeFromString.find(bindingNode["type"].asString());
				if (typeIterator == VkDescriptorTypeFromString.end()) {
					std::cout << srcPath << ": Error: Invalid descriptor type at binding index " << bindingIndex
							  << " for set " << setIndex << "." << std::endl;
					return;
				}

				VkShaderStageFlags shaderStageFlags = 0;
				auto stageFlagsString = splitString(bindingNode["stages"].asString());
				for (auto& flag : stageFlagsString) {
					auto flagIterator = VkShaderStageFlagBitsFromString.find(flag);
					if (flagIterator == VkShaderStageFlagBitsFromString.end()) {
						std::cout << srcPath << ": Warning: Invalid shader stage flag bits at binding index "
								  << bindingIndex << " for set " << setIndex << ", ignoring bit." << std::endl;
					} else {
						shaderStageFlags |= flagIterator->second;
					}
				}
				if (!shaderStageFlags) {
					std::cout << srcPath << ": Error: Invalid stage flags at binding index " << bindingIndex
							  << " for set " << setIndex << "." << std::endl;
					return;
				}

				bool usesImmutableSamplers = false;
				if (bindingNode["immutable-samplers"].isBool()) {
					usesImmutableSamplers = bindingNode["immutable-samplers"].asBool();
				}

				VkDescriptorSetLayoutBinding binding = { .binding = bindingNode["binding"].asUInt(),
														 .descriptorType = typeIterator->second,
														 .descriptorCount = bindingNode["count"].asUInt(),
														 .stageFlags = shaderStageFlags,
														 .pImmutableSamplers = nullptr };
				bindingLayoutInfos[setIndex].push_back(
					{ .binding = binding, .usesImmutableSamplers = usesImmutableSamplers });
			}
		}
		++setIndex;
	}

	for (auto& set : bindingLayoutInfos) {
		std::optional<uint32_t> matchingCandidateIndex;
		uint32_t currentIndex = 0;
		for (auto& candidate : setLayoutInfos) {
			bool hasMismatch = false;
			for (auto& binding : set) {
				if (std::find(candidate.begin(), candidate.end(), binding) == candidate.end()) {
					hasMismatch = true;
					break;
				}
			}
			if (!hasMismatch) {
				matchingCandidateIndex = currentIndex;
				break;
			}

			++currentIndex;
		}

		if(matchingCandidateIndex.has_value()) {
			m_setLayoutIndices.push_back(matchingCandidateIndex.value());
		}
		else {
			m_setLayoutIndices.push_back(setLayoutInfos.size());
			setLayoutInfos.push_back(std::move(set));
		}
	}

	if (!archetypeRoot["push-constants"].isNull() && !archetypeRoot["push-constants"].isArray()) {
		std::cout << srcPath << ": Error: Invalid push constants for pipeline archetype." << std::endl;
		return;
	}

	if (!archetypeRoot["push-constants"].isNull()) {
		m_pushConstantRanges.reserve(archetypeRoot["push-constants"].size());
		for (auto& constant : archetypeRoot["push-constants"]) {
			if (!constant.isObject() || !constant["offset"].isUInt() || !constant["size"].isUInt() ||
				!constant["stages"].isString()) {
				std::cout << srcPath << ": Warning: Invalid push constant for pipeline archetype at index "
						  << m_pushConstantRanges.size() << ".\n";
				continue;
			}

			VkShaderStageFlags shaderStageFlags = 0;
			auto stageFlagsString = splitString(constant["stages"].asString());
			for (auto& flag : stageFlagsString) {
				auto flagIterator = VkShaderStageFlagBitsFromString.find(flag);
				if (flagIterator == VkShaderStageFlagBitsFromString.end()) {
					std::cout << srcPath << ": Warning: Invalid shader stage flag bits at push constant index "
							  << m_pushConstantRanges.size() << ", ignoring bit.\n";
				} else {
					shaderStageFlags |= flagIterator->second;
				}
			}

			if (!shaderStageFlags) {
				std::cout << srcPath << ": Error: Invalid stage flags at push constant index "
						  << m_pushConstantRanges.size() << ".\n";
				continue;
			}

			m_pushConstantRanges.push_back({ .stageFlags = shaderStageFlags,
											 .offset = constant["offset"].asUInt(),
											 .size = constant["size"].asUInt() });
		}
	}

	m_isValid = true;
}

void PipelineArchetypeRecord::compileShaders(const std::string& tempDir, const std::string& compilerCommand,
											 const std::vector<std::string>& additionalArgs) {
	m_compilerSubprocessIDs.clear();
	m_compilerSubprocessIDs.reserve(m_files.size());
	for (auto& file : m_files) {
		std::string dstFile;
		switch (file.stage) {
			case VK_SHADER_STAGE_VERTEX_BIT:
				dstFile = tempDir + "/vert.bin";
				break;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				dstFile = tempDir + "/frag.bin";
				break;
			case VK_SHADER_STAGE_COMPUTE_BIT:
				dstFile = tempDir + "/comp.bin";
				break;
			default:
				break;
		}

		std::vector<const char*> args = { file.path.c_str(), "-o", dstFile.c_str() };
		// source file, -o, dst file + command + additional args
		args.reserve(3 + additionalArgs.size());
		for (auto& arg : additionalArgs) {
			args.push_back(arg.c_str());
		}

		m_compilerSubprocessIDs.push_back(startSubprocess(compilerCommand.c_str(), args));
	}
}

std::vector<ReflectedShader> PipelineArchetypeRecord::retrieveCompileResults(const std::string_view& srcPath,
																			 const std::string& tempDir) {
	size_t subprocessIDIndex = 0;

	std::vector<ReflectedShader> shaderModules;
	shaderModules.reserve(m_files.size());

	for (auto& file : m_files) {
		std::string srcFile;
		switch (file.stage) {
			case VK_SHADER_STAGE_VERTEX_BIT:
				srcFile = tempDir + "/vert.bin";
				break;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				srcFile = tempDir + "/frag.bin";
				break;
			case VK_SHADER_STAGE_COMPUTE_BIT:
				srcFile = tempDir + "/comp.bin";
				break;
			default:
				break;
		}

		waitForSubprocess(m_compilerSubprocessIDs[subprocessIDIndex]);

		size_t fileSize;
		void* data = readFile(srcFile.c_str(), &fileSize);

		if (data) {
			m_compiledShaders.push_back({ .stage = file.stage, .data = data, .dataSize = fileSize });

			SpvReflectShaderModule shaderModule;
			spvReflectCreateShaderModule(fileSize, data, &shaderModule);
			shaderModules.push_back({ .stage = file.stage, .shader = shaderModule });
		} else {
			std::cout << srcPath << ": Warning: Compiled file expected but could not be read, did compilation fail?\n";
		}
		++subprocessIDIndex;
	}

	return shaderModules;
}

void PipelineArchetypeRecord::verifyArchetype(const std::string_view& srcPath, std::vector<std::vector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
											  const std::vector<ReflectedShader>& shaders) {
	for (auto& shader : shaders) {
		uint32_t pushConstantCount;
		spvReflectEnumeratePushConstantBlocks(&shader.shader, &pushConstantCount, nullptr);

		auto pushConstants = std::vector<SpvReflectBlockVariable*>(pushConstantCount);
		spvReflectEnumeratePushConstantBlocks(&shader.shader, &pushConstantCount, pushConstants.data());

		for (auto& constant : pushConstants) {
			auto rangeIterator =
				std::find_if(m_pushConstantRanges.begin(), m_pushConstantRanges.end(), [constant](const auto& range) {
					return isFullyInRange(constant->absolute_offset, constant->size, range.offset, range.size);
				});
			if (rangeIterator == m_pushConstantRanges.end()) {
				std::cout << srcPath << ": Error: Unbound push constant range at offset " << constant->absolute_offset
						  << ".\n";
				m_isValid = false;
			} else if (!(rangeIterator->stageFlags & shader.stage)) {
				std::cout << srcPath << ": Error: Wrong usage flag for push constant range at offset "
						  << constant->absolute_offset << ".\n";
				m_isValid = false;
			}
		}

		uint32_t descriptorSetCount;
		spvReflectEnumerateDescriptorSets(&shader.shader, &descriptorSetCount, nullptr);

		auto descriptorSets = std::vector<SpvReflectDescriptorSet*>(descriptorSetCount);
		spvReflectEnumerateDescriptorSets(&shader.shader, &descriptorSetCount, descriptorSets.data());

		uint32_t setIndex = 0;
		for (auto& set : descriptorSets) {
			if (setIndex >= m_setLayoutIndices.size()) {
				std::cout << srcPath << ": Error: Not enough descriptor sets specified.\n";
				m_isValid = false;
				break;
			}
			bool shouldBreak = false;

			auto& bindingInfos = setLayoutInfos[m_setLayoutIndices[setIndex]];
			for (size_t i = 0; i < set->binding_count; ++i) {
				if (i >= bindingInfos.size()) {
					shouldBreak = true;
					break;
				}

				auto& binding = set->bindings[i];
				auto pipelineBindingIterator =
					std::find_if(bindingInfos.begin(), bindingInfos.end(), [setIndex, i](const auto& info) {
						return info.binding.binding == i;
					});
				if (pipelineBindingIterator == bindingInfos.end()) {
					std::cout << srcPath << ": Error: Unbound descriptor at set " << setIndex << ", binding" << i
							  << ".\n";
					m_isValid = false;
					continue;
				}

				auto& pipelineBinding = *pipelineBindingIterator;

				if (static_cast<VkDescriptorType>(binding->descriptor_type) != pipelineBinding.binding.descriptorType) {
					std::cout << srcPath << ": Error: Descriptor type mismatch at set " << setIndex << ", binding" << i
							  << ".\n";
					m_isValid = false;
				}
				if (binding->count != pipelineBinding.binding.descriptorCount) {
					std::cout << srcPath << ": Error: Descriptor count mismatch at set " << setIndex << ", binding" << i
							  << ".\n";
					m_isValid = false;
				}
				if (!(pipelineBinding.binding.stageFlags & shader.stage)) {
					std::cout << srcPath << ": Error: Missing usage flags for descriptor at set " << setIndex
							  << ", binding" << i << ".\n";
					m_isValid = false;
				}
			}
			if (shouldBreak)
				break;

			++setIndex;
		}
	}
}

size_t PipelineArchetypeRecord::serializedSize() const {
	size_t totalSize = 0;

	totalSize += sizeof(uint32_t);
	totalSize += sizeof(uint32_t);
	for (auto& shader : m_compiledShaders) {
		totalSize += sizeof(uint32_t) * 2 + shader.dataSize;
	}

	totalSize += sizeof(uint32_t);
	totalSize += sizeof(uint32_t) * m_setLayoutIndices.size();

	totalSize += sizeof(uint32_t);
	totalSize += sizeof(uint32_t) * 3 * m_pushConstantRanges.size();
	return totalSize;
}

void PipelineArchetypeRecord::serialize(void* data) {
	uint32_t pipelineType = static_cast<uint32_t>(m_pipelineType);
	std::memcpy(data, &pipelineType, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));
	uint32_t shaderCount = static_cast<uint32_t>(m_compiledShaders.size());
	std::memcpy(data, &shaderCount, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));

	for (auto& compiledShader : m_compiledShaders) {
		std::memcpy(data, &compiledShader.stage, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		uint32_t shaderSize = static_cast<uint32_t>(compiledShader.dataSize);
		std::memcpy(data, &shaderSize, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));

		std::memcpy(data, compiledShader.data, compiledShader.dataSize);
		data = offsetVoidPtr(data, compiledShader.dataSize);
	}

	uint32_t setLayoutIndexCount = static_cast<uint32_t>(m_setLayoutIndices.size());
	std::memcpy(data, &setLayoutIndexCount, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));

	for (auto index : m_setLayoutIndices) {
		std::memcpy(data, &index, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
	}

	uint32_t constantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
	std::memcpy(data, &constantRangeCount, sizeof(uint32_t));
	data = offsetVoidPtr(data, sizeof(uint32_t));

	for (auto& range : m_pushConstantRanges) {
		std::memcpy(data, &range.stageFlags, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &range.offset, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
		std::memcpy(data, &range.size, sizeof(uint32_t));
		data = offsetVoidPtr(data, sizeof(uint32_t));
	}
}