#include <ui/UISubsystem.hpp>

namespace vanadium::ui {
	UISubsystem::UISubsystem(const graphics::RenderContext& context, bool clearBackground, const Vector4& clearValue) {
		if (clearBackground)
			m_rendererNode = new UIRendererNode(context, clearBackground);
		else
			m_rendererNode = new UIRendererNode(context);
	}

	void UISubsystem::addRendererNode(graphics::FramegraphContext& context) {
		m_rendererNode->create(&context);
		context.appendExistingNode(m_rendererNode);
	}
} // namespace vanadium::ui
