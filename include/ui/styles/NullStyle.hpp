#pragma once

#include <ui/Shape.hpp>
#include <ui/UISubsystem.hpp>
#include <vector>

namespace vanadium::ui::styles {
	struct NullStyle {
		struct StyleParameters {};
		static std::vector<Shape*> createShapes(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position, const Vector2& size,
												const StyleParameters& parameters) {
			return {};
		}
		static void repositionShapes(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position, const Vector2& size,
									 std::vector<Shape*>& shapes) {}
		static void hoverStart(UISubsystem* subsystem, std::vector<Shape*>& shapes) {}
		static void hoverEnd(UISubsystem* subsystem, std::vector<Shape*>& shapes) {}
	};

} // namespace vanadium::ui::styles
