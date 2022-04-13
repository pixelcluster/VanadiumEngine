#pragma once

#include <math/Vector.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <span>
#include <string_view>

namespace vanadium::ui {
	//Subclasses need to define a Registry typedef that points to a subclass of ShapeRegistry
	//For a subclass Foo, the type Foo::Registry must be a subclass of ShapeRegistry
	class Shape {
	  public:
		virtual ~Shape() = 0;

		Vector2 position() const { return m_position; }
		Vector2& position() { return m_position; }

		size_t typenameHash() const { return m_typenameHash; }

		bool dirtyFlag() const { return m_dirtyFlag; }
		void clearDirtyFlag() { m_dirtyFlag = false; }

		virtual void render() = 0;

	  protected:
		Shape(const std::string_view& typeName, const Vector2& relativePos);

	  private:
		size_t m_typenameHash;
		bool m_dirtyFlag = true;

		Vector2 m_position;
	};

} // namespace vanadium::ui