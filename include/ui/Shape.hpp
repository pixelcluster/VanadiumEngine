#pragma once

#include <math/Vector.hpp>

namespace vanadium::ui {
	class Shape {
	  public:
		Shape(const std::string_view& name);
		virtual ~Shape() = 0;

		const Vector2& relativePos() const { return m_relativePos; }
		Vector2& relativePos() { return m_relativePos; }
		const Vector2& absolutePos() const { return m_absolutePos; }
		Vector2& absolutePos() { return m_absolutePos; }

	  private:
		Vector2 m_relativePos;
		Vector2 m_absolutePos;
	};

} // namespace vanadium::ui