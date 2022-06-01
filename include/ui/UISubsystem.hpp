#pragma once

#include <ui/Control.hpp>
#include <ui/FontLibrary.hpp>
#include <ui/UIRendererNode.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium::ui {
	class UISubsystem {
	  public:
		UISubsystem(windowing::WindowInterface* windowInterface, const graphics::RenderContext& context,
					const std::string_view& fontLibraryFile, bool clearBackground, const Vector4& clearValue);

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* addShape(Args&&... args) {
			return m_rendererNode->constructShape<T>(args...);
		}
		void removeShape(Shape* shape) { m_rendererNode->removeShape(shape); }

		void addRendererNode(graphics::FramegraphContext& context);

		FontLibrary& fontLibrary() { return m_fontLibrary; }

		uint32_t monitorDPIX() const { return m_windowInterface->contentScaleDPIX(); }
		uint32_t monitorDPIY() const { return m_windowInterface->contentScaleDPIY(); }

		void acquireInputFocus(Control* newInputFocusControl, windowing::KeyModifierFlags modifierMask,
							   windowing::KeyStateFlags stateMask);
		void releaseInputFocus();

		void invokeMouseHover(const Vector2& mousePos);
		void invokeMouseButton(const Vector2& mousePos, uint32_t buttonID);
		void invokeKey(uint32_t keyID, windowing::KeyModifierFlags modifierFlags, windowing::KeyStateFlags stateFlags);
		void invokeCharacter(uint32_t codepoint);

	  private:
		windowing::WindowInterface* m_windowInterface;

		UIRendererNode* m_rendererNode;
		FontLibrary m_fontLibrary;
		Control m_rootControl;

		Control* m_inputFocusControl;
		windowing::KeyModifierFlags m_inputFocusModifierMask;
		windowing::KeyStateFlags m_inputFocusStateMask;
	};

} // namespace vanadium::ui