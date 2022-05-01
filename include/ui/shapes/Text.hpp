#pragma once

#include <graphics/RenderContext.hpp>
#include <math/Vector.hpp>
#include <robin_hood.h>
#include <string>
#include <ui/ShapeRegistry.hpp>
#include <ui/UISubsystem.hpp>
#include <vector>

#include <ft2build.h>
#include <hb-ft.h>
#include <hb.h>
#include <helper/UTF8.hpp>

namespace vanadium::ui::shapes {
	class TextShape;

	struct FontData {
		hb_face_t* fontFace = nullptr;
		hb_font_t* font = nullptr;
	};

	struct ShapeGlyphData {
		TextShape* referencedShape;
		uint32_t glyphIndex;
		Vector2 offset;
		Vector2 size;
	};

	struct GlyphAtlasCoords {
		Vector2 position;
		Vector2 size;
	};

	struct RenderedGlyphData {
		Vector3 position;
		float _pad1;
		Vector4 color;
		Vector2 size;
		Vector2 uvPosition;
		Vector2 uvSize;
		float _pad2[2];
	};

	struct FontAtlasIdentifier {
		static constexpr float eps = 0.001f;
		uint32_t fontID;
		float pointSize;

		bool operator==(const FontAtlasIdentifier& other) const {
			return fontID == other.fontID && fabsf(pointSize - other.pointSize) < eps;
		}
	};

	struct FontAtlas {
		bool dirtyFlag = false;
		bool bufferDirtyFlag = false;

		uint32_t lastRecreateFrameIndex = ~0U;
		uint32_t lastUpdateFrameIndex = ~0U;

		VkDeviceSize transferBufferCapacity = 0U;
		graphics::GPUTransferHandle glyphDataTransfer = ~0U;
		graphics::ImageResourceHandle fontAtlasImage = ~0U;

		robin_hood::unordered_map<uint32_t, GlyphAtlasCoords> fontAtlasPositions;
		std::vector<ShapeGlyphData> shapeGlyphData;
		std::vector<RenderedGlyphData> glyphData;
		Vector2 atlasSize;
		uint32_t maxGlyphHeight;

		graphics::DescriptorSetAllocation setAllocations[graphics::frameInFlightCount] = {};
		std::vector<TextShape*> referencingShapes;
	};
} // namespace vanadium::ui::shapes

namespace robin_hood {
	template <> struct hash<vanadium::ui::shapes::FontAtlasIdentifier> {
		size_t operator()(const vanadium::ui::shapes::FontAtlasIdentifier& object) const {
			float fmodPointSize = fmodf(object.pointSize, vanadium::ui::shapes::FontAtlasIdentifier::eps);
			return hashCombine(robin_hood::hash<uint32_t>()(object.fontID),
							   robin_hood::hash<float>()(object.pointSize - fmodPointSize));
		}
	};
} // namespace robin_hood

namespace vanadium::ui::shapes {

	class TextShapeRegistry : public ShapeRegistry {
	  public:
		TextShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context, VkRenderPass uiRenderPass,
						  const graphics::RenderPassSignature& uiRenderPassSignature);
		~TextShapeRegistry() {}

		void addShape(Shape* shape) override;
		void removeShape(Shape* shape) override;
		void renderShapes(VkCommandBuffer commandBuffers, uint32_t frameIndex,
						  const graphics::RenderPassSignature& uiRenderPassSignature) override;
		void destroy(const graphics::RenderPassSignature& uiRenderPassSignature) override;

		void determineLineBreaksAndDimensions(TextShape* shape);

	  private:
		void regenerateFontAtlas(const FontAtlasIdentifier& identifier, uint32_t frameIndex);
		void regenerateGlyphData(const FontAtlasIdentifier& identifier, uint32_t frameIndex);
		void updateAtlasDescriptors(const FontAtlasIdentifier& identifier, uint32_t frameIndex);
		void destroyAtlas(const FontAtlasIdentifier& identifier);

		uint32_t m_textPipelineID;
		graphics::DescriptorSetAllocationInfo m_textSetAllocationInfo;

		graphics::ImageResourceViewInfo m_atlasViewInfo;

		robin_hood::unordered_map<FontAtlasIdentifier, FontAtlas> m_fontAtlases;
		std::vector<FontData> m_fonts;
		std::vector<TextShape*> m_shapes;

		graphics::RenderContext m_renderContext;
		UISubsystem* m_uiSubsystem;
	};

	class TextShape : public Shape {
	  public:
		using ShapeRegistry = TextShapeRegistry;

		TextShape(const Vector3& position, float maxWidth, float rotation, const std::string_view& text, float fontSize,
				  uint32_t fontID, const Vector4& color);
		~TextShape();

		void setInternalRegistry(ShapeRegistry* currentRegistry) { m_registry = currentRegistry; }
		void setText(const std::string_view& text);

		// internal, do not call yourself
		void setInternalSize(const Vector2& size) { m_size = size; }

		const Vector2& size() const { return m_size; }
		const Vector4& color() const { return m_color; }
		uint32_t fontID() const { return m_fontID; }
		float pointSize() const { return m_pointSize; }
		float maxWidth() const { return m_maxWidth; }

		void addLinebreak(uint32_t index) { m_linebreakGlyphIndices.push_back(index); }
		const std::vector<uint32_t>& linebreakGlyphIndices() const { return m_linebreakGlyphIndices; }
		hb_buffer_t* internalTextBuffer() { return m_textBuffer; }

		bool textDirtyFlag() const { return m_textDirtyFlag; }
		void clearTextDirtyFlag() { m_textDirtyFlag = false; }

	  private:
		ShapeRegistry* m_registry;

		bool m_textDirtyFlag = false;
		uint32_t m_fontID;
		float m_pointSize;

		float m_maxWidth;

		Vector2 m_size;
		Vector4 m_color;

		hb_buffer_t* m_textBuffer;
		std::vector<uint32_t> m_linebreakGlyphIndices;
		std::string m_text;
	};

} // namespace vanadium::ui::shapes
