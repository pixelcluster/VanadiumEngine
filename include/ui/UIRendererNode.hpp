#pragma once

#include <concepts>
#include <graphics/DeviceContext.hpp>
#include <graphics/framegraph/FramegraphNode.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>
#include <robin_hood.h>
#include <ui/ShapeRegistry.hpp>

namespace vanadium::ui {

	template <typename T>
	concept RenderableShape = requires(T t) {
		typename T::ShapeRegistry;
	}
	&&std::derived_from<T, Shape>;

	class UIRendererNode : public graphics::FramegraphNode {
	  public:
		UIRendererNode(const graphics::RenderContext& context) : m_renderContext(context) {}
		UIRendererNode(const graphics::RenderContext& context, const Vector4& backgroundClearColor)
			: m_renderContext(context), m_backgroundClearColor(backgroundClearColor) {}

		void create(graphics::FramegraphContext* context) override;

		void setupResources(graphics::FramegraphContext* context) override;

		void initResources(graphics::FramegraphContext* context) override;

		void recordCommands(graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
							const graphics::FramegraphNodeContext& nodeContext) override;

		void handleWindowResize(graphics::FramegraphContext* context, uint32_t width, uint32_t height) override;

		void destroy(graphics::FramegraphContext* context) override;

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* constructShape(uint32_t childDepth, Args... args);

		void removeShape(Shape* shape);

	  private:
		graphics::RenderContext m_renderContext;
		graphics::FramegraphContext* m_framegraphContext;
		graphics::RenderPassSignature m_uiPassSignature;
		VkRenderPass m_uiRenderPass;

		graphics::ImageResourceViewInfo m_swapchainImageResourceViewInfo;
		std::vector<VkFramebuffer> m_imageFramebuffers;

		size_t m_lastRenderedFrameIndex = ~0U;
		size_t m_framebufferDeletionFrameIndex = ~0U;
		std::vector<VkFramebuffer> m_oldImageFramebuffers;

		robin_hood::unordered_map<size_t, ShapeRegistry*> m_shapeRegistries;

		Vector4 m_backgroundClearColor = Vector4(0.0f);
	};

	template <RenderableShape T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* UIRendererNode::constructShape(uint32_t childDepth, Args... args) {
		T* shape = new T(args...);
		if (m_shapeRegistries.find(shape->typenameHash()) == m_shapeRegistries.end()) {
			m_shapeRegistries.insert(robin_hood::pair<size_t, ShapeRegistry*>(
				shape->typenameHash(), new
				typename T::ShapeRegistry(m_framegraphContext->renderContext(), m_uiRenderPass, m_uiPassSignature)));
		}
		m_shapeRegistries[shape->typenameHash()]->addShape(shape, childDepth);
		return shape;
	}

} // namespace vanadium::ui
