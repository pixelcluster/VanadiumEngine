#pragma once

#include <ui/FontLibrary.hpp>
#include <ui/UIRendererNode.hpp>

namespace vanadium::ui {
	class UISubsystem {
	  public:
		UISubsystem(const graphics::RenderContext& context, uint32_t monitorDPIX, uint32_t monitorDPIY,
					const std::string_view& fontLibraryFile, bool clearBackground, const Vector4& clearValue);

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* addShape(Args&&... args) {
			return m_rendererNode->constructShape<T>(args...);
		}
		void removeShape(Shape* shape) { m_rendererNode->removeShape(shape); }

		void addRendererNode(graphics::FramegraphContext& context);

		FontLibrary& fontLibrary() { return m_fontLibrary; }

		uint32_t monitorDPIX() const { return m_monitorDPIX; }
		uint32_t monitorDPIY() const { return m_monitorDPIY; }

	  private:
		uint32_t m_monitorDPIX;
		uint32_t m_monitorDPIY;

		UIRendererNode* m_rendererNode;
		FontLibrary m_fontLibrary;
		windowing::WindowInterface* m_windowInterface;
	};

} // namespace vanadium::ui
