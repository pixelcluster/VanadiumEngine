#include <ui/Control.hpp>

namespace vanadium::ui::functionalities {
	using MouseButtonHandler = void (*)(Control* triggerControl, const Vector2& absolutePosition, uint32_t buttonID);

	class ButtonFunctionality : public Functionality {
	  public:
		ButtonFunctionality(MouseButtonHandler handler) : m_mouseButtonHandler(handler) {}

		void mouseButtonHandler(UISubsystem* subsystem, Control* triggerControl, const Vector2& absolutePosition,
								uint32_t buttonID) override {
			m_mouseButtonHandler(triggerControl, absolutePosition, buttonID);
		}

	  private:
		MouseButtonHandler m_mouseButtonHandler;
	};
} // namespace vanadium::ui::functionalities

namespace vanadium::ui {
	template <typename Layout> struct IsCompatible<functionalities::ButtonFunctionality, Layout> {
		static constexpr bool value = true;
	};
} // namespace vanadium::ui
