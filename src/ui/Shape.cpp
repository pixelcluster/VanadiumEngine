#include <ui/Shape.hpp>

namespace vanadium::ui {
	Shape::Shape(const std::string_view& typeName, const Vector3& position, float rotation)
		: m_typenameHash(std::hash<std::string_view>()(typeName)), m_position(position), m_rotation(rotation) {}

	void Shape::setPosition(const Vector3& position) {
		m_dirtyFlag = true;
		m_position = position;
	}

	void Shape::setRotation(float rotation) {
		m_dirtyFlag = true;
		m_rotation = rotation;
	}
} // namespace vanadium::ui
