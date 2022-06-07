#pragma once

#include <ui/Shape.hpp>
#include <ui/UISubsystem.hpp>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>
#include <vector>

namespace vanadium::ui::styles {
	struct SimpleRectStyle {
		struct StyleParameters {
			Vector4 color;
		};
		static std::vector<Shape*> createShapes(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position,
												const Vector2& size, const StyleParameters& parameters) {
			return { subsystem->addShape<vanadium::ui::shapes::RectShape>(position, treeLevel, size, 0.0f,
																		  Vector4(1.0f, 0.0f, 0.0f, 1.0f)) };
		}
		static void repositionShapes(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position,
									 const Vector2& size, std::vector<Shape*>& shapes) {
			shapes[0]->setPosition(position);
			reinterpret_cast<vanadium::ui::shapes::RectShape*>(shapes[0])->setSize(size);
		}
		static void hoverStart(UISubsystem* subsystem, std::vector<Shape*>& shapes) {}
		static void hoverEnd(UISubsystem* subsystem, std::vector<Shape*>& shapes) {}
	};

	struct TextRectStyle {
		struct StyleParameters {
			std::string_view text;
			uint32_t fontID;
			float fontSize;
			Vector4 color;
		};
		static std::vector<Shape*> createShapes(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position,
												const Vector2& size, const StyleParameters& parameters) {
			vanadium::ui::shapes::TextShape* shape = subsystem->addShape<vanadium::ui::shapes::TextShape>(
				position, treeLevel, size.x, 0.0f, parameters.text, parameters.fontSize, parameters.fontID,
				parameters.color);
			shape->setPosition(position + (size - shape->size()) / 2.0f);
			return {
				subsystem->addShape<vanadium::ui::shapes::RectShape>(position, treeLevel, size, 0.0f, parameters.color),
				shape
			};
		}
		static void repositionShapes(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position,
									 const Vector2& size, std::vector<Shape*>& shapes) {
			shapes[0]->setPosition(position);
			reinterpret_cast<vanadium::ui::shapes::RectShape*>(shapes[0])->setSize(size);
			shapes[1]->setPosition(position);
			reinterpret_cast<vanadium::ui::shapes::TextShape*>(shapes[1])->setMaxWidth(size.x);
		}
		static void hoverStart(UISubsystem* subsystem, std::vector<Shape*>& shapes) {}
		static void hoverEnd(UISubsystem* subsystem, std::vector<Shape*>& shapes) {}

		static void setText(std::vector<Shape*>& shapes, const std::string_view& text) {
			reinterpret_cast<vanadium::ui::shapes::TextShape*>(shapes[1])->setText(text);
		}
	};

} // namespace vanadium::ui::styles
