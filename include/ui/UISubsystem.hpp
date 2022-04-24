#pragma once

#include <ui/FontLibrary.hpp>
#include <ui/UIRendererNode.hpp>

namespace vanadium::ui {
	class UISubsystem {
	  public:
		UISubsystem(const graphics::RenderContext& context, const std::string_view& fontLibraryFile,
					bool clearBackground, const Vector4& clearValue);

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* addShape(uint32_t childDepth, Args&&... args) {
			return m_rendererNode->constructShape<T>(childDepth, args...);
		}
		void removeShape(Shape* shape) { m_rendererNode->removeShape(shape); }

		void addRendererNode(graphics::FramegraphContext& context);

		FontLibrary& fontLibrary() { return m_fontLibrary; }

	  private:
		UIRendererNode* m_rendererNode;
		FontLibrary m_fontLibrary;
	};

} // namespace vanadium::ui
