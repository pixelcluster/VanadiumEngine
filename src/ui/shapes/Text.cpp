#include <CharacterGroup.hpp>
#include <Log.hpp>
#include <graphics/helper/DebugHelper.hpp>
#include <math/Matrix.hpp>
#include <ui/shapes/Text.hpp>
#include <ui/util/BreakClassRule.hpp>
#include <volk.h>

namespace vanadium::ui::shapes {

	TextShapeRegistry::TextShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context,
										 VkRenderPass uiRenderPass,
										 const graphics::RenderPassSignature& uiRenderPassSignature,
										 const std::string_view pipelineName) {
		m_textPipelineID = context.pipelineLibrary->findGraphicsPipeline(pipelineName);

		graphics::DescriptorSetLayoutInfo textLayoutInfo =
			context.pipelineLibrary->graphicsPipelineSet(m_textPipelineID, 0);
		m_textSetAllocationInfo = { .layout = textLayoutInfo.layout };
		m_textSetAllocationInfo.typeInfos.reserve(textLayoutInfo.bindingInfos.size());
		for (auto& info : textLayoutInfo.bindingInfos) {
			m_textSetAllocationInfo.typeInfos.push_back({ .type = info.descriptorType, .count = info.descriptorCount });
		}

		m_atlasViewInfo = { .viewType = VK_IMAGE_VIEW_TYPE_2D,
							.components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
											.g = VK_COMPONENT_SWIZZLE_IDENTITY,
											.b = VK_COMPONENT_SWIZZLE_IDENTITY,
											.a = VK_COMPONENT_SWIZZLE_IDENTITY },
							.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
												  .baseMipLevel = 0,
												  .levelCount = 1,
												  .baseArrayLayer = 0,
												  .layerCount = 1 } };

		context.pipelineLibrary->createForPass(uiRenderPassSignature, uiRenderPass, { m_textPipelineID });
		m_renderContext = context;
		m_uiSubsystem = subsystem;
	}

	void TextShapeRegistry::addShape(Shape* shape) {
		TextShape* textShape = reinterpret_cast<TextShape*>(shape);

		FontAtlasIdentifier identifier = { .fontID = textShape->fontID(), .pointSize = textShape->pointSize() };

		determineLineBreaksAndDimensions(textShape);

		if (m_fontAtlases.find(identifier) == m_fontAtlases.end()) {
			std::vector<graphics::DescriptorSetAllocationInfo> infos =
				std::vector<graphics::DescriptorSetAllocationInfo>(graphics::frameInFlightCount,
																   m_textSetAllocationInfo);
			auto allocations = m_renderContext.descriptorSetAllocator->allocateDescriptorSets(infos);
			for (uint32_t i = 0; i < graphics::frameInFlightCount; ++i) {
				m_fontAtlases[identifier].setAllocations[i] = allocations[i];
			}
		}

		m_fontAtlases[identifier].dirtyFlag = true;
		m_fontAtlases[identifier].referencingShapes.push_back(textShape);
		textShape->setInternalRegistry(this);
		m_shapes.push_back(textShape);

		m_maxLayer = std::max(m_maxLayer, shape->layerIndex());
	}

	void TextShapeRegistry::removeShape(Shape* shape) {
		TextShape* textShape = reinterpret_cast<TextShape*>(shape);

		if (!textShape->textDirtyFlag()) {
			FontAtlasIdentifier identifier = { .fontID = textShape->fontID(), .pointSize = textShape->pointSize() };
			m_fontAtlases[identifier].referencingShapes.erase(
				std::find(m_fontAtlases[identifier].referencingShapes.begin(),
						  m_fontAtlases[identifier].referencingShapes.end(), textShape));
			m_fontAtlases[identifier].dirtyFlag = true;
			if (m_fontAtlases[identifier].referencingShapes.empty())
				destroyAtlas(identifier);
		} else {
			for (auto& [key, atlas] : m_fontAtlases) {
				auto iterator = std::find(atlas.referencingShapes.begin(), atlas.referencingShapes.end(), textShape);
				if (iterator != atlas.referencingShapes.end()) {
					atlas.referencingShapes.erase(iterator);
					atlas.dirtyFlag = true;
					if (atlas.referencingShapes.empty())
						destroyAtlas(key);
					break;
				}
			}
		}
		auto iterator = std::find(m_shapes.begin(), m_shapes.end(), textShape);
		if (iterator != m_shapes.end())
			m_shapes.erase(iterator);
	}

	void TextShapeRegistry::prepareFrame(uint32_t frameIndex) {
		m_maxLayer = 0;
		for (auto& shape : m_shapes) {
			FontAtlasIdentifier identifier = { .fontID = shape->fontID(), .pointSize = shape->pointSize() };
			if (shape->textDirtyFlag()) {
				for (auto& [key, atlas] : m_fontAtlases) {
					auto iterator = std::find(atlas.referencingShapes.begin(), atlas.referencingShapes.end(), shape);
					if (iterator != atlas.referencingShapes.end()) {
						atlas.referencingShapes.erase(iterator);
						atlas.dirtyFlag = true;
						break;
					}
				}

				if (m_fontAtlases.find(identifier) == m_fontAtlases.end()) {
					std::vector<graphics::DescriptorSetAllocationInfo> infos =
						std::vector<graphics::DescriptorSetAllocationInfo>(graphics::frameInFlightCount,
																		   m_textSetAllocationInfo);
					auto allocations = m_renderContext.descriptorSetAllocator->allocateDescriptorSets(infos);
					for (uint32_t i = 0; i < graphics::frameInFlightCount; ++i) {
						m_fontAtlases[identifier].setAllocations[i] = allocations[i];
					}
				}

				m_fontAtlases[identifier].dirtyFlag = true;
				m_fontAtlases[identifier].referencingShapes.push_back(shape);
			} else if (shape->dirtyFlag()) {
				m_fontAtlases[identifier].bufferDirtyFlag = true;
			}
			m_maxLayer = std::max(shape->layerIndex(), m_maxLayer);
		}

	atlasTrim:
		for (auto& [key, atlas] : m_fontAtlases) {
			if (atlas.referencingShapes.empty()) {
				destroyAtlas(key);
				goto atlasTrim; // iterator invalidation, have to retry
			}
		}

		for (auto& [key, atlas] : m_fontAtlases) {
			if (atlas.dirtyFlag) {
				regenerateFontAtlas(key, frameIndex);
				regenerateGlyphData(key, frameIndex);
				if (atlas.glyphDataTransfer != ~0U)
					m_renderContext.transferManager->updateTransferData(atlas.glyphDataTransfer, frameIndex,
																		atlas.glyphData.data());

				updateAtlasDescriptors(key, frameIndex);
			} else if (atlas.bufferDirtyFlag) {
				regenerateGlyphData(key, frameIndex);
				m_renderContext.transferManager->updateTransferData(atlas.glyphDataTransfer, frameIndex,
																	atlas.glyphData.data());

				updateAtlasDescriptors(key, frameIndex);
			} else {
				if (atlas.lastUpdateFrameIndex != ~0U) {
					if (atlas.lastUpdateFrameIndex == frameIndex) {
						atlas.lastUpdateFrameIndex = ~0U;
					} else {

						m_renderContext.transferManager->updateTransferData(atlas.glyphDataTransfer, frameIndex,
																			atlas.glyphData.data());
					}
				}

				if (atlas.lastRecreateFrameIndex != ~0U) {
					if (atlas.lastRecreateFrameIndex == frameIndex) {
						atlas.lastRecreateFrameIndex = ~0U;
					} else {
						updateAtlasDescriptors(key, frameIndex);
					}
				}
			}

			for (auto& shape : atlas.referencingShapes) {
				shape->clearDirtyFlag();
				shape->clearTextDirtyFlag();
			}
			atlas.dirtyFlag = false;
			atlas.bufferDirtyFlag = false;
		}
	}

	void TextShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t layerIndex,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		auto scissorRect = m_uiSubsystem->layerScissor(layerIndex);

		if (scissorRect.extent.width == 0 && scissorRect.extent.height == 0) {
			scissorRect.extent = { m_renderContext.targetSurface->properties().width,
								   m_renderContext.targetSurface->properties().height };
			scissorRect.offset = {};
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						  m_renderContext.pipelineLibrary->graphicsPipeline(m_textPipelineID, uiRenderPassSignature));
		VkViewport viewport = { .width = static_cast<float>(m_renderContext.targetSurface->properties().width),
								.height = static_cast<float>(m_renderContext.targetSurface->properties().height),
								.minDepth = 0.0f,
								.maxDepth = 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

		VkShaderStageFlags stageFlags =
			m_renderContext.pipelineLibrary->graphicsPipelinePushConstantRanges(m_textPipelineID)[0].stageFlags;

		for (auto& [key, atlas] : m_fontAtlases) {
			if (layerIndex >= atlas.layers.size() || atlas.layers[layerIndex].elementCount == 0) {
				continue;
			}
			PushConstantData constantData = { .targetDimensions =
												  Vector2(m_renderContext.targetSurface->properties().width,
														  m_renderContext.targetSurface->properties().height),
											  .instanceOffset = atlas.layers[layerIndex].offset };
			vkCmdPushConstants(commandBuffer, m_renderContext.pipelineLibrary->graphicsPipelineLayout(m_textPipelineID),
							   stageFlags, 0, sizeof(PushConstantData), &constantData);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
									m_renderContext.pipelineLibrary->graphicsPipelineLayout(m_textPipelineID), 0, 1,
									&atlas.setAllocations[frameIndex].set, 0, nullptr);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(6U * atlas.layers[layerIndex].elementCount), 1, 0, 0);
		}
	}

	void TextShapeRegistry::destroy(const graphics::RenderPassSignature&) {
		for (auto& shape : m_shapes) {
			delete shape;
		}
		for (auto& atlas : m_fontAtlases) {
			destroyAtlas(atlas.first);
		}
		for (auto& fontGroup : m_fonts) {
			for (auto& [key, font] : fontGroup.fonts) {
				hb_font_destroy(font);
			}
			hb_face_destroy(fontGroup.fontFace);
		}
	}

	void TextShapeRegistry::regenerateFontAtlas(const FontAtlasIdentifier& identifier, uint32_t frameIndex) {
		FT_Face face = m_uiSubsystem->fontLibrary().fontFace(identifier.fontID);
		robin_hood::unordered_set<uint32_t> usedGlyphs;

		hb_position_t oneLinePenX = 0;
		uint32_t maxGlyphHeight = 0;
		size_t totalGlyphCount = 0;
		hb_codepoint_t previousGlyphIndex = 0;

		m_fontAtlases[identifier].shapeGlyphData.clear();
		m_fontAtlases[identifier].fontAtlasPositions.clear();
		m_fontAtlases[identifier].maxGlyphHeight = 0;

		for (auto& shape : m_fontAtlases[identifier].referencingShapes) {
			FT_Set_Char_Size(face, shape->pointSize() * 64.0f, 0, m_uiSubsystem->monitorDPIX(),
							 m_uiSubsystem->monitorDPIY());

			unsigned int glyphCount = 0;
			hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(shape->internalTextBuffer(), &glyphCount);
			hb_glyph_position_t* shapeGlyphPositions =
				hb_buffer_get_glyph_positions(shape->internalTextBuffer(), &glyphCount);

			hb_position_t penX = 0;
			hb_position_t penY = 0;
			previousGlyphIndex = 0;

			std::string_view textView = shape->text();

			for (unsigned int i = 0; i < glyphCount; ++i) {
				hb_codepoint_t glyphID = glyphInfos[i].codepoint;
				FT_Load_Glyph(face, glyphID, FT_LOAD_DEFAULT);

				hb_position_t xOffset = (shapeGlyphPositions[i].x_offset + face->glyph->metrics.horiBearingX) / 64;
				hb_position_t yOffset = (shapeGlyphPositions[i].y_offset) / 64;
				hb_position_t xAdvance = shapeGlyphPositions[i].x_advance / 64;
				hb_position_t yAdvance = shapeGlyphPositions[i].y_advance / 64;

				if (i > 0 && std::find(shape->linebreakGlyphIndices().begin(), shape->linebreakGlyphIndices().end(),
									   i - 1) != shape->linebreakGlyphIndices().end()) {
					penY += face->size->metrics.height / 64;
					penX = 0;
				} else if (previousGlyphIndex) {
					FT_Vector kerningDelta;
					FT_Get_Kerning(face, previousGlyphIndex, glyphInfos[i].codepoint, FT_KERNING_DEFAULT,
								   &kerningDelta);
					penX += kerningDelta.x / 64;
				}

				// skip over line control characters
				BreakClass breakClass = codepointBreakClass(
					utf8Codepoint(textView.data() + glyphInfos[i].cluster, textView.size() - glyphInfos[i].cluster));
				if (breakClass == BreakClass::CR || breakClass == BreakClass::LF || breakClass == BreakClass::BK)
					continue;

				m_fontAtlases[identifier].shapeGlyphData.push_back(
					{ .referencedShape = shape,
					  .glyphIndex = glyphID,
					  .offset = Vector2(penX + xOffset, penY + yOffset),
					  .size = Vector2(face->glyph->metrics.width / 64, face->glyph->metrics.height / 64),
					  .bearingY = static_cast<uint32_t>(face->glyph->metrics.horiBearingY / 64) });
				usedGlyphs.insert(glyphID);

				penX += xAdvance;
				penY += yAdvance;

				oneLinePenX += xAdvance;
				maxGlyphHeight = std::max(maxGlyphHeight, static_cast<uint32_t>(face->size->metrics.height));

				m_fontAtlases[identifier].maxGlyphHeight =
					std::max(m_fontAtlases[identifier].maxGlyphHeight,
							 static_cast<uint32_t>(face->glyph->metrics.horiBearingY / 64));

				++totalGlyphCount;
				previousGlyphIndex = glyphInfos[i].codepoint;
			}
		}

		std::sort(m_fontAtlases[identifier].shapeGlyphData.begin(), m_fontAtlases[identifier].shapeGlyphData.end(),
				  [](const auto& first, const auto& second) {
					  return first.referencedShape->layerIndex() < second.referencedShape->layerIndex();
				  });

		m_fontAtlases[identifier].layers.clear();

		if (m_fontAtlases[identifier].shapeGlyphData.empty())
			return;

		m_fontAtlases[identifier].layers.reserve(m_maxLayer + 1);
		for (uint32_t i = 0; i <= m_maxLayer; ++i) {
			auto layerBegin = std::lower_bound(
				m_fontAtlases[identifier].shapeGlyphData.begin(), m_fontAtlases[identifier].shapeGlyphData.end(), i,
				[](const auto& one, const auto& layer) { return one.referencedShape->layerIndex() < layer; });
			auto layerEnd = std::lower_bound(
				m_fontAtlases[identifier].shapeGlyphData.begin(), m_fontAtlases[identifier].shapeGlyphData.end(), i + 1,
				[](const auto& one, const auto& layer) { return one.referencedShape->layerIndex() < layer; });
			m_fontAtlases[identifier].layers.push_back(
				{ .offset = static_cast<uint32_t>(layerBegin - m_fontAtlases[identifier].shapeGlyphData.begin()),
				  .elementCount = static_cast<uint32_t>(layerEnd - layerBegin) });
		}

		uint32_t approximateTextureDimensions = std::sqrt(oneLinePenX * maxGlyphHeight);

		uint32_t pixelPenX = 0;
		uint32_t pixelPenY = 0;

		uint32_t maxLineHeight = 0;

		for (auto& glyphIndex : usedGlyphs) {
			FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);

			// 1-pixel margin on the right for linear filtering
			uint32_t pixelWidth = face->glyph->metrics.width / 64 + 1;
			// 1-pixel margin on the bottom for linear filtering
			uint32_t pixelHeight = face->glyph->metrics.height / 64 + 1;

			// If few characters are in the atlas, the width of the texture might be smaller than the width of a glyph
			approximateTextureDimensions = std::max(pixelWidth, approximateTextureDimensions);

			if (pixelPenX + pixelWidth > approximateTextureDimensions) {
				pixelPenY += maxLineHeight;
				maxLineHeight = pixelHeight;
				pixelPenX = 0;
			}

			m_fontAtlases[identifier].fontAtlasPositions[glyphIndex] = { .position = Vector2(pixelPenX, pixelPenY),
																		 .size =
																			 Vector2(pixelWidth - 1, pixelHeight - 1) };

			pixelPenX += pixelWidth;
			maxLineHeight = std::max(pixelHeight, maxLineHeight);
		}

		uint32_t atlasWidth = approximateTextureDimensions;
		uint32_t atlasHeight = pixelPenY + maxLineHeight;

		char* atlasData = new char[atlasWidth * atlasHeight];
		std::memset(atlasData, 0, atlasWidth * atlasHeight);
		for (auto& [glyphIndex, glyphCoord] : m_fontAtlases[identifier].fontAtlasPositions) {
			FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);
			char* glyphOffset = atlasData + static_cast<uint32_t>(glyphCoord.position.y) * atlasWidth +
								static_cast<uint32_t>(glyphCoord.position.x);
			for (unsigned int row = 0; row < face->glyph->bitmap.rows; ++row) {
				std::memcpy(glyphOffset + row * atlasWidth,
							face->glyph->bitmap.buffer + row * face->glyph->bitmap.pitch, face->glyph->bitmap.width);
			}
		}

		m_fontAtlases[identifier].glyphData.reserve(totalGlyphCount);
		m_fontAtlases[identifier].atlasSize = Vector2(atlasWidth, atlasHeight);

		VkImageCreateInfo atlasImageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8_UNORM,
			.extent = { .width = atlasWidth, .height = atlasHeight, .depth = 1U },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		if (m_fontAtlases[identifier].fontAtlasImage != ~0U) {
			m_renderContext.resourceAllocator->destroyImage(m_fontAtlases[identifier].fontAtlasImage);
		}
		m_fontAtlases[identifier].fontAtlasImage =
			m_renderContext.resourceAllocator->createImage(atlasImageCreateInfo, {}, { .deviceLocal = true });
		m_fontAtlases[identifier].lastRecreateFrameIndex = frameIndex;
		m_renderContext.transferManager->submitImageTransfer(
			m_fontAtlases[identifier].fontAtlasImage,
			{ .bufferOffset = 0,
			  .bufferRowLength = 0,
			  .bufferImageHeight = 0,
			  .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									.mipLevel = 0,
									.baseArrayLayer = 0,
									.layerCount = 1 },
			  .imageExtent = atlasImageCreateInfo.extent },
			atlasData, atlasWidth * atlasHeight, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if constexpr (vanadiumGPUDebug) {
			setObjectName(
				m_renderContext.deviceContext->device(), VK_OBJECT_TYPE_IMAGE,
				m_renderContext.resourceAllocator->nativeImageHandle(m_fontAtlases[identifier].fontAtlasImage),
				"Font atlas image (ID hash " + std::to_string(robin_hood::hash<FontAtlasIdentifier>()(identifier)) +
					")");
		}
		delete[] atlasData;
	}

	void TextShapeRegistry::regenerateGlyphData(const FontAtlasIdentifier& identifier, uint32_t frameIndex) {
		if (m_fontAtlases[identifier].shapeGlyphData.empty())
			return;

		m_fontAtlases[identifier].glyphData.clear();
		for (auto& data : m_fontAtlases[identifier].shapeGlyphData) {
			Vector2 basePosition = data.referencedShape->position();
			Matrix2 rotationMatrix =
				Matrix2(cosf(data.referencedShape->rotation()), -sinf(data.referencedShape->rotation()),
						sinf(data.referencedShape->rotation()), cosf(data.referencedShape->rotation()));
			Vector2 preRotationOffset =
				Vector2(data.offset.x, data.offset.y + (m_fontAtlases[identifier].maxGlyphHeight - data.bearingY));
			Vector2 rotatedOffset = rotationMatrix * preRotationOffset;

			m_fontAtlases[identifier].glyphData.push_back(
				{ .position = Vector2(basePosition.x + rotatedOffset.x, basePosition.y + rotatedOffset.y),
				  .size = data.size,
				  .color = data.referencedShape->color(),
				  .uvPosition = m_fontAtlases[identifier].fontAtlasPositions[data.glyphIndex].position /
								m_fontAtlases[identifier].atlasSize,
				  .uvSize = m_fontAtlases[identifier].fontAtlasPositions[data.glyphIndex].size /
							m_fontAtlases[identifier].atlasSize,
				  .cosSinRotation = { cosf(data.referencedShape->rotation()),
									  sinf(data.referencedShape->rotation()) } });
		}

		if (m_fontAtlases[identifier].transferBufferCapacity <=
			m_fontAtlases[identifier].glyphData.size() * sizeof(RenderedGlyphData)) {
			if (m_fontAtlases[identifier].glyphDataTransfer != ~0U) {
				m_renderContext.transferManager->destroyTransfer(m_fontAtlases[identifier].glyphDataTransfer);
			}

			m_fontAtlases[identifier].transferBufferCapacity = std::max(
				static_cast<VkDeviceSize>(m_fontAtlases[identifier].transferBufferCapacity * 1.61),
				static_cast<VkDeviceSize>(m_fontAtlases[identifier].glyphData.size() * sizeof(RenderedGlyphData)));

			m_fontAtlases[identifier].glyphData.reserve(m_fontAtlases[identifier].transferBufferCapacity);

			m_fontAtlases[identifier].glyphDataTransfer = m_renderContext.transferManager->createTransfer(
				m_fontAtlases[identifier].transferBufferCapacity,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
			m_fontAtlases[identifier].lastRecreateFrameIndex = frameIndex;
		}
		m_fontAtlases[identifier].lastUpdateFrameIndex = frameIndex;
	}

	void TextShapeRegistry::updateAtlasDescriptors(const FontAtlasIdentifier& identifier, uint32_t frameIndex) {
		if (m_fontAtlases[identifier].glyphData.empty())
			return;

		VkDescriptorBufferInfo bufferInfo = {
			.buffer = m_renderContext.resourceAllocator->nativeBufferHandle(
				m_renderContext.transferManager->dstBufferHandle(m_fontAtlases[identifier].glyphDataTransfer)),
			.offset = 0,
			.range = m_fontAtlases[identifier].glyphData.size() * sizeof(RenderedGlyphData)
		};
		VkDescriptorImageInfo imageInfo = { .imageView = m_renderContext.resourceAllocator->requestImageView(
												m_fontAtlases[identifier].fontAtlasImage, m_atlasViewInfo),
											.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkWriteDescriptorSet writes[2] = { { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
											 .dstSet = m_fontAtlases[identifier].setAllocations[frameIndex].set,
											 .dstBinding = 0,
											 .dstArrayElement = 0,
											 .descriptorCount = 1,
											 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
											 .pBufferInfo = &bufferInfo },
										   { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
											 .dstSet = m_fontAtlases[identifier].setAllocations[frameIndex].set,
											 .dstBinding = 1,
											 .dstArrayElement = 0,
											 .descriptorCount = 1,
											 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
											 .pImageInfo = &imageInfo } };
		vkUpdateDescriptorSets(m_renderContext.deviceContext->device(), 2, writes, 0, nullptr);
	}

	void TextShapeRegistry::destroyAtlas(const FontAtlasIdentifier& identifier) {
		if (m_fontAtlases[identifier].glyphDataTransfer != ~0U)
			m_renderContext.transferManager->destroyTransfer(m_fontAtlases[identifier].glyphDataTransfer);
		if (m_fontAtlases[identifier].fontAtlasImage != ~0U)
			m_renderContext.resourceAllocator->destroyImage(m_fontAtlases[identifier].fontAtlasImage);
		for (auto& set : m_fontAtlases[identifier].setAllocations)
			m_renderContext.descriptorSetAllocator->freeDescriptorSet(set, m_textSetAllocationInfo);
		m_fontAtlases.erase(identifier);
	}

	void TextShapeRegistry::determineLineBreaksAndDimensions(TextShape* shape) {
		FT_Face face = m_uiSubsystem->fontLibrary().fontFace(shape->fontID());

		if (shape->fontID() >= m_fonts.size()) {
			m_fonts.resize(shape->fontID() + 1);
		}

		if (m_fonts[shape->fontID()].fontFace == nullptr) {
			m_fonts[shape->fontID()].fontFace =
				hb_ft_face_create_referenced(m_uiSubsystem->fontLibrary().fontFace(shape->fontID()));
		}

		FT_Set_Char_Size(face, shape->pointSize() * 64.0f, 0, m_uiSubsystem->monitorDPIX(),
						 m_uiSubsystem->monitorDPIY());

		float pointSizeKey = shape->pointSize() - fmodf(shape->pointSize(), FontAtlasIdentifier::eps);
		if (m_fonts[shape->fontID()].fonts.find(pointSizeKey) == m_fonts[shape->fontID()].fonts.end()) {
			m_fonts[shape->fontID()].fonts.insert(
				{ pointSizeKey, hb_ft_font_create_referenced(m_uiSubsystem->fontLibrary().fontFace(shape->fontID())) });
			hb_ft_font_set_funcs(m_fonts[shape->fontID()].fonts[pointSizeKey]);
		}

		std::string_view textView = shape->text();
		// The text, but instead of letters each codepoint is replaced with its break class
		std::vector<BreakClassRuleTraits> ruleTraitsString;
		// Linebreak status for each character of the text
		std::vector<LinebreakStatus> linebreakStatusString =
			std::vector<LinebreakStatus>(textView.size(), LinebreakStatus::Undefined);
		ruleTraitsString.reserve(textView.size());
		for (unsigned int i = 0; i < textView.size(); ++i) {
			ruleTraitsString.push_back(
				{ .breakClass = codepointBreakClass(utf8Codepoint(textView.data() + i, textView.size() - i)),
				  .eastAsianWidth = codepointEastAsianWidth(utf8Codepoint(textView.data() + i, textView.size() - i)),
				  .isExtendedPictographic =
					  isCodepointExtendedPictographic(utf8Codepoint(textView.data() + i, textView.size() - i)) });
		}

		hb_shape(m_fonts[shape->fontID()].fonts[pointSizeKey], shape->internalTextBuffer(), nullptr, 0);

		unsigned int glyphCount = 0;
		hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(shape->internalTextBuffer(), &glyphCount);
		hb_glyph_position_t* glyphPositions = hb_buffer_get_glyph_positions(shape->internalTextBuffer(), &glyphCount);

		for (auto& rule : defaultLinebreakRules) {
			executeRule(ruleTraitsString, linebreakStatusString, rule);
		}

		for (uint32_t i = 0; i < linebreakStatusString.size(); ++i) {
			if (linebreakStatusString[i] == LinebreakStatus::Mandatory) {
				uint32_t glyphIndex = i;
				if (i >= glyphCount || glyphInfos[i].cluster != i) {
					glyphIndex = std::find_if(glyphInfos, glyphInfos + glyphCount, [i](const auto& glyph) {
									 return glyph.cluster == i;
								 })->cluster;
				}
				shape->addLinebreak(glyphIndex);
			}
		}

		hb_position_t maxPenX = 0;

		hb_position_t penX = 0;
		hb_position_t penY = 0;
		hb_codepoint_t previousGlyphIndex = 0;

		uint32_t maxLineHeight = 0;
		uint32_t maxBearingY = 0;
		bool onFirstLine = true;

		std::vector<GlyphInfo> shapeGlyphInfos;
		shapeGlyphInfos.reserve(glyphCount);

		for (unsigned int i = 0; i < glyphCount; ++i) {
			FT_Load_Glyph(face, glyphInfos[i].codepoint, FT_LOAD_DEFAULT);

			hb_position_t xAdvance = glyphPositions[i].x_advance / 64;
			hb_position_t yAdvance = glyphPositions[i].y_advance / 64;

			penX += xAdvance;
			penY += yAdvance;

			if (std::find(shape->linebreakGlyphIndices().begin(), shape->linebreakGlyphIndices().end(), i) !=
				shape->linebreakGlyphIndices().end()) {
				penY += face->size->metrics.height;
				maxPenX = std::max(penX, maxPenX);
				penX = 0;
				maxLineHeight = 0;
				onFirstLine = false;
			} else {
				FT_Vector kerningDelta;
				FT_Get_Kerning(face, previousGlyphIndex, glyphInfos[i].codepoint, FT_KERNING_DEFAULT, &kerningDelta);
				penX += kerningDelta.x / 64;
			}

			// skip over line control characters
			BreakClass breakClass = codepointBreakClass(
				utf8Codepoint(textView.data() + glyphInfos[i].cluster, textView.size() - glyphInfos[i].cluster));
			if (breakClass == BreakClass::CR || breakClass == BreakClass::LF || breakClass == BreakClass::BK) {
				continue;
			}

			if (penX > shape->maxWidth() && shape->maxWidth() > 0.0f) {
				uint32_t lastLinebreakIndex = 0;
				for (auto& index : shape->linebreakGlyphIndices()) {
					if (index > i)
						break;
					else
						lastLinebreakIndex = index;
				}
				bool foundLinebreak = false;
				for (unsigned int j = glyphInfos[i].cluster; j > glyphInfos[lastLinebreakIndex].cluster; --j) {
					if (linebreakStatusString[j] == LinebreakStatus::Opportunity) {
						unsigned int brokenGlyphIndex = 0;
						for (; brokenGlyphIndex < glyphCount; ++brokenGlyphIndex) {
							if (glyphInfos[brokenGlyphIndex].cluster > j)
								break;
						}
						if (brokenGlyphIndex != 0 && brokenGlyphIndex != glyphCount) {
							shape->addLinebreak(brokenGlyphIndex - 1);
							i = lastLinebreakIndex;
							penX = 0;
							maxLineHeight = 0;
							foundLinebreak = true;
							previousGlyphIndex = 0;
							break;
						}
					}
				}

				if (!foundLinebreak && i > 0) {
					// emergency linebreak, ignore rules
					shape->addLinebreak(i - 1);
					previousGlyphIndex = 0;
					// i gets incremented at end of loop
					i -= 2;
				}
				previousGlyphIndex = glyphInfos[i].codepoint;
			}

			previousGlyphIndex = glyphInfos[i].codepoint;
			maxLineHeight = std::max(maxLineHeight, static_cast<uint32_t>(face->glyph->metrics.height / 64));

			if (onFirstLine)
				maxBearingY = std::max(maxBearingY, static_cast<uint32_t>(face->glyph->metrics.horiBearingY / 64));
			// undo addition by xAdvance
			shapeGlyphInfos.push_back({ .position = Vector2(penX - xAdvance, penY),
										.advance = static_cast<float>(xAdvance),
										.clusterIndex = glyphInfos[i].cluster });
		}
		maxPenX = std::max(penX, maxPenX);
		shape->addLinebreak(glyphCount - 1); // LB3
		shape->setGlyphInfos(std::move(shapeGlyphInfos));
		shape->setInternalSize(Vector2(maxPenX, penY + maxLineHeight));
		shape->setBaselineOffset(maxBearingY);
	}

	TextShape::TextShape(const Vector2& position, uint32_t layerIndex, float maxWidth, float rotation,
						 const std::string_view& text, float fontSize, uint32_t fontID, const Vector4& color)
		: Shape("Text", layerIndex, position, rotation), m_fontID(fontID), m_pointSize(fontSize), m_maxWidth(maxWidth),
		  m_color(color), m_text(text) {
		m_textBuffer = hb_buffer_create();
		hb_buffer_add_utf8(m_textBuffer, text.data(), text.size(), 0, -1);
		hb_buffer_set_direction(m_textBuffer, HB_DIRECTION_LTR);
		hb_buffer_set_script(m_textBuffer, HB_SCRIPT_LATIN);
		hb_buffer_set_language(m_textBuffer, hb_language_from_string("en", -1));
	}

	TextShape::~TextShape() { hb_buffer_destroy(m_textBuffer); }

	void TextShape::setText(const std::string_view& text) {
		m_textDirtyFlag = true;
		m_text = text;
		hb_buffer_destroy(m_textBuffer);
		m_textBuffer = hb_buffer_create();
		hb_buffer_add_utf8(m_textBuffer, text.data(), text.size(), 0, -1);
		hb_buffer_set_direction(m_textBuffer, HB_DIRECTION_LTR);
		hb_buffer_set_script(m_textBuffer, HB_SCRIPT_LATIN);
		hb_buffer_set_language(m_textBuffer, hb_language_from_string("en", -1));
		m_linebreakGlyphIndices.clear();
		m_registry->determineLineBreaksAndDimensions(this);
	}

	void TextShape::setMaxWidth(float maxWidth) {
		m_textDirtyFlag = true;
		m_maxWidth = maxWidth;
	}

	void TextShape::setPointSize(float pointSize) {
		m_textDirtyFlag = true;
		m_pointSize = pointSize;
	}

	void TextShape::deleteClusters(uint32_t glyphIndex) {
		uint32_t charCount = utf8CharSize(m_text[m_glyphInfos[glyphIndex].clusterIndex]);
		m_text.erase(m_text.begin() + m_glyphInfos[glyphIndex].clusterIndex,
					 m_text.begin() + m_glyphInfos[glyphIndex].clusterIndex + charCount);

		m_textDirtyFlag = true;
		hb_buffer_destroy(m_textBuffer);
		m_textBuffer = hb_buffer_create();
		hb_buffer_add_utf8(m_textBuffer, m_text.data(), m_text.size(), 0, -1);
		hb_buffer_set_direction(m_textBuffer, HB_DIRECTION_LTR);
		hb_buffer_set_script(m_textBuffer, HB_SCRIPT_LATIN);
		hb_buffer_set_language(m_textBuffer, hb_language_from_string("en", -1));
		m_linebreakGlyphIndices.clear();
		m_registry->determineLineBreaksAndDimensions(this);
	}

	void TextShape::setGlyphInfos(std::vector<GlyphInfo>&& glyphInfos) {
		m_glyphInfos = std::forward<std::vector<GlyphInfo>>(glyphInfos);
	}
} // namespace vanadium::ui::shapes
