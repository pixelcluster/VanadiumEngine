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
#include <PipelineArchetypeRecord.hpp>
#include <algorithm>
#include <iostream>
#include <optional>
#include <util/WholeFileReader.hpp>

// returns true if [offset1;size1] is inside [offset2;size2]
bool isFullyInRange(uint32_t offset1, uint32_t size1, uint32_t offset2, uint32_t size2) {
	return offset2 <= offset1 && (offset2 + size2) >= (offset1 + size1);
}

PipelineArchetypeRecord::PipelineArchetypeRecord(
	const std::string_view& srcPath, const std::string& projectDir,
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
				m_files.push_back({ .stage = VK_SHADER_STAGE_VERTEX_BIT,
									.path = projectDir + "/" + archetypeRoot["vert"].asCString() });
			}
			if (!archetypeRoot["frag"].isString()) {
				std::cout << srcPath << ": Error: Vertex shader node has invalid type." << std::endl;
				return;
			} else {
				m_files.push_back({ .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
									.path = projectDir + "/" + archetypeRoot["frag"].asCString() });
			}
			break;
		case PipelineType::Compute:
			m_files.reserve(1);
			if (!archetypeRoot["comp"].isString()) {
				std::cout << srcPath << ": Error: Vertex shader node has invalid type." << std::endl;
				return;
			} else {
				m_files.push_back({ .stage = VK_SHADER_STAGE_COMPUTE_BIT,
									.path = projectDir + "/" + archetypeRoot["comp"].asCString() });
			}
			break;
	}

	if (!archetypeRoot["sets"].isArray()) {
		std::cout << srcPath << ": Error: Invalid descriptor sets for pipeline archetype." << std::endl;
		return;
	}
	uint32_t setIndex = 0;

	auto bindingLayoutInfos =
		std::vector<std::vector<DescriptorBindingLayoutInfo>>(archetypeRoot["sets"].size());
	m_setLayoutIndices.reserve(archetypeRoot["sets"].size());

	for (auto& set : archetypeRoot["sets"]) {
		if (set["bindings"].isNull()) {
			++setIndex;
			continue;
		} else if (!set["bindings"].isArray()) {
			std::cout << srcPath << ": Error: Invalid descriptor bindings for set " << setIndex << "." << std::endl;
			return;
		} else {
			bindingLayoutInfos[setIndex].reserve(set["bindings"].size());
			uint32_t bindingIndex = 0;
			for (auto& bindingNode : set["bindings"]) {
				if (!(bindingNode["binding"].isUInt() && bindingNode["type"].isString() &&
					  bindingNode["count"].isUInt() && bindingNode["stages"].isString() &&
					  (bindingNode["immutable-samplers"].isArray() || bindingNode["immutable-samplers"].isNull()))) {
					std::cout << srcPath << ": Error: Invalid descriptor binding at index " << bindingIndex
							  << " for set " << setIndex << "." << std::endl;
					return;
				}

				VkDescriptorType type = VkDescriptorTypeFromString(bindingNode["type"].asCString());
				if (type == static_cast<VkDescriptorType>(~0U)) {
					std::cout << srcPath << ": Error: Invalid descriptor type at binding index " << bindingIndex
							  << " for set " << setIndex << "." << std::endl;
					return;
				}

				VkShaderStageFlags shaderStageFlags = 0;
				auto stageFlagsString = splitString(bindingNode["stages"].asString());
				for (auto& flag : stageFlagsString) {
					VkShaderStageFlagBits flagBit = VkShaderStageFlagBitsFromString(flag);
					if (flagBit == static_cast<VkShaderStageFlagBits>(~0U)) {
						std::cout << srcPath << ": Warning: Invalid shader stage flag bits at binding index "
								  << bindingIndex << " for set " << setIndex << ", ignoring bit." << std::endl;
					} else {
						shaderStageFlags |= flagBit;
					}
				}
				if (!shaderStageFlags) {
					std::cout << srcPath << ": Error: Invalid stage flags at binding index " << bindingIndex
							  << " for set " << setIndex << "." << std::endl;
					return;
				}

				bool usesImmutableSamplers = false;
				std::vector<SamplerInfo> samplerInfos;
				if (bindingNode["immutable-samplers"].isArray()) {
					usesImmutableSamplers = !bindingNode["immutable-samplers"].empty();
					samplerInfos.reserve(bindingNode["immutable-samplers"].size());
					for (auto& node : bindingNode["immutable-samplers"]) {
						VkFilter magFilter = VkFilterFromString(asCStringOr(node, "mag-filter", "VK_FILTER_NEAREST"));
						if (magFilter == static_cast<VkFilter>(~0U)) {
							std::cout
								<< srcPath
								<< ": Warning: Invalid magnification filter! Choosing Nearest, might cause errors...\n";
							magFilter = VK_FILTER_NEAREST;
						}

						VkFilter minFilter = VkFilterFromString(asCStringOr(node, "min-filter", "VK_FILTER_NEAREST"));
						if (minFilter == static_cast<VkFilter>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid minification filter! Choosing Nearest, might cause "
										 "unintended behaviour...\n";
							minFilter = VK_FILTER_NEAREST;
						}

						VkSamplerMipmapMode mipmapMode = VkSamplerMipmapModeFromString(
							asCStringOr(node, "mipmap-mode", "VK_SAMPLER_MIPMAP_MODE_NEAREST"));
						if (mipmapMode == static_cast<VkSamplerMipmapMode>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid mipmap mode! Choosing Nearest, might cause unintended "
										 "behaviour...\n";
							mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
						}

						VkSamplerAddressMode addressModeU = VkSamplerAddressModeFromString(
							asCStringOr(node, "address-mode-u", "VK_SAMPLER_ADDRESS_MODE_REPEAT"));
						if (addressModeU == static_cast<VkSamplerAddressMode>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid U address mode! Choosing Repeat, might cause unintended "
										 "behaviour...\n";
							addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						}

						VkSamplerAddressMode addressModeV = VkSamplerAddressModeFromString(
							asCStringOr(node, "address-mode-v", "VK_SAMPLER_ADDRESS_MODE_REPEAT"));
						if (addressModeV == static_cast<VkSamplerAddressMode>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid V address mode! Choosing Repeat, might cause unintended "
										 "behaviour...\n";
							addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						}

						VkSamplerAddressMode addressModeW = VkSamplerAddressModeFromString(
							asCStringOr(node, "address-mode-w", "VK_SAMPLER_ADDRESS_MODE_REPEAT"));
						if (addressModeW == static_cast<VkSamplerAddressMode>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid W address mode! Choosing Repeat, might cause unintended "
										 "behaviour...\n";
							addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						}

						VkCompareOp compareOp =
							VkCompareOpFromString(asCStringOr(node, "compare-op", "VK_COMPARE_OP_ALWAYS"));
						if (compareOp == static_cast<VkCompareOp>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid sampler compare operation! Choosing Always, might cause "
										 "unintended "
										 "behaviour...\n";
							compareOp = VK_COMPARE_OP_ALWAYS;
						}

						VkBorderColor borderColor = VkBorderColorFromString(
							asCStringOr(node, "border-color", "VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK"));
						if (borderColor == static_cast<VkBorderColor>(~0U)) {
							std::cout << srcPath
									  << ": Warning: Invalid sampler border color! Choosing transparent black (float), "
										 "might cause unintended "
										 "behaviour...\n";
							borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
						}

						samplerInfos.push_back({
							.magFilter = magFilter,
							.minFilter = minFilter,
							.mipmapMode = mipmapMode,
							.addressModeU = addressModeU,
							.addressModeV = addressModeV,
							.addressModeW = addressModeW,
							.mipLodBias = asFloatOr(node, "mip-lod-bias", 0.0f),
							.anisotropyEnable = asBoolOr(node, "anisotropy-enable", false),
							.maxAnisotropy = asFloatOr(node, "max-anisotropy", 0.0f),
							.compareEnable = asBoolOr(node, "compare-enable", false),
							.compareOp = compareOp,
							.minLod = asFloatOr(node, "min-lod", 0.0f),
							.maxLod = asFloatOr(node, "max-lod", 999999.0f),
							.borderColor = borderColor,
							.unnormalizedCoordinates = asBoolOr(node, "unnormalized-coordinates", false),
						});
					}
				}

				VkDescriptorSetLayoutBinding binding = { .binding = bindingNode["binding"].asUInt(),
														 .descriptorType = type,
														 .descriptorCount = bindingNode["count"].asUInt(),
														 .stageFlags = shaderStageFlags,
														 .pImmutableSamplers = nullptr };
				bindingLayoutInfos[setIndex].push_back({ .binding = binding,
														 .usesImmutableSamplers = usesImmutableSamplers,
														 .immutableSamplerInfos = samplerInfos });
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

		if (matchingCandidateIndex.has_value()) {
			m_setLayoutIndices.push_back(matchingCandidateIndex.value());
		} else {
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
				auto flagBit = VkShaderStageFlagBitsFromString(flag);
				if (flagBit == static_cast<VkShaderStageFlagBits>(~0U)) {
					std::cout << srcPath << ": Warning: Invalid shader stage flag bits at push constant index "
							  << m_pushConstantRanges.size() << ", ignoring bit.\n";
				} else {
					shaderStageFlags |= flagBit;
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
			m_compiledShaders.push_back({ .stage = file.stage, .dataSize = fileSize, .data = data });

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

void PipelineArchetypeRecord::verifyArchetype(
	const std::string_view& srcPath,
	std::vector<std::vector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
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
						  << " (size " << constant->size << ")"
						  << ".\n";
				m_isValid = false;
			} else if (!(rangeIterator->stageFlags & shader.stage)) {
				std::cout << srcPath << ": Error: Missing stage flags for push constant range at offset "
						  << constant->absolute_offset << ".\n";
				m_isValid = false;
			}
		}

		uint32_t descriptorSetCount;
		spvReflectEnumerateDescriptorSets(&shader.shader, &descriptorSetCount, nullptr);

		auto descriptorSets = std::vector<SpvReflectDescriptorSet*>(descriptorSetCount);
		spvReflectEnumerateDescriptorSets(&shader.shader, &descriptorSetCount, descriptorSets.data());

		for (auto& set : descriptorSets) {
			if (set->set >= m_setLayoutIndices.size()) {
				std::cout << srcPath << ": Error: Not enough descriptor sets specified.\n";
				m_isValid = false;
				break;
			}
			bool shouldBreak = false;

			auto& bindingInfos = setLayoutInfos[m_setLayoutIndices[set->set]];
			for (size_t i = 0; i < set->binding_count; ++i) {
				auto& binding = set->bindings[i];
				auto pipelineBindingIterator =
					std::find_if(bindingInfos.begin(), bindingInfos.end(),
								 [binding](const auto& info) { return info.binding.binding == binding->binding; });
				if (pipelineBindingIterator == bindingInfos.end()) {
					std::cout << srcPath << ": Error: Unbound descriptor at set " << set->set << ", binding " << i
							  << ".\n";
					m_isValid = false;
					continue;
				}

				auto& pipelineBinding = *pipelineBindingIterator;

				if (static_cast<VkDescriptorType>(binding->descriptor_type) != pipelineBinding.binding.descriptorType) {
					std::cout << srcPath << ": Error: Descriptor type mismatch at set " << set->set << ", binding " << i
							  << ".\n";
					m_isValid = false;
				}
				if (binding->count != pipelineBinding.binding.descriptorCount) {
					std::cout << srcPath << ": Error: Descriptor count mismatch at set " << set->set << ", binding "
							  << i << ".\n";
					m_isValid = false;
				}
				if (!(pipelineBinding.binding.stageFlags & shader.stage)) {
					std::cout << srcPath << ": Error: Missing stage flags for descriptor at set " << set->set
							  << ", binding " << i << ".\n";
					m_isValid = false;
				}
			}
			if (shouldBreak)
				break;
		}
	}
}

void PipelineArchetypeRecord::serialize(std::ofstream& outStream) {
	::serialize(m_pipelineType, outStream);
	serializeVector(m_compiledShaders, outStream);
	serializeVector(m_setLayoutIndices, outStream);
	serializeVector(m_pushConstantRanges, outStream);
}

void PipelineArchetypeRecord::freeShaders() {
	for (auto& shader : m_compiledShaders) {
		delete[] reinterpret_cast<char*>(shader.data);
	}
}
