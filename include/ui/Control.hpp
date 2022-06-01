#pragma once
#include <ui/Shape.hpp>
#include <ui/util/ControlPosition.hpp>
#include <vector>
#include <windowing/WindowInterface.hpp>

namespace vanadium::ui {

	class UISubsystem;
	class Control;

	// Use functions/function pointers to make a

	using StylePFNRepositionShapes = void (*)(UISubsystem* subsystem, const Vector2& position, const Vector2& size,
											  std::vector<Shape*>& shapes);
	using StylePFNHoverStart = void (*)(UISubsystem* subsystem, std::vector<Shape*>& shapes);
	using StylePFNHoverEnd = void (*)(UISubsystem* subsystem, std::vector<Shape*>& shapes);

	template <typename T>
	concept StyleTemplate = requires(UISubsystem* subsystem, Vector2 position, Vector2 size) {
		typename T::StyleParameters;
		{
			T::createShapes(subsystem, position, size, typename T::StyleParameters())
			} -> std::same_as<std::vector<Shape*>>;
		std::convertible_to<decltype(T::repositionShapes), StylePFNRepositionShapes>;
		std::convertible_to<decltype(T::hoverStart), StylePFNHoverStart>;
		std::convertible_to<decltype(T::hoverEnd), StylePFNHoverEnd>;
	};

	using LayoutPFNRegenerate = void (*)(const std::vector<Control*>& children, const Vector2& controlPosition,
										 const Vector2& controlSize);

	template <typename T>
	concept LayoutTemplate = requires() {
		std::convertible_to<decltype(T::regenerateLayout), LayoutPFNRegenerate>;
	};

	struct ControlStyleLayoutParameters {
		// needed for shape creation
		Control* parent;
		UISubsystem* subsystem;
		ControlPosition requestedPosition;
		Vector2 requestedSize;

		
		std::vector<Shape*> shapes;
		StylePFNRepositionShapes repositionPFN;
		StylePFNHoverStart hoverStartPFN;
		StylePFNHoverEnd hoverEndPFN;
		LayoutPFNRegenerate regeneratePFN;
	};

	class Control {
	  public:
		Control(ControlStyleLayoutParameters&& parameters)
			: m_parent(parameters.parent), m_subsystem(parameters.subsystem), m_shapes(parameters.shapes),
			  m_position(parameters.requestedPosition), m_size(parameters.requestedSize),
			  m_repositionPFN(parameters.repositionPFN), m_hoverStartPFN(parameters.hoverStartPFN),
			  m_hoverEndPFN(parameters.hoverEndPFN), m_regeneratePFN(parameters.regeneratePFN) {}

		virtual void mouseButtonHandler(UISubsystem* subsystem, const Vector2& absolutePosition, uint32_t buttonID) {
			for (auto& child : m_children) {
				if (boxTest(absolutePosition,
							child->position().absoluteTopLeft(topLeftPosition(), m_size, child->size()),
							child->size())) {
					child->mouseButtonHandler(subsystem, absolutePosition, buttonID);
				}
			}
		}

		virtual void hoverHandler(UISubsystem* subsystem, const Vector2& absolutePosition) {
			for (auto& child : m_children) {
				if (boxTest(absolutePosition,
							child->position().absoluteTopLeft(topLeftPosition(), m_size, child->size()),
							child->size())) {
					child->hoverHandler(subsystem, absolutePosition);
				}
			}
		}

		virtual void keyInputHandler(UISubsystem* subsystem, uint32_t keyID, windowing::KeyModifierFlags modifierFlags,
									 windowing::KeyStateFlags keyStateFlags) {}

		virtual void charInputHandler(UISubsystem* subsystem, uint32_t unicodeCodepoint) {}

		Vector2 topLeftPosition() const {
			Vector2 parentTopLeftPosition = m_parent != nullptr ? m_parent->topLeftPosition() : Vector2(0.0f);
			return m_position.absoluteTopLeft(parentTopLeftPosition, m_parent->size(), m_size);
		}
		const ControlPosition& position() const { return m_position; }
		const Vector2& size() const { return m_size; }

	  protected:
		Control* m_parent;
		std::vector<Control*> m_children;

		UISubsystem* m_subsystem;
		std::vector<Shape*> m_shapes;

		ControlPosition m_position;
		Vector2 m_size;

		StylePFNRepositionShapes m_repositionPFN;
		StylePFNHoverStart m_hoverStartPFN;
		StylePFNHoverEnd m_hoverEndPFN;
		LayoutPFNRegenerate m_regeneratePFN;

	  private:
		bool boxTest(const Vector2& position, const Vector2& boxOrigin, const Vector2& boxExtent) {
			return position.x >= boxOrigin.x && position.x <= (boxOrigin.x + boxExtent.x) &&
				   position.y >= boxOrigin.y && position.y <= (boxOrigin.y + boxExtent.y);
		}
	};

	template <StyleTemplate Style, LayoutTemplate Layout>
	ControlStyleLayoutParameters createParameters(UISubsystem* subsystem, Control* parent,
												  const ControlPosition& requestedPosition,
												  const Vector2& requestedSize,
												  const typename Style::StyleParameters& parameters) {
		Vector2 absolutePosition = parent == nullptr ? Vector2(0.0f, 0.0f)
													 : requestedPosition.absoluteTopLeft(parent->topLeftPosition(),
																						 parent->size(), requestedSize);
		return { .parent = parent,
				 .subsystem = subsystem,
				 .requestedPosition = requestedPosition,
				 .requestedSize = requestedSize,
				 .shapes = Style::createShapes(subsystem, absolutePosition, requestedSize, parameters),
				 .repositionPFN = Style::repositionShapes,
				 .hoverStartPFN = Style::hoverStart,
				 .hoverEndPFN = Style::hoverEnd,
				 .regeneratePFN = Layout::regenerateLayout };
	}

} // namespace vanadium::ui
