#pragma once

#include <math/Vector.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <span>
#include <variant>

namespace vanadium::ui {

	class ShapeProperty {
	  public:
		ShapeProperty(float value);
		ShapeProperty(int value);
		ShapeProperty(bool value);
		ShapeProperty(const std::string_view& value);
		ShapeProperty(const Vector2& value);
		ShapeProperty(const Vector3& value);
		ShapeProperty(const Vector4& value);

		template <typename T> bool isT() const { return static_cast<bool>(std::get_if<T>(&m_value)) }
		bool isFloat() const { return isT<float>(); }
		bool isInt() const { return isT<int>(); }
		bool isBool() const { return isT<bool>(); }
		bool isString() const { return isT<std::string>(); }
		bool isVector2() const { return isT<Vector2>(); }
		bool isVector3() const { return isT<Vector3>(); }
		bool isVector4() const { return isT<Vector4>(); }

		float asFloat() const { return *std::get_if<float>(&m_value); }
		int asInt() const { return *std::get_if<int>(&m_value); }
		bool asBool() const { return *std::get_if<bool>(&m_value); }
		const std::string& asString() const { return *std::get_if<std::string>(&m_value); }
		const Vector2& asVector2() const { return *std::get_if<Vector2>(&m_value); }
		const Vector3& asVector3() const { return *std::get_if<Vector3>(&m_value); }
		const Vector4& asVector4() const { return *std::get_if<Vector4>(&m_value); }

		float& asFloat() { return *std::get_if<float>(&m_value); }
		int& asInt() { return *std::get_if<int>(&m_value); }
		bool& asBool() { return *std::get_if<bool>(&m_value); }
		std::string& asString() { return *std::get_if<std::string>(&m_value); }
		Vector2& asVector2() { return *std::get_if<Vector2>(&m_value); }
		Vector3& asVector3() { return *std::get_if<Vector3>(&m_value); }
		Vector4& asVector4() { return *std::get_if<Vector4>(&m_value); }

		template <typename T> void setValue(const T& value) { m_value = value; }

	  private:
		std::variant<float, int, bool, std::string, Vector2, Vector3, Vector4> m_value;
	};

	// prevent allocations when fetching relative/absolute position
	static std::string relativePosName = "relativePos";
	static std::string absolutePosName = "absolutePos";

	// Always-defined properties:
	// relativePos: relative position
	// absolutePos: absolute position
	// shapeIndex: index of shape in shape data array (copying data can be skipped if neither data nor index changed)
	// name: name of shape instance (NOT name of the shape passed in constructor)
	class Shape {
	  public:
		virtual ~Shape() = 0;

		Vector2 relativePos() const;
		Vector2& relativePos() { return m_properties["relativePos"].asVector2(); }
		Vector2 absolutePos() const;
		Vector2& absolutePos() { return m_properties["absolutePos"].asVector2(); }

		size_t nameHash() const { return m_nameHash; }

		const Shape* parent() const { return m_parent; }
		std::vector<Shape*>& children() { return m_children; }

		bool dirtyFlag() const { return m_dirtyFlag; }
		void clearDirtyFlag() { m_dirtyFlag = false; }

		ShapeProperty property(const std::string_view& name) const;
		ShapeProperty& property(const std::string& name) { return m_properties[name]; }

		void setProperty(const std::string& name, const ShapeProperty& property);

		virtual void render() = 0;

	  protected:
		Shape(const std::string_view& name, const Vector2& relativePos);

		robin_hood::unordered_map<std::string, ShapeProperty> m_properties;

	  private:
		Shape* m_parent;
		std::vector<Shape*> m_children;

		size_t m_nameHash;
		bool m_dirtyFlag = true;
	};

} // namespace vanadium::ui