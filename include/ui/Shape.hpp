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

		Vector3 position() const { return m_position; }
		void setPosition(const Vector3& position);
		float rotation() const { return m_rotation; }
		void setRotation(float rotation);

		size_t typenameHash() const { return m_typenameHash; }

		bool dirtyFlag() const { return m_dirtyFlag; }
		void clearDirtyFlag() { m_dirtyFlag = false; }

	  protected:
		Shape(const std::string_view& typeName, const Vector3& relativePos, float rotation);

	  private:
		size_t m_typenameHash;
		bool m_dirtyFlag = true;

		Vector3 m_position;
		float m_rotation;
	};

} // namespace vanadium::ui