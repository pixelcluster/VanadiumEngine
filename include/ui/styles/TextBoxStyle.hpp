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
#include <ui/Shape.hpp>
#include <ui/UISubsystem.hpp>
#include <ui/shapes/FilledRoundedRect.hpp>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>

namespace vanadium::ui::styles {
	class TextBoxStyle : public Style {
	  public:
		TextBoxStyle(uint32_t fontID) : m_fontID(fontID) {}
		shapes::RectShape* cursorShape() { return m_cursorShape; }
		shapes::TextShape* inputTextShape() { return m_textShape; }
		void incrementCursorGlyphIndex();
		void decrementCursorGlyphIndex();
		void eraseLetterBefore();
		void eraseLetterAfter();
		void toggleCursor();
		void disableCursor() { m_cursorShape->setColor(Vector4(0.0f)); }
		void recalculateTextPosition();

	  protected:
		void recalculateCursorOffset();
		uint32_t m_fontID;
		shapes::RectShape* m_cursorShape;
		shapes::TextShape* m_textShape;

		uint32_t m_cursorGlyphIndex = 0;
		Vector2 m_currentCursorOffset = Vector2(0.0f);
		Vector2 m_contentOrigin = Vector2(0.0f);
		Vector2 m_contentSize = Vector2(0.0f);
	};

	class RoundedTextBoxStyle : public TextBoxStyle {
	  public:
		RoundedTextBoxStyle(uint32_t fontID, float edgeSize) : TextBoxStyle(fontID), m_edgeSize(edgeSize) {}

		void createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
						  const Vector2& size) override;

		void repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
							  const Vector2& size) override;

		uint32_t layerCount() const override { return 3; }

	  private:
		float m_edgeSize;
		uint32_t m_layerIndex;

		shapes::FilledRoundedRectShape* m_backgroundShape;
		shapes::FilledRoundedRectShape* m_outlineShape;
	};

} // namespace vanadium::ui::styles
