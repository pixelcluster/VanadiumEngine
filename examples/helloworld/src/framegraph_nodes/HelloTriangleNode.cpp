#include <framegraph_nodes/HelloTriangleNode.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

#include <helper/WholeFileReader.hpp>

void HelloTriangleNode::setupResources(VFramegraphContext* context) {
	VkAttachmentDescription description = { .format = VK_FORMAT_B8G8R8A8_SRGB,
											.samples = VK_SAMPLE_COUNT_1_BIT,
											.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
											.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
											.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
											.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

	VkAttachmentReference reference = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
												.colorAttachmentCount = 1,
												.pColorAttachments = &reference };

	VkRenderPassCreateInfo renderPassCreateInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
													.attachmentCount = 1,
													.subpassCount = 1,
													.pSubpasses = &subpassDescription };
	verifyResult(vkCreateRenderPass(context->gpuContext()->device(), &renderPassCreateInfo, nullptr, &m_renderPass));

	VkPipelineLayoutCreateInfo layoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	verifyResult(
		vkCreatePipelineLayout(context->gpuContext()->device(), &layoutCreateInfo, nullptr, &m_pipelineLayout));

	VkVertexInputAttributeDescription attributeDescriptions[] = {
		// positions
		{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0 },
		// colors
		{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 2 * sizeof(float) }
	};
	VkVertexInputBindingDescription bindingDescription = { .binding = 0,
														   .stride = 5 * sizeof(float),
														   .inputRate = VK_VERTEX_INPUT_RATE_VERTEX };

	VkPipelineVertexInputStateCreateInfo inputStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = attributeDescriptions
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	};

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE
	};

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
	};

	VkPipelineColorBlendAttachmentState attachmentBlendState = { .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
																				   VK_COLOR_COMPONENT_G_BIT |
																				   VK_COLOR_COMPONENT_B_BIT |
																				   VK_COLOR_COMPONENT_A_BIT };

	VkPipelineColorBlendStateCreateInfo blendStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &attachmentBlendState
	};

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStates
	};

	size_t shaderSize;
	uint32_t* shaderData = reinterpret_cast<uint32_t*>(readFile("./shaders/triangle-vertex.spv", &shaderSize));

	VkShaderModuleCreateInfo shaderModuleCreateInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
														.codeSize = shaderSize,
														.pCode = shaderData };
	VkShaderModule vertexShaderModule;
	verifyResult(
		vkCreateShaderModule(context->gpuContext()->device(), &shaderModuleCreateInfo, nullptr, &vertexShaderModule));

	free(shaderData);

	shaderData = reinterpret_cast<uint32_t*>(readFile("./shaders/triangle-fragment.spv", &shaderSize));
	shaderModuleCreateInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
							   .codeSize = shaderSize,
							   .pCode = shaderData };
	VkShaderModule fragmentShaderModule;
	verifyResult(
		vkCreateShaderModule(context->gpuContext()->device(), &shaderModuleCreateInfo, nullptr, &fragmentShaderModule));

	free(shaderData);

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
		{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage = VK_SHADER_STAGE_VERTEX_BIT,
		  .module = vertexShaderModule,
		  .pName = "main" },
		{ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		  .module = fragmentShaderModule,
		  .pName = "main" },
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
														.stageCount = 2,
														.pStages = shaderStageCreateInfos,
														.pVertexInputState = &inputStateCreateInfo,
														.pInputAssemblyState = &inputAssemblyCreateInfo,
														.pRasterizationState = &rasterizationStateCreateInfo,
														.pMultisampleState = &multisampleStateCreateInfo,
														.pDepthStencilState = &depthStencilStateCreateInfo,
														.pColorBlendState = &blendStateCreateInfo,
														.pDynamicState = &dynamicStateCreateInfo,
														.layout = m_pipelineLayout,
														.renderPass = m_renderPass,
														.subpass = 0,
														.basePipelineIndex = -1 };
	verifyResult(vkCreateGraphicsPipelines(context->gpuContext()->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo,
										   nullptr, &m_graphicsPipeline));
}

void HelloTriangleNode::recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) {}

void HelloTriangleNode::handleWindowResize(VFramegraphContext* context) {}
