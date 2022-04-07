#include <ui/Shape.hpp>

namespace vanadium::ui {
	Shape::Shape(const std::string_view& name, const Vector2& relativePos)
		: m_nameHash(std::hash<std::string_view>()(name)) {
		m_properties.insert({ relativePosName, relativePos });
		m_properties.insert({ absolutePosName, Vector2() });
	}

	void Shape::setProperty(const std::string& name, const ShapeProperty& property) {
		m_properties[name] = property;
		m_dirtyFlag = true;
	}
} // namespace vanadium::ui
