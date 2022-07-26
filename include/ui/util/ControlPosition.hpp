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
#include <math/Vector.hpp>
#include <util/CompilerSpecific.hpp>

namespace vanadium::ui {
	enum class PositionOffsetType {
		// Origin is at the top left, coordinate axes point to the right and down
		TopLeft,
		// Origin is at the top right, coordinate axes point to the left and down
		TopRight,
		// Origin is at the bottom left, coordinate axes point to the right and up
		BottomLeft,
		// Origin is at the bottom right, coordinate axes point to the left and up
		BottomRight,
	};

	class ControlPosition {
	  public:
		// position should be normalized in [0;1]
		ControlPosition(PositionOffsetType offsetType, const Vector2& position)
			: m_offsetType(offsetType), m_position(position) {}

		Vector2 absolutePosition(const Vector2& origin, const Vector2& extents) const {
			switch (m_offsetType) {
				case PositionOffsetType::TopLeft:
					return origin + m_position * extents;
				case PositionOffsetType::TopRight:
					return Vector2(origin.x + extents.x - m_position.x * extents.x,
								   origin.y + m_position.y * extents.y);
				case PositionOffsetType::BottomLeft:
					return Vector2(origin.x + m_position.x * extents.x,
								   origin.y + extents.y - m_position.y * extents.y);
				case PositionOffsetType::BottomRight:
					return Vector2(origin.x + extents.x - m_position.x * extents.x,
								   origin.y + extents.y - m_position.y * extents.y);
			}
			UNREACHABLE
		}

		Vector2 absoluteTopLeft(const Vector2& origin, const Vector2& parentExtent, const Vector2& shapeExtent) const {
			switch (m_offsetType) {
				case PositionOffsetType::TopLeft:
					return absolutePosition(origin, parentExtent);
				case PositionOffsetType::TopRight:
					return absolutePosition(origin, parentExtent) - Vector2(shapeExtent.x, 0.0f);
				case PositionOffsetType::BottomLeft:
					return absolutePosition(origin, parentExtent) - Vector2(0.0f, shapeExtent.y);
				case PositionOffsetType::BottomRight:
					return absolutePosition(origin, parentExtent) - shapeExtent;
			}
			UNREACHABLE
		}

	  private:
		PositionOffsetType m_offsetType;
		Vector2 m_position;
	};

} // namespace vanadium::ui
