#pragma once

#include <graphics/RenderContext.hpp>
#include <ui/ShapeRegistry.hpp>
#include <math/Vector.hpp>
#include <string>
#include <robin_hood.h>
#include <vector>

#include <ft2build.h>
#include <hb.h>
#include <hb-ft.h>

namespace vanadium::ui
{
    class TextShape;

    struct FontData
    {
        hb_face_t* fontFace;
        hb_font_t* font;
    };

    struct GlyphPos
    {
        Vector2 position;
        Vector2 size;
    };
    

    class TextShapeRegistry
    {
    public:
        TextShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context, VkRenderPass uiRenderPass,
						  const graphics::RenderPassSignature& uiRenderPassSignature);
        ~TextShapeRegistry();
    private:
        FT_Library m_freetypeLibrary;
        
        std::vector<FontData> m_fonts;
        std::vector<TextShape*> textShapes;
    };

    class TextShape
    {
    public:
        TextShape(const std::string_view& text, const std::string_view& fontName);
        ~TextShape();
    private:
        hb_buffer_t* m_textBuffer;
        robin_hood::unordered_map<char, std::vector<GlyphPos>> m_glyphPositions;
        std::string m_text;
    };
    
} // namespace vanadium::ui
