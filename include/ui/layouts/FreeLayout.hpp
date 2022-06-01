#pragma once
#include <ui/Control.hpp>
#include <vector>

namespace vanadium::ui::layouts {
	struct FreeLayout {
		static void regenerateLayout(const std::vector<Control*>& children, const Vector2& controlPosition,
									 const Vector2& controlSize) {}
	};

} // namespace vanadium::ui::layouts
