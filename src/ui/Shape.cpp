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
#include <ui/Shape.hpp>

namespace vanadium::ui {
	Shape::Shape(const std::string_view& typeName, uint32_t layerIndex, const Vector2& position, float rotation)
		: m_typenameHash(std::hash<std::string_view>()(typeName)), m_position(position), m_rotation(rotation), m_layerIndex(layerIndex) {}

	void Shape::setPosition(const Vector2& position) {
		m_dirtyFlag = true;
		m_position = position;
	}

	void Shape::setRotation(float rotation) {
		m_dirtyFlag = true;
		m_rotation = rotation;
	}
	void Shape::setLayerIndex(uint32_t layerIndex) {
		m_dirtyFlag = true;
		m_layerIndex = layerIndex;
	}
} // namespace vanadium::ui
