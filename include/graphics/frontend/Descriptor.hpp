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

#include <graphics/frontend/Resource.hpp>

namespace vanadium::graphics::rhi {

	enum class DescriptorType {
		Sampler,
		SampledImage,
		CombinedImageSampler,
		StorageImage,
		ReadonlyBuffer,
		ReadWriteBuffer
	};

	class DescriptorSet {
	  public:
		virtual ~DescriptorSet() = 0;

		virtual size_t descriptorCount() const = 0;
		virtual DescriptorType descriptorType(size_t index) const = 0;

		virtual void writeSampler(size_t index, Sampler* sampler) = 0;
		virtual void writeSampledImage(size_t index, ImageView* view) = 0;
		virtual void writeCombinedImageSampler(size_t index, ImageView* view, Sampler* sampler) = 0;
		virtual void writeStorageImage(size_t index, ImageView* view) = 0;
		virtual void writeReadonlyBuffer(size_t index, Buffer* buffer) = 0;
		virtual void writeReadWriteBuffer(size_t index, Buffer* buffer) = 0;

	  private:
	};
} // namespace vanadium::graphics::rhi
