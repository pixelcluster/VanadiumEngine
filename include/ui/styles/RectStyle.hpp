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
#include <ui/shapes/DropShadowRect.hpp>
#include <ui/shapes/FilledRoundedRect.hpp>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>

namespace vanadium::ui::styles {
	class SimpleRectStyle : public Style {
		SimpleRectStyle(const Vector4& color) : m_color(color) {}

		void createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
						  const Vector2& size) override {
			m_shape = subsystem->addShape<vanadium::ui::shapes::RectShape>(position, layerID, size, 0.0f, m_color);
		}
		void repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
							  const Vector2& size) override {
			m_shape->setPosition(position);
			m_shape->setSize(size);
			m_shape->setLayerIndex(layerID);
		}

	  private:
		Vector4 m_color;
		shapes::RectShape* m_shape;
	};

	class TextRectStyle : public Style {
	  public:
		TextRectStyle(std::string_view text, uint32_t fontID, float fontSize, const Vector4& color)
			: m_text(text), m_fontID(fontID), m_fontSize(fontSize), m_color(color) {}

		void createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
						  const Vector2& size) override {
			m_textShape = subsystem->addShape<vanadium::ui::shapes::TextShape>(position, layerID, size.x, 0.0f, m_text,
																			   m_fontSize, m_fontID, m_color);
			m_textShape->setPosition(position + (size - m_textShape->size()) / 2.0f);
			m_rectShape = subsystem->addShape<vanadium::ui::shapes::RectShape>(position, layerID, size, 0.0f, m_color);
		}
		void repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
							  const Vector2& size) override {
			m_rectShape->setPosition(position);
			m_rectShape->setSize(size);
			m_rectShape->setLayerIndex(layerID);
			m_textShape->setPosition(position + (size - m_textShape->size()) / 2.0f);
			m_textShape->setMaxWidth(size.x);
			m_textShape->setLayerIndex(layerID);
		}

		void setText(const std::string_view& text) { m_textShape->setText(text); }

	  private:
		std::string_view m_text;
		uint32_t m_fontID;
		float m_fontSize;
		Vector4 m_color;

		shapes::RectShape* m_rectShape;
		shapes::TextShape* m_textShape;
	};

	class TextRoundedRectStyle : public Style {
	  public:
		TextRoundedRectStyle(std::string_view text, uint32_t fontID, float fontSize, const Vector4& textColor, const Vector4& rectColor,
							 float edgeSize)
			: m_text(text), m_fontID(fontID), m_fontSize(fontSize), m_textColor(textColor), m_rectColor(rectColor), m_edgeSize(edgeSize) {}

		void createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
						  const Vector2& size) override {
			m_textShape = subsystem->addShape<vanadium::ui::shapes::TextShape>(position, layerID + 1, size.x, 0.0f, m_text,
																			   m_fontSize, m_fontID, m_textColor);
			m_textShape->setPosition(position + (size - m_textShape->size()) / 2.0f);
			m_rectShape = subsystem->addShape<vanadium::ui::shapes::FilledRoundedRectShape>(position, layerID, size,
																							0.0f, m_edgeSize, m_rectColor);
		}
		void repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
							  const Vector2& size) override {
			m_rectShape->setPosition(position);
			m_rectShape->setSize(size);
			m_rectShape->setLayerIndex(layerID);
			m_textShape->setPosition(position + (size - m_textShape->size()) / 2.0f);
			m_textShape->setMaxWidth(size.x);
			m_textShape->setLayerIndex(layerID + 1);
		}

		void setText(const std::string_view& text) { m_textShape->setText(text); }

		uint32_t layerCount() const override { return 2; }

	  private:
		std::string_view m_text;
		uint32_t m_fontID;
		float m_fontSize;
		Vector4 m_textColor;
		Vector4 m_rectColor;
		float m_edgeSize;

		shapes::FilledRoundedRectShape* m_rectShape;
		shapes::TextShape* m_textShape;
	};

	class TextDropShadowStyle : public Style {
	  public:
		TextDropShadowStyle(std::string_view text, uint32_t fontID, float fontSize, Vector4 color,
							Vector2 shadowPeakPos, float maxOpacity)
			: m_text(text), m_fontID(fontID), m_fontSize(fontSize), m_color(color), m_shadowPeakPos(shadowPeakPos),
			  m_maxOpacity(maxOpacity) {}

		void createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
						  const Vector2& size) override {
			m_textShape = subsystem->addShape<vanadium::ui::shapes::TextShape>(position, layerID + 1, size.x, 0.0f,
																			   m_text, m_fontSize, m_fontID, m_color);
			Vector2 contentOrigin = position + m_shadowPeakPos * size;
			Vector2 contentSize = size * (Vector2(1.0f) - Vector2(2.0f) * m_shadowPeakPos);
			m_textShape->setPosition(contentOrigin + (contentSize - m_textShape->size()) / 2.0f);
			m_rectShape = subsystem->addShape<vanadium::ui::shapes::DropShadowRectShape>(position, layerID, size, 0.0f,
																						 m_shadowPeakPos, m_maxOpacity);
		}
		void repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
							  const Vector2& size) override {
			m_rectShape->setPosition(position);
			m_rectShape->setLayerIndex(layerID);
			m_rectShape->setSize(size);
			Vector2 contentOrigin = position + m_shadowPeakPos * size;
			Vector2 contentSize = size * (Vector2(1.0f) - Vector2(2.0f) * m_shadowPeakPos);
			m_textShape->setPosition(contentOrigin + (contentSize - m_textShape->size()) / 2.0f);
			m_textShape->setMaxWidth(size.x);
			m_textShape->setLayerIndex(layerID + 1);
		}

		void setText(const std::string_view& text) { m_textShape->setText(text); }

		void setShadowPeakPos(const Vector2& shadowPeakPos) {
			m_shadowPeakPos = shadowPeakPos;
			m_rectShape->setShadowPeakPos(shadowPeakPos);
		}

		uint32_t layerCount() const override { return 2; }

	  private:
		std::string_view m_text;
		uint32_t m_fontID;
		float m_fontSize;
		Vector4 m_color;
		Vector2 m_shadowPeakPos;
		float m_maxOpacity;

		shapes::DropShadowRectShape* m_rectShape;
		shapes::TextShape* m_textShape;
	};

} // namespace vanadium::ui::styles
