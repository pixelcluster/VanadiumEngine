#pragma once

#include <math/Vector.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <span>
#include <string_view>

namespace vanadium::ui {
	// Subclasses need to define a Registry typedef that points to a subclass of ShapeRegistry
	// For a subclass Foo, the type Foo::Registry must be a subclass of ShapeRegistry
	class Shape {
	  public:
		virtual ~Shape() {}

		//Position in pixels. Origin (0, 0) is the upper left corner.
		Vector2 position() const { return m_position; }
		void setPosition(const Vector2& position);
		float rotation() const { return m_rotation; }
		void setRotation(float rotation);
		uint32_t layerIndex() const { return m_layerIndex; }
		void setLayerIndex(uint32_t layerIndex);

		size_t typenameHash() const { return m_typenameHash; }

		bool dirtyFlag() const { return m_dirtyFlag; }
		void clearDirtyFlag() { m_dirtyFlag = false; }

	  protected:
		Shape(const std::string_view& typeName, uint32_t layerIndex, const Vector2& relativePos, float rotation);
		bool m_dirtyFlag = true;
	  private:
		size_t m_typenameHash;

		Vector2 m_position;
		float m_rotation;
		uint32_t m_layerIndex;
	};

} // namespace vanadium::ui