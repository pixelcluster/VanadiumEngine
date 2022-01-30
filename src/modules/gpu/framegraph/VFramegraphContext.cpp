#include <modules/gpu/framegraph/VFramegraphContext.hpp>
#include <modules/gpu/framegraph/VFramegraphRenderingNode.hpp>

void VFramegraphContext::declareCreatedBuffer(VFramegraphNode* creator, const std::string_view& name,
											  const VFramegraphNodeBufferUsage& usage) {
	m_buffers.insert(
		std::pair<std::string, VFramegraphBufferResource>(name, VFramegraphBufferResource{ .creator = creator }));
	m_bufferUsages.insert(std::pair<std::string, VFramegraphNodeBufferUsage>(name, usage));
}

void VFramegraphContext::declareCreatedImage(VFramegraphNode* creator, const std::string_view& name,
											 const VFramegraphNodeImageUsage& usage) {
	m_images.insert(
		std::pair<std::string, VFramegraphImageResource>(name, VFramegraphImageResource{ .creator = creator }));
	m_imageUsages.insert(std::pair<std::string, VFramegraphNodeImageUsage>(name, usage));
}

void VFramegraphContext::declareImportedBuffer(const std::string_view& name, VBufferResourceHandle handle,
											   const VFramegraphNodeBufferUsage& usage) {
	m_buffers.insert(std::pair<std::string, VFramegraphBufferResource>(
		name, VFramegraphBufferResource{ .bufferResourceHandle = handle }));
	m_bufferUsages.insert(std::pair<std::string, VFramegraphNodeBufferUsage>(name, usage));
}

void VFramegraphContext::declareImportedImage(const std::string_view& name, VImageResourceHandle handle,
											  const VFramegraphNodeImageUsage& usage) {
	m_images.insert(std::pair<std::string, VFramegraphImageResource>(
		name, VFramegraphImageResource{ .imageResourceHandle = handle }));
	m_imageUsages.insert(std::pair<std::string, VFramegraphNodeImageUsage>(name, usage));
}

void VFramegraphContext::declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
												 const VFramegraphNodeBufferUsage& usage) {
	auto nodeIterator = std::find(m_nodes.begin(), m_nodes.end(), user);
	if (nodeIterator == m_nodes.end()) {
		// TODO: log invalid node for dependency
		return;
	}

	auto usageIterator = m_bufferUsages.find(std::string(name));
	if (usageIterator == m_bufferUsages.end()) {
		// TODO: log invalid resource for dependency
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	m_nodeBufferDependencies[nodeIndex].push_back({ .resourceName = std::string(name),
													.srcStages = usageIterator->second.pipelineStages,
													.dstStages = usage.pipelineStages,
													.srcAccesses = usageIterator->second.accessTypes,
													.dstAccesses = usage.accessTypes });
	usageIterator->second = usage;
}

void VFramegraphContext::declareReferencedImage(VFramegraphNode* user, const std::string_view& name,
												const VFramegraphNodeImageUsage& usage) {
	auto nodeIterator = std::find(m_nodes.begin(), m_nodes.end(), user);
	if (nodeIterator == m_nodes.end()) {
		// TODO: log invalid node for dependency
		return;
	}

	auto usageIterator = m_imageUsages.find(std::string(name));
	if (usageIterator == m_imageUsages.end()) {
		// TODO: log invalid resource for dependency
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	m_nodeImageDependencies[nodeIndex].push_back({ .resourceName = std::string(name),
									.srcStages = usageIterator->second.pipelineStages,
									.dstStages = usage.pipelineStages,
									.srcAccesses = usageIterator->second.accessTypes,
									.dstAccesses = usage.accessTypes,
									.oldLayout = usageIterator->second.finishLayout,
									.newLayout = usage.startLayout });
	usageIterator->second = usage;
}