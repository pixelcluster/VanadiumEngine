#pragma once
#include <Engine.hpp>
#include <ui/Control.hpp>
#include <ui/styles/TextBoxStyle.hpp>

namespace vanadium::ui::functionalities {
	class TextBoxFunctionality : public Functionality {
	  public:
		TextBoxFunctionality(Engine* engine) : m_engine(engine) {}
		~TextBoxFunctionality();

		virtual void keyInputHandler(UISubsystem* subsystem, Control* triggeringControl, uint32_t keyID,
									 windowing::KeyModifierFlags modifierFlags, windowing::KeyState keyState) override;
		virtual void charInputHandler(UISubsystem* subsystem, Control* triggeringControl, uint32_t codepoint) override;

		virtual KeyMask keyInputMask() const override;
		SimpleVector<uint32_t> keyCodes() const override;

		virtual void inputFocusGained(UISubsystem* subsystem, Control* triggeringControl) override;
		virtual void inputFocusLost(UISubsystem* subsystem, Control* triggeringControl) override;

	  private:
		Engine* m_engine;
		timers::TimerHandle m_timer = ~0U;
	};
} // namespace vanadium::ui::functionalities

namespace vanadium::ui {
	template <std::derived_from<styles::TextBoxStyle> Style> struct IsCompatible<functionalities::TextBoxFunctionality, Style> {
		static constexpr bool value = true;
	};
} // namespace vanadium::ui

