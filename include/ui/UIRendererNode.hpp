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
		T::ShapeRegistry;
	}
	&&std::derived_from<T, Shape>;

	class UIRendererNode : public graphics::FramegraphNode {
	  public:
		UIRendererNode() {}

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
	  	graphics::FramegraphContext* m_framegraphContext;
		graphics::RenderPassSignature m_uiPassSignature;
		VkRenderPass m_uiRenderPass;

		graphics::ImageResourceViewInfo m_swapchainImageResourceViewInfo;
		std::vector<VkFramebuffer> m_imageFramebuffers;

		robin_hood::unordered_map<size_t, ShapeRegistry*> m_shapeRegistries;
	};

	template <RenderableShape T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* UIRendererNode::constructShape(uint32_t childDepth, Args... args) {
		T* shape = new T(args...);
		if (m_shapeRegistries.find(shape->typenameHash()) == m_shapeRegistries.end()) {
			m_shapeRegistries.insert(new typename T::Registry(m_framegraphContext->renderContext(), m_uiRenderPass, m_uiPassSignature));
		}
		m_shapeRegistries[shape->typenameHash()]->addShape(shape);
	}

} // namespace vanadium::ui
