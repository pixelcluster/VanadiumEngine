#include <ui/Shape.hpp>

namespace vanadium::ui {
	Shape::Shape(const std::string_view& typeName, const Vector2& position)
		: m_typenameHash(std::hash<std::string_view>()(typeName)), m_position(position) {}

} // namespace vanadium::ui
