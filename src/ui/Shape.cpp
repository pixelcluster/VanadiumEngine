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
