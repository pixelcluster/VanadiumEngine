/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <concepts>
#include <graphics/DeviceContext.hpp>
#include <graphics/framegraph/FramegraphNode.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>
#include <robin_hood.h>
#include <ui/ShapeRegistry.hpp>

namespace vanadium::ui {

	class UISubsystem;

	template <typename T>
	concept RenderableShape = requires(T t) {
		typename T::ShapeRegistry;
	} && std::derived_from<T, Shape>;

	class UIRendererNode : public graphics::FramegraphNode {
	  public:
		UIRendererNode(UISubsystem* subsystem, const graphics::RenderContext& context) : m_renderContext(context), m_subsystem(subsystem)  {}
		UIRendererNode(UISubsystem* subsystem, const graphics::RenderContext& context, const Vector4& backgroundClearColor)
			: m_renderContext(context), m_subsystem(subsystem) , m_backgroundClearColor(backgroundClearColor) {}

		void create(graphics::FramegraphContext* context) override;

		void recordCommands(graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
							const graphics::FramegraphNodeContext& nodeContext) override;

		void recreateSwapchainResources(graphics::FramegraphContext* context, uint32_t width, uint32_t height) override;

		void destroy(graphics::FramegraphContext* context) override;

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* constructShape(Args... args);

		void removeShape(Shape* shape);

	  private:
		graphics::RenderContext m_renderContext;
		graphics::FramegraphContext* m_framegraphContext;
		graphics::RenderPassSignature m_uiPassSignature;
		UISubsystem* m_subsystem;
		VkRenderPass m_uiRenderPass;

		graphics::ImageResourceViewInfo m_attachmentResourceViewInfo;
		std::vector<VkFramebuffer> m_imageFramebuffers;

		robin_hood::unordered_map<size_t, ShapeRegistry*> m_shapeRegistries;

		Vector4 m_backgroundClearColor = Vector4(0.0f);
	};

	template <RenderableShape T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* UIRendererNode::constructShape(Args... args) {
		T* shape = new T(args...);
		if (m_shapeRegistries.find(shape->typenameHash()) == m_shapeRegistries.end()) {
			m_shapeRegistries.insert(robin_hood::pair<size_t, ShapeRegistry*>(
				shape->typenameHash(), new
				typename T::ShapeRegistry(m_subsystem, m_framegraphContext->renderContext(), m_uiRenderPass, m_uiPassSignature)));
		}
		m_shapeRegistries[shape->typenameHash()]->addShape(shape);
		return shape;
	}

} // namespace vanadium::ui
