#include <ui/UISubsystem.hpp>

namespace vanadium::ui {
	UISubsystem::UISubsystem(const graphics::RenderContext& context, uint32_t monitorDPIX, uint32_t monitorDPIY, const std::string_view& fontLibraryFile,
							 bool clearBackground, const Vector4& clearValue)
		: m_fontLibrary(fontLibraryFile), m_monitorDPIX(monitorDPIX), m_monitorDPIY(monitorDPIY) {
		if (clearBackground)
			m_rendererNode = new UIRendererNode(this, context, clearBackground);
		else
			m_rendererNode = new UIRendererNode(this, context);
	}

	void UISubsystem::addRendererNode(graphics::FramegraphContext& context) {
		m_rendererNode->create(&context);
		context.appendExistingNode(m_rendererNode);
	}
} // namespace vanadium::ui
