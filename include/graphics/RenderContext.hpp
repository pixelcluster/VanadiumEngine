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

#include <graphics/DeviceContext.hpp>
#include <graphics/RenderTargetSurface.hpp>
#include <graphics/util/GPUDescriptorSetAllocator.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>

namespace vanadium::graphics
{
    struct RenderContext
    {
        DeviceContext* deviceContext;
		GPUResourceAllocator* resourceAllocator;
		GPUDescriptorSetAllocator* descriptorSetAllocator;
		GPUTransferManager* transferManager;
		PipelineLibrary* pipelineLibrary;
		RenderTargetSurface* targetSurface;
    };
    
} // namespace vanadium::graphics
