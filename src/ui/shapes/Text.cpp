#include <Log.hpp>
#include <ui/shapes/Text.hpp>

namespace vanadium::ui {
	TextShapeRegistry::TextShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context,
										 VkRenderPass uiRenderPass,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		auto error = FT_Init_FreeType(&m_freetypeLibrary);
		if (error != FT_Err_Ok) {
			logError("Failed to initialize FreeType! Error: %s", FT_Error_String(error));
		}
	}
} // namespace vanadium::ui
