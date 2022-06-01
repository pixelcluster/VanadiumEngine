#include <ui/UISubsystem.hpp>
#include <ui/layouts/FreeLayout.hpp>
#include <ui/styles/NullStyle.hpp>

namespace vanadium::ui {

	UISubsystem::UISubsystem(windowing::WindowInterface* windowInterface, const graphics::RenderContext& context,
							 const std::string_view& fontLibraryFile, bool clearBackground, const Vector4& clearValue)
		: m_windowInterface(windowInterface), m_fontLibrary(fontLibraryFile),
		  m_rootControl(createParameters<styles::NullStyle, layouts::FreeLayout>(
									 this, nullptr, ControlPosition(PositionOffsetType::TopLeft, Vector2(0.0f, 0.0f)),
									 Vector2(static_cast<float>(context.targetSurface->properties().width),
											 static_cast<float>(context.targetSurface->properties().height)),
									 {})) {
		if (clearBackground)
			m_rendererNode = new UIRendererNode(this, context, clearBackground);
		else
			m_rendererNode = new UIRendererNode(this, context);
	}

	void UISubsystem::addRendererNode(graphics::FramegraphContext& context) {
		m_rendererNode->create(&context);
		context.appendExistingNode(m_rendererNode);
	}

	void UISubsystem::acquireInputFocus(Control* newInputFocusControl, windowing::KeyModifierFlags modifierMask,
										windowing::KeyStateFlags stateMask) {
		m_inputFocusControl = newInputFocusControl;
		m_inputFocusModifierMask = modifierMask;
		m_inputFocusStateMask = stateMask;
	}

	void UISubsystem::releaseInputFocus() { m_inputFocusControl = nullptr; }

	void UISubsystem::invokeMouseHover(const Vector2& mousePos) { m_rootControl.hoverHandler(this, mousePos); }

	void UISubsystem::invokeMouseButton(const Vector2& mousePos, uint32_t buttonID) {
		m_rootControl.mouseButtonHandler(this, mousePos, buttonID);
	}

	void UISubsystem::invokeKey(uint32_t keyID, windowing::KeyModifierFlags modifierFlags,
								windowing::KeyStateFlags stateFlags) {
		if (m_inputFocusControl)
			m_inputFocusControl->keyInputHandler(this, keyID, modifierFlags, stateFlags);
	}

	void UISubsystem::invokeCharacter(uint32_t codepoint) {
		if (m_inputFocusControl)
			m_inputFocusControl->charInputHandler(this, codepoint);
	}

} // namespace vanadium::ui
