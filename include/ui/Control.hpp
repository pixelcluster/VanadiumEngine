#pragma once
#include <ui/Shape.hpp>
#include <ui/util/ControlPosition.hpp>
#include <vector>
#include <windowing/WindowInterface.hpp>
namespace vanadium::ui {

	class UISubsystem;
	class Control;

	// Use functions/function pointers to make a

	using StylePFNRepositionShapes = void (*)(UISubsystem* subsystem, uint32_t treeLevel, const Vector2& position,
											  const Vector2& size, std::vector<Shape*>& shapes);
	using StylePFNHoverStart = void (*)(UISubsystem* subsystem, std::vector<Shape*>& shapes);
	using StylePFNHoverEnd = void (*)(UISubsystem* subsystem, std::vector<Shape*>& shapes);

	// clang-format off
	template <typename T>
	concept StyleTemplate = requires(UISubsystem* subsystem, uint32_t treeLevel, Vector2 position, Vector2 size) {
		typename T::StyleParameters;
		{
			T::createShapes(subsystem, treeLevel, position, size, typename T::StyleParameters())
		} -> std::same_as<std::vector<Shape*>>;
		{ T::repositionShapes } -> std::convertible_to<StylePFNRepositionShapes>;
		{ T::hoverStart } -> std::convertible_to<StylePFNHoverStart>;
		{ T::hoverEnd } -> std::convertible_to<StylePFNHoverEnd>;
	};

	using LayoutPFNRegenerate = void (*)(const std::vector<Control*>& children, const Vector2& controlPosition,
										 const Vector2& controlSize);

	template <typename T>
	concept LayoutTemplate = requires() {
		{ T::regenerateLayout} -> std::convertible_to<LayoutPFNRegenerate>;
	};

	using MouseButtonHandler = void (*)(UISubsystem* subsystem, Control* triggerControl, void* localData,
										const Vector2& absolutePosition, uint32_t buttonID);
	using MouseHoverHandler = void (*)(UISubsystem* subsystem, Control* triggerControl, void* localData,
									   const Vector2& absolutePosition);
	using KeyInputHandler = void (*)(UISubsystem* subsystem, Control* triggerControl, void* localData, uint32_t keyID,
									 windowing::KeyModifierFlags modifierFlags, windowing::KeyState keyState);
	using CharInputHandler = void (*)(UISubsystem* subsystem, Control* triggerControl, void* localData,
									  uint32_t codepoint);

	class FunctionalityLocalData {
	  public:
		virtual ~FunctionalityLocalData() {}
	};

	template <typename T>
	concept FunctionalityTemplate = requires(T t) {
		typename T::LocalData;
		{ t.mouseButtonHandler } -> std::convertible_to<MouseButtonHandler>;
		{ t.mouseHoverHandler } -> std::convertible_to<MouseHoverHandler>; 
		{ t.keyInputHandler } -> std::convertible_to<KeyInputHandler>;
		{ t.charInputHandler } -> std::convertible_to<CharInputHandler>;
	} && std::derived_from<typename T::LocalData, FunctionalityLocalData>;

	struct ControlStyleLayoutParameters {
		// needed for shape creation
		Control* parent;
		UISubsystem* subsystem;
		ControlPosition requestedPosition;
		Vector2 requestedSize;

		FunctionalityLocalData* functionalityLocalData;

		std::vector<Shape*> shapes;
		StylePFNRepositionShapes repositionPFN;
		StylePFNHoverStart hoverStartPFN;
		StylePFNHoverEnd hoverEndPFN;
		LayoutPFNRegenerate regeneratePFN;

		MouseButtonHandler mouseButtonHandlerPFN;
		MouseHoverHandler mouseHoverHandlerPFN;
		KeyInputHandler keyInputHandlerPFN;
		CharInputHandler charInputHandlerPFN;
	};

	// to be specialized for each functionality and/or layout
	// must define constexpr static boolean member named "value" that defines if the layout and functionality are
	// compatible
	template <typename Functionality, typename Layout> struct IsCompatible { static constexpr bool value = false; };

	template <typename Functionality, typename Layout>
	concept CompatibleWith = IsCompatible<Functionality, Layout>::value;

	class Control {
	  public:
		Control(ControlStyleLayoutParameters&& parameters);
		Control(const Control&) = delete;
		Control& operator=(const Control&) = delete;
		Control(Control&&) = default;
		Control& operator=(Control&&) = default;
		~Control();

		void invokeMouseButtonHandler(UISubsystem* subsystem, const Vector2& absolutePosition, uint32_t buttonID);
		void invokeHoverHandler(UISubsystem* subsystem, const Vector2& absolutePosition);
		void invokeKeyInputHandler(UISubsystem* subsystem, uint32_t keyID, windowing::KeyModifierFlags modifierFlags,
								   windowing::KeyState keyStateFlags);
		void invokeCharInputHandler(UISubsystem* subsystem, uint32_t unicodeCodepoint);

		Vector2 topLeftPosition() const;
		const ControlPosition& position() const { return m_position; }
		const Vector2& size() const { return m_size; }

		void setPosition(const ControlPosition& position);
		void setSize(const Vector2& size);

		uint32_t treeLevel() const;

		// TODO: Child lifetime handling is a mess (move memory management to parent or UI subsystem?)
		void addChild(Control* newChild) { m_children.push_back(newChild); }

		std::vector<Shape*>& shapes() { return m_shapes; }

		void reposition();

	  protected:
		Control* m_parent;
		std::vector<Control*> m_children;

		UISubsystem* m_subsystem;
		std::vector<Shape*> m_shapes;

		FunctionalityLocalData* m_functionalityLocalData;

		ControlPosition m_position;
		Vector2 m_size;

		StylePFNRepositionShapes m_repositionPFN;
		StylePFNHoverStart m_hoverStartPFN;
		StylePFNHoverEnd m_hoverEndPFN;
		LayoutPFNRegenerate m_regeneratePFN;

		MouseButtonHandler m_mouseButtonHandlerPFN;
		MouseHoverHandler m_mouseHoverHandlerPFN;
		KeyInputHandler m_keyInputHandlerPFN;
		CharInputHandler m_charInputHandlerPFN;

	  private:
		void seekTreeLevel(uint32_t& childLevel) const {
			if (m_parent) {
				++childLevel;
				m_parent->seekTreeLevel(childLevel);
			}
		}

		bool boxTest(const Vector2& position, const Vector2& boxOrigin, const Vector2& boxExtent) {
			return position.x >= boxOrigin.x && position.x <= (boxOrigin.x + boxExtent.x) &&
				   position.y >= boxOrigin.y && position.y <= (boxOrigin.y + boxExtent.y);
		}
	};

	template <StyleTemplate Style, LayoutTemplate Layout, FunctionalityTemplate Functionality>
	requires(CompatibleWith<Functionality, Layout>) ControlStyleLayoutParameters
		createParameters(UISubsystem* subsystem, Control* parent, const ControlPosition& requestedPosition,
						 const Vector2& requestedSize, const typename Style::StyleParameters& parameters,
						 const Functionality& functionality) {
		Vector2 absolutePosition = parent == nullptr ? Vector2(0.0f, 0.0f)
													 : requestedPosition.absoluteTopLeft(parent->topLeftPosition(),
																						 parent->size(), requestedSize);
		uint32_t treeLevel = 0;
		if (parent) {
			treeLevel = parent->treeLevel() + 1;
		}
		return { .parent = parent,
				 .subsystem = subsystem,
				 .requestedPosition = requestedPosition,
				 .requestedSize = requestedSize,
				 .functionalityLocalData = new typename Functionality::LocalData,
				 .shapes = Style::createShapes(subsystem, treeLevel, absolutePosition, requestedSize, parameters),
				 .repositionPFN = Style::repositionShapes,
				 .hoverStartPFN = Style::hoverStart,
				 .hoverEndPFN = Style::hoverEnd,
				 .regeneratePFN = Layout::regenerateLayout,
				 .mouseButtonHandlerPFN = functionality.mouseButtonHandler,
				 .mouseHoverHandlerPFN = functionality.mouseHoverHandler,
				 .keyInputHandlerPFN = functionality.keyInputHandler,
				 .charInputHandlerPFN = functionality.charInputHandler };
	}

} // namespace vanadium::ui
