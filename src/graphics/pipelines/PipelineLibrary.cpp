#include <fstream>
#include <graphics/pipelines/PipelineLibrary.hpp>
#include <helper/WholeFileReader.hpp>
#include <volk.h>
#include <graphics/helper/ErrorHelper.hpp>

namespace vanadium::graphics {
	void PipelineLibrary::create(const std::string& libraryFileName, DeviceContext* context) {
		m_deviceContext = context;

		m_buffer = readFile(libraryFileName.c_str(), &m_fileSize);
		assertFatal(m_fileSize > 0, "PipelineLibrary: Invalid pipeline library file!\n");
		uint64_t headerReadOffset;
		uint32_t version = readBuffer<uint32_t>(headerReadOffset);

		assertFatal(version == pipelineFileVersion, "PipelineLibrary: Invalid pipeline file version!\n");

		uint32_t pipelineCount = readBuffer<uint32_t>(headerReadOffset);
		uint32_t setLayoutCount = readBuffer<uint32_t>(headerReadOffset);

		std::vector<uint64_t> setLayoutOffsets;
		setLayoutOffsets.reserve(setLayoutCount);
		for (uint32_t i = 0; i < setLayoutCount; ++i) {
			setLayoutOffsets.push_back(readBuffer<uint64_t>(headerReadOffset));
		}

		for (auto& offset : setLayoutOffsets) {
			uint32_t bindingCount = readBuffer<uint32_t>(offset);
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			bindings.reserve(bindingCount);

			for (uint32_t i = 0; i < bindingCount; ++i) {
				VkDescriptorSetLayoutBinding binding = { .binding = readBuffer<uint32_t>(offset),
														 .descriptorCount = readBuffer<uint32_t>(offset),
														 .descriptorType = readBuffer<VkDescriptorType>(offset),
														 .stageFlags = readBuffer<VkShaderStageFlags>(offset) };
				uint32_t immutableSamplerCount = readBuffer<uint32_t>(offset);
				if (immutableSamplerCount > 0) {
					for (uint32_t j = 0; j < immutableSamplerCount; ++j) {
						VkSamplerCreateInfo createInfo = {
							.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
							.magFilter = readBuffer<VkFilter>(offset),
							.minFilter = readBuffer<VkFilter>(offset),
							.mipmapMode = readBuffer<VkSamplerMipmapMode>(offset),
							.addressModeU = readBuffer<VkSamplerAddressMode>(offset),
							.addressModeV = readBuffer<VkSamplerAddressMode>(offset),
							.addressModeW = readBuffer<VkSamplerAddressMode>(offset),
							.mipLodBias = readBuffer<float>(offset),
							.anisotropyEnable = readBuffer<bool>(offset),
							.maxAnisotropy = readBuffer<float>(offset),
							.compareEnable = readBuffer<bool>(offset),
							.compareOp = readBuffer<VkCompareOp>(offset),
							.minLod = readBuffer<float>(offset),
							.maxLod = readBuffer<float>(offset),
							.borderColor = readBuffer<VkBorderColor>(offset),
							.unnormalizedCoordinates = readBuffer<bool>(offset)
						};
						VkSampler sampler;
						verifyResult(vkCreateSampler(m_deviceContext->device(), &createInfo, nullptr, &sampler));
						m_immutableSamplers.push_back(sampler);
					}
					binding.pImmutableSamplers = m_immutableSamplers.data() + m_immutableSamplers.size() - immutableSamplerCount;
				}
			}

			VkDescriptorSetLayoutCreateInfo createInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = bindingCount,
				.pBindings = bindings.data()
			};
			VkDescriptorSetLayout layout;
			verifyResult(vkCreateDescriptorSetLayout(m_deviceContext->device(), &createInfo, nullptr, &layout));
			m_descriptorSetLayouts.push_back(layout);
		}

		headerReadOffset = setLayoutOffsets.back();

		std::vector<uint64_t> pipelineDataOffsets;
		pipelineDataOffsets.reserve(pipelineCount);
		for(size_t i = 0; i < pipelineCount; ++i) {
			pipelineDataOffsets.push_back(readBuffer<uint64_t>(headerReadOffset));
		}

		for(auto& pipelineOffset : pipelineDataOffsets) {
			PipelineType pipelineType = readBuffer<PipelineType>(pipelineOffset);
			switch (pipelineType)
			{
			case PipelineType::Graphics:
				createGraphicsPipeline(pipelineOffset);
				break;
			case PipelineType::Compute:
				createComputePipeline(pipelineOffset);
				break;
			}
		}
	}

	void PipelineLibrary::createGraphicsPipeline(uint64_t& bufferOffset) {
		
	}

	void PipelineLibrary::createComputePipeline(uint64_t& bufferOffset) {

	}
} // namespace vanadium::graphics
