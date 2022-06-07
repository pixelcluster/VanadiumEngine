#include <ui/UISubsystem.hpp>
#include <ui/functionalities/NullFunctionality.hpp>
#include <ui/layouts/FreeLayout.hpp>
#include <ui/styles/NullStyle.hpp>

namespace vanadium::ui {

	void windowSizeListener(uint32_t width, uint32_t height, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->setWindowSize(width, height);
	}

	void mouseButtonListener(uint32_t keyCode, uint32_t modifiers, windowing::KeyState state, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeMouseButton(keyCode);
	}

	void mouseListener(const Vector2& pos, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeMouseHover(pos);
	}

	void keyListener(uint32_t keyCode, uint32_t modifiers, windowing::KeyState state, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeKey(keyCode, modifiers, state);
	}

	UISubsystem::UISubsystem(windowing::WindowInterface* windowInterface, const graphics::RenderContext& context,
							 const std::string_view& fontLibraryFile, bool clearBackground, const Vector4& clearValue)
		: m_windowInterface(windowInterface), m_fontLibrary(fontLibraryFile),
		  m_rootControl(createParameters<styles::NullStyle, layouts::FreeLayout, functionalities::NullFunctionality>(
			  this, nullptr, ControlPosition(PositionOffsetType::TopLeft, Vector2(0.0f, 0.0f)), Vector2(0.0f, 0.0f), {},
			  {})) {
		if (clearBackground)
			m_rendererNode = new UIRendererNode(this, context, clearBackground);
		else
			m_rendererNode = new UIRendererNode(this, context);

		windowInterface->addSizeListener({ .eventCallback = windowSizeListener,
										   .listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
										   .userData = this });
		windowInterface->addMouseKeyListener(GLFW_MOUSE_BUTTON_LEFT, static_cast<windowing::KeyModifierFlags>(~0U),
											 static_cast<windowing::KeyStateFlags>(windowing::KeyState::Pressed),
											 { .eventCallback = mouseButtonListener,
											   .listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
											   .userData = this });
		windowInterface->addMouseMoveListener({ .eventCallback = mouseListener,
												.listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
												.userData = this });
	}

	void UISubsystem::addRendererNode(graphics::FramegraphContext& context) {
		m_rendererNode->create(&context);
		context.appendExistingNode(m_rendererNode);
	}

	void UISubsystem::setWindowSize(uint32_t windowWidth, uint32_t windowHeight) {
		m_rootControl.setSize(Vector2(windowWidth, windowHeight));
	}

	void UISubsystem::acquireInputFocus(Control* newInputFocusControl, windowing::KeyModifierFlags modifierMask,
										windowing::KeyStateFlags stateMask) {
		m_inputFocusControl = newInputFocusControl;
		m_inputFocusModifierMask = modifierMask;
		m_inputFocusStateMask = stateMask;
	}

	void UISubsystem::releaseInputFocus() { m_inputFocusControl = nullptr; }

	void UISubsystem::invokeMouseHover(const Vector2& mousePos) { m_rootControl.invokeHoverHandler(this, mousePos); }

	void UISubsystem::invokeMouseButton(uint32_t buttonID) {
		Vector2 mousePos = m_windowInterface->mousePos();
		m_rootControl.invokeMouseButtonHandler(this, mousePos, buttonID);
	}

	void UISubsystem::invokeKey(uint32_t keyID, windowing::KeyModifierFlags modifierFlags,
								windowing::KeyState stateFlags) {
		if (m_inputFocusControl)
			m_inputFocusControl->invokeKeyInputHandler(this, keyID, modifierFlags, stateFlags);
	}

	void UISubsystem::invokeCharacter(uint32_t codepoint) {
		if (m_inputFocusControl)
			m_inputFocusControl->invokeCharInputHandler(this, codepoint);
	}

} // namespace vanadium::ui
