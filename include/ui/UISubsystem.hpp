#pragma once

#include <ui/UIRendererNode.hpp>
#include <ui/FontRegistry.hpp>

namespace vanadium::ui {
	class UISubsystem {
	  public:
		UISubsystem(const graphics::RenderContext& context, bool clearBackground, const Vector4& clearValue);

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* addShape(uint32_t childDepth, Args&&... args) {
			return m_rendererNode->constructShape<T>(childDepth, args...);
		}
		void removeShape(Shape* shape) { m_rendererNode->removeShape(shape); }

		void addRendererNode(graphics::FramegraphContext& context);

	  private:
		UIRendererNode* m_rendererNode;
	};

} // namespace vanadium::ui
