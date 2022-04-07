#pragma once

#include <concepts>
#include <graphics/DeviceContext.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>
#include <robin_hood.h>
#include <ui/Shape.hpp>

namespace vanadium::ui {

	class UIRenderer {
	  public:
		UIRenderer() {}

		void create(graphics::DeviceContext* context, graphics::GPUResourceAllocator* resourceAllocator,
					graphics::GPUDescriptorSetAllocator* descriptorSetAllocator,
					graphics::GPUTransferManager* transferManager, graphics::PipelineLibrary* pipelineLibrary,
					graphics::WindowSurface* windowSurface);

		template <std::derived_from<Shape> T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* constructShape(Shape* parent, Args... args);

		void registerShapePipeline(size_t shapeNameHash, uint32_t id);

	  private:
		graphics::DeviceContext* m_context;
		graphics::GPUResourceAllocator* m_resourceAllocator;
		graphics::GPUDescriptorSetAllocator* m_descriptorSetAllocator;
		graphics::GPUTransferManager* m_transferManager;
		graphics::PipelineLibrary* m_pipelineLibrary;
		graphics::WindowSurface* m_windowSurface;

		std::vector<Shape*> m_rootShapes;
		robin_hood::unordered_map<size_t, uint32_t> m_shapePipelineIDs;
	};

	template <std::derived_from<Shape> T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* UIRenderer::constructShape(Shape* parent, Args... args) {
		if (!parent) {
			m_rootShapes.push_back(new T(args...));
		} else {
			parent->children().push_back(new T(args...));
		}
	}

} // namespace vanadium::ui
