#include <CharacterGroup.hpp>
#include <Log.hpp>
#include <graphics/helper/DebugHelper.hpp>
#include <ui/shapes/Text.hpp>
#include <ui/util/BreakClassRule.hpp>
#include <volk.h>

namespace vanadium::ui::shapes {

	TextShapeRegistry::TextShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context,
										 VkRenderPass uiRenderPass,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		m_textPipelineID = context.pipelineLibrary->findGraphicsPipeline("UI Text");

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
			for (auto& atlas : m_fontAtlases) {
				auto iterator =
					std::find(atlas.second.referencingShapes.begin(), atlas.second.referencingShapes.end(), textShape);
				if (iterator != atlas.second.referencingShapes.end()) {
					atlas.second.referencingShapes.erase(iterator);
					atlas.second.dirtyFlag = true;
					if (atlas.second.referencingShapes.empty())
						destroyAtlas(atlas.first);
					break;
				}
			}
		}
	}

	void TextShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		for (auto& shape : m_shapes) {
			FontAtlasIdentifier identifier = { .fontID = shape->fontID(), .pointSize = shape->pointSize() };
			if (shape->textDirtyFlag()) {
				for (auto& atlas : m_fontAtlases) {
					auto iterator =
						std::find(atlas.second.referencingShapes.begin(), atlas.second.referencingShapes.end(), shape);
					if (iterator != atlas.second.referencingShapes.end()) {
						atlas.second.referencingShapes.erase(iterator);
						atlas.second.dirtyFlag = true;
						break;
					}
				}
				m_fontAtlases[identifier].dirtyFlag = true;
				m_fontAtlases[identifier].referencingShapes.push_back(shape);
			} else if (shape->dirtyFlag()) {
				m_fontAtlases[identifier].bufferDirtyFlag = true;
			}
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						  m_renderContext.pipelineLibrary->graphicsPipeline(m_textPipelineID, uiRenderPassSignature));
		VkViewport viewport = { .width = static_cast<float>(m_renderContext.targetSurface->properties().width),
								.height = static_cast<float>(m_renderContext.targetSurface->properties().height),
								.minDepth = 0.0f,
								.maxDepth = 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		Vector2 windowSize = Vector2(m_renderContext.targetSurface->properties().width,
									 m_renderContext.targetSurface->properties().height);
		VkShaderStageFlags stageFlags =
			m_renderContext.pipelineLibrary->graphicsPipelinePushConstantRanges(m_textPipelineID)[0].stageFlags;
		vkCmdPushConstants(commandBuffer, m_renderContext.pipelineLibrary->graphicsPipelineLayout(m_textPipelineID),
						   stageFlags, 0, sizeof(Vector2), &windowSize);

		for (auto& atlas : m_fontAtlases) {
			if (atlas.second.dirtyFlag) {
				regenerateFontAtlas(atlas.first, frameIndex);
				regenerateGlyphData(atlas.first, frameIndex);
				m_renderContext.transferManager->updateTransferData(atlas.second.glyphDataTransfer, frameIndex,
																	atlas.second.glyphData.data());
				if (atlas.second.lastRecreateFrameIndex != ~0U)
					updateAtlasDescriptors(atlas.first, frameIndex);
			} else if (atlas.second.bufferDirtyFlag) {
				regenerateGlyphData(atlas.first, frameIndex);
				m_renderContext.transferManager->updateTransferData(atlas.second.glyphDataTransfer, frameIndex,
																	atlas.second.glyphData.data());
				if (atlas.second.lastRecreateFrameIndex != ~0U)
					updateAtlasDescriptors(atlas.first, frameIndex);
			} else {
				if (atlas.second.lastUpdateFrameIndex != ~0U) {
					if (atlas.second.lastUpdateFrameIndex == frameIndex) {
						atlas.second.lastUpdateFrameIndex = ~0U;
					} else {
						m_renderContext.transferManager->updateTransferData(atlas.second.glyphDataTransfer, frameIndex,
																			atlas.second.glyphData.data());
					}
				}

				if (atlas.second.lastRecreateFrameIndex != ~0U) {
					if (atlas.second.lastRecreateFrameIndex == frameIndex) {
						atlas.second.lastRecreateFrameIndex = ~0U;
					} else {
						updateAtlasDescriptors(atlas.first, frameIndex);
					}
				}
			}

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
									m_renderContext.pipelineLibrary->graphicsPipelineLayout(m_textPipelineID), 0, 1,
									&atlas.second.setAllocations[frameIndex].set, 0, nullptr);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(6U * atlas.second.glyphData.size()), 1, 0, 0);

			for (auto& shape : atlas.second.referencingShapes) {
				shape->clearDirtyFlag();
				shape->clearTextDirtyFlag();
			}
			atlas.second.dirtyFlag = false;
			atlas.second.bufferDirtyFlag = false;
		}
	}

	void TextShapeRegistry::destroy(const graphics::RenderPassSignature& uiRenderPassSignature) {
		for (auto& shape : m_shapes) {
			delete shape;
		}
		for (auto& atlas : m_fontAtlases) {
			destroyAtlas(atlas.first);
		}
	}

	void TextShapeRegistry::regenerateFontAtlas(const FontAtlasIdentifier& identifier, uint32_t frameIndex) {
		FT_Face face = m_uiSubsystem->fontLibrary().fontFace(identifier.fontID);
		robin_hood::unordered_set<uint32_t> usedGlyphs;

		hb_position_t oneLinePenX = 0;
		uint32_t maxGlyphHeight = 0;
		size_t totalGlyphCount = 0;

		for (auto& shape : m_fontAtlases[identifier].referencingShapes) {
			FT_Set_Char_Size(face, shape->pointSize() * 64.0f, 0, m_uiSubsystem->monitorDPIX(),
							 m_uiSubsystem->monitorDPIY());

			unsigned int glyphCount = 0;
			hb_glyph_info_t* glyphInfos = hb_buffer_get_glyph_infos(shape->internalTextBuffer(), &glyphCount);
			hb_glyph_position_t* shapeGlyphPositions =
				hb_buffer_get_glyph_positions(shape->internalTextBuffer(), &glyphCount);

			hb_position_t penX = 0;
			hb_position_t penY = 0;

			std::string_view textView = shape->text();

			for (unsigned int i = 0; i < glyphCount; ++i) {
				hb_codepoint_t glyphID = glyphInfos[i].codepoint;
				hb_position_t xOffset = shapeGlyphPositions[i].x_offset / 64;
				hb_position_t yOffset = shapeGlyphPositions[i].y_offset / 64;
				hb_position_t xAdvance = shapeGlyphPositions[i].x_advance / 64;
				hb_position_t yAdvance = shapeGlyphPositions[i].y_advance / 64;

				if (i > 0 && std::find(shape->linebreakGlyphIndices().begin(), shape->linebreakGlyphIndices().end(),
									   i - 1) != shape->linebreakGlyphIndices().end()) {
					penY += face->size->metrics.height / 64;
					penX = 0;
				}

				// skip over line control characters
				BreakClass breakClass = codepointBreakClass(
					utf8Codepoint(textView.data() + glyphInfos[i].cluster, textView.size() - glyphInfos[i].cluster));
				if (breakClass == BreakClass::CR || breakClass == BreakClass::LF || breakClass == BreakClass::BK)
					continue;

				FT_Load_Glyph(face, glyphID, FT_LOAD_DEFAULT);

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
				maxGlyphHeight =
					std::max(maxGlyphHeight, static_cast<uint32_t>(face->glyph->metrics.horiBearingY / 64));

				++totalGlyphCount;
			}
		}

		uint32_t approximateTextureDimensions = std::sqrt(oneLinePenX * maxGlyphHeight);

		uint32_t pixelPenX = 0;
		uint32_t pixelPenY = 0;

		uint32_t maxLineHeight = 0;
		m_fontAtlases[identifier].maxGlyphHeight = 0;

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
																		 .size = Vector2(pixelWidth, pixelHeight) };

			pixelPenX += pixelWidth;
			maxLineHeight = std::max(pixelHeight, maxLineHeight);
			m_fontAtlases[identifier].maxGlyphHeight = std::max(pixelHeight, m_fontAtlases[identifier].maxGlyphHeight);
		}

		uint32_t atlasWidth = approximateTextureDimensions;
		uint32_t atlasHeight = pixelPenY + maxLineHeight;

		char* atlasData = new char[atlasWidth * atlasHeight];
		std::memset(atlasData, 0, atlasWidth * atlasHeight);
		for (auto& glyphCoordinatePair : m_fontAtlases[identifier].fontAtlasPositions) {
			FT_Load_Glyph(face, glyphCoordinatePair.first, FT_LOAD_RENDER);
			char* glyphOffset = atlasData + static_cast<uint32_t>(glyphCoordinatePair.second.position.y) * atlasWidth +
								static_cast<uint32_t>(glyphCoordinatePair.second.position.x);
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
		auto& atlas = m_fontAtlases[identifier];
		m_fontAtlases[identifier].glyphData.clear();
		for (auto& data : m_fontAtlases[identifier].shapeGlyphData) {
			Vector3 basePosition = data.referencedShape->position();
			m_fontAtlases[identifier].glyphData.push_back(
				{ .position = Vector3(basePosition.x + data.offset.x,
									  basePosition.y + data.offset.y +
										  (m_fontAtlases[identifier].maxGlyphHeight - data.bearingY),
									  basePosition.z),
				  .color = data.referencedShape->color(),
				  .size = data.size,
				  .uvPosition = m_fontAtlases[identifier].fontAtlasPositions[data.glyphIndex].position /
								m_fontAtlases[identifier].atlasSize,
				  .uvSize = m_fontAtlases[identifier].fontAtlasPositions[data.glyphIndex].size /
							m_fontAtlases[identifier].atlasSize });
		}

		if (m_fontAtlases[identifier].transferBufferCapacity <=
			m_fontAtlases[identifier].glyphData.size() * sizeof(RenderedGlyphData)) {
			if (m_fontAtlases[identifier].glyphDataTransfer != ~0U) {
				m_renderContext.transferManager->destroyTransfer(m_fontAtlases[identifier].glyphDataTransfer);
			}

			m_fontAtlases[identifier].transferBufferCapacity = std::max(
				static_cast<VkDeviceSize>(m_fontAtlases[identifier].transferBufferCapacity * 1.61),
				static_cast<VkDeviceSize>(m_fontAtlases[identifier].glyphData.size() * sizeof(RenderedGlyphData)));

			m_fontAtlases[identifier].glyphDataTransfer = m_renderContext.transferManager->createTransfer(
				m_fontAtlases[identifier].transferBufferCapacity,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
			m_fontAtlases[identifier].lastRecreateFrameIndex = frameIndex;
		}
		m_fontAtlases[identifier].lastUpdateFrameIndex = frameIndex;
	}

	void TextShapeRegistry::updateAtlasDescriptors(const FontAtlasIdentifier& identifier, uint32_t frameIndex) {
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
		m_renderContext.transferManager->destroyTransfer(m_fontAtlases[identifier].glyphDataTransfer);
		m_renderContext.resourceAllocator->destroyImage(m_fontAtlases[identifier].fontAtlasImage);
		for (auto& set : m_fontAtlases[identifier].setAllocations)
			m_renderContext.descriptorSetAllocator->freeDescriptorSet(set, m_textSetAllocationInfo);
		m_fontAtlases.erase(identifier);
		for (auto& font : m_fonts) {
			hb_font_destroy(font.font);
			hb_face_destroy(font.fontFace);
		}
	}

	void TextShapeRegistry::determineLineBreaksAndDimensions(TextShape* shape) {
		FT_Face face = m_uiSubsystem->fontLibrary().fontFace(shape->fontID());
		FT_Set_Char_Size(face, shape->pointSize() * 64.0f, 0, m_uiSubsystem->monitorDPIX(),
						 m_uiSubsystem->monitorDPIY());

		if (shape->fontID() >= m_fonts.size()) {
			m_fonts.resize(shape->fontID() + 1);
		}

		if (m_fonts[shape->fontID()].font == nullptr) {
			m_fonts[shape->fontID()].fontFace =
				hb_ft_face_create_referenced(m_uiSubsystem->fontLibrary().fontFace(shape->fontID()));
			m_fonts[shape->fontID()].font =
				hb_ft_font_create_referenced(m_uiSubsystem->fontLibrary().fontFace(shape->fontID()));
			hb_ft_font_set_funcs(m_fonts[shape->fontID()].font);
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

		hb_shape(m_fonts[shape->fontID()].font, shape->internalTextBuffer(), nullptr, 0);

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

		for (unsigned int i = 0; i < glyphCount; ++i) {
			hb_position_t xAdvance = glyphPositions[i].x_advance / 64;
			hb_position_t yAdvance = glyphPositions[i].y_advance / 64;

			penX += xAdvance;
			penY += yAdvance;

			if (std::find(shape->linebreakGlyphIndices().begin(), shape->linebreakGlyphIndices().end(), i) !=
				shape->linebreakGlyphIndices().end()) {
				penY += face->size->metrics.height;
				maxPenX = std::max(penX, maxPenX);
				penX = 0;
			}

			// skip over line control characters
			BreakClass breakClass = codepointBreakClass(
				utf8Codepoint(textView.data() + glyphInfos[i].cluster, textView.size() - glyphInfos[i].cluster));
			if (breakClass == BreakClass::CR || breakClass == BreakClass::LF || breakClass == BreakClass::BK) {
				continue;
			}

			if (penX > shape->maxWidth()) {
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
							foundLinebreak = true;
							break;
						}
					}
				}

				if (!foundLinebreak && i > 0) {
					// emergency linebreak, ignore rules
					shape->addLinebreak(i - 1);
					// i gets incremented at end of loop
					i -= 2;
				}
			}
		}
		shape->addLinebreak(glyphCount - 1); // LB3

		shape->setInternalSize(Vector2(maxPenX, shape->linebreakGlyphIndices().size() * face->size->metrics.height));
	}

	TextShape::TextShape(const Vector3& position, float maxWidth, float rotation, const std::string_view& text,
						 float fontSize, uint32_t fontID, const Vector4& color)
		: Shape("Text", position, rotation), m_maxWidth(maxWidth), m_pointSize(fontSize), m_fontID(fontID),
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
		m_registry->determineLineBreaksAndDimensions(this);
	}
} // namespace vanadium::ui::shapes
