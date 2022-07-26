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
#pragma once

#define VK_NO_PROTOTYPES
#include <util/HashCombine.hpp>
#include <vulkan/vulkan.h>

namespace vanadium::graphics {
	struct AttachmentPassSignature {
		bool isUsed;
		VkFormat format;
		VkSampleCountFlagBits sampleCount;
		bool operator==(const AttachmentPassSignature& other) const {
			if (!isUsed)
				return isUsed == other.isUsed;
			return isUsed == other.isUsed && format == other.format && sampleCount == other.sampleCount;
		}
	};

	struct SubpassSignature {
		std::vector<AttachmentPassSignature> outputAttachments;
		std::vector<AttachmentPassSignature> inputAttachments;
		std::vector<AttachmentPassSignature> resolveAttachments;
		std::vector<AttachmentPassSignature> preserveAttachments;
		AttachmentPassSignature depthStencilAttachment;
		bool operator==(const SubpassSignature& other) const {
			return outputAttachments == other.outputAttachments && inputAttachments == other.inputAttachments &&
				   resolveAttachments == other.resolveAttachments && preserveAttachments == other.preserveAttachments &&
				   depthStencilAttachment == other.depthStencilAttachment;
		}
	};

	struct RenderPassSignature {
		std::vector<SubpassSignature> subpassSignatures;
		std::vector<AttachmentPassSignature> attachmentDescriptionSignatures;
		std::vector<VkSubpassDependency> subpassDependencies;

		bool operator==(const RenderPassSignature& other) const {
			for (auto& dependency : subpassDependencies) {
				if (std::find_if(other.subpassDependencies.begin(), other.subpassDependencies.end(),
								 [dependency](const auto& otherDependency) {
									 return dependency.srcSubpass == otherDependency.srcSubpass &&
											dependency.dstSubpass == otherDependency.dstSubpass &&
											dependency.srcStageMask == otherDependency.srcStageMask &&
											dependency.dstStageMask == otherDependency.dstStageMask &&
											dependency.srcAccessMask == otherDependency.srcAccessMask &&
											dependency.dstAccessMask == otherDependency.dstAccessMask &&
											dependency.dependencyFlags == otherDependency.dependencyFlags;
								 }) == other.subpassDependencies.end())
					return false;
			}
			return subpassSignatures == other.subpassSignatures &&
				   attachmentDescriptionSignatures == other.attachmentDescriptionSignatures;
		}
	};
} // namespace vanadium::graphics

namespace robin_hood {

	template <> struct hash<vanadium::graphics::AttachmentPassSignature> {
		size_t operator()(const vanadium::graphics::AttachmentPassSignature& object) const {
			if (!object.isUsed)
				return 0ULL;
			return hashCombine(hash<VkFormat>()(object.format), hash<VkSampleCountFlagBits>()(object.sampleCount));
		}
	};

	template <> struct hash<std::vector<vanadium::graphics::AttachmentPassSignature>> {
		size_t operator()(const std::vector<vanadium::graphics::AttachmentPassSignature>& object) const {
			size_t value = 0;
			for (const auto& signature : object) {
				value = hashCombine(value, hash<vanadium::graphics::AttachmentPassSignature>()(signature));
			}
			return value;
		}
	};

	template <> struct hash<vanadium::graphics::SubpassSignature> {
		size_t operator()(const vanadium::graphics::SubpassSignature& object) const {
			return hashCombine(hash<decltype(object.inputAttachments)>()(object.inputAttachments),
							   hash<decltype(object.outputAttachments)>()(object.outputAttachments),
							   hash<decltype(object.resolveAttachments)>()(object.resolveAttachments),
							   hash<decltype(object.preserveAttachments)>()(object.preserveAttachments),
							   hash<decltype(object.depthStencilAttachment)>()(object.depthStencilAttachment));
		}

		size_t operator()(const vanadium::graphics::SubpassSignature& object, bool isSingleSubpass) const {
			if (isSingleSubpass) {
				return hashCombine(hash<decltype(object.inputAttachments)>()(object.inputAttachments),
								   hash<decltype(object.outputAttachments)>()(object.outputAttachments),
								   hash<decltype(object.preserveAttachments)>()(object.preserveAttachments));
			} else {
				return operator()(object);
			}
		}
	};

	template <> struct hash<vanadium::graphics::RenderPassSignature> {
		size_t operator()(const vanadium::graphics::RenderPassSignature& object) const {
			size_t subpassHash = 0;
			if (object.subpassSignatures.size() == 1) {
				subpassHash = hash<vanadium::graphics::SubpassSignature>()(object.subpassSignatures[0], true);
			} else {
				for (const auto& signature : object.subpassSignatures) {
					subpassHash = hashCombine(subpassHash, hash<vanadium::graphics::SubpassSignature>()(signature));
				}
			}
			size_t descriptionHash = hash<std::vector<vanadium::graphics::AttachmentPassSignature>>()(
				object.attachmentDescriptionSignatures);
			size_t dependencyHash = 0;
			for (auto& dependency : object.subpassDependencies) {
				dependencyHash =
					hashCombine(dependencyHash, hash<decltype(dependency.srcAccessMask)>()(dependency.srcAccessMask),
								hash<decltype(dependency.srcStageMask)>()(dependency.srcStageMask),
								hash<decltype(dependency.srcSubpass)>()(dependency.srcSubpass),
								hash<decltype(dependency.dstAccessMask)>()(dependency.dstAccessMask),
								hash<decltype(dependency.dstStageMask)>()(dependency.dstStageMask),
								hash<decltype(dependency.dstSubpass)>()(dependency.dstSubpass),
								hash<decltype(dependency.dependencyFlags)>()(dependency.dependencyFlags));
			}
			return hashCombine(subpassHash, descriptionHash, dependencyHash);
		}
	};
} // namespace robin_hood
