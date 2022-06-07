#pragma once

#include <ui/Control.hpp>

namespace vanadium::ui::functionalities {
	struct NullFunctionality {
		static void mouseButtonHandler(UISubsystem* subsystem, Control* triggeringControl, void* localData,
									   const Vector2& absolutePosition, uint32_t buttonID) {}
		static void mouseHoverHandler(UISubsystem* subsystem, Control* triggeringControl, void* localData,
									  const Vector2& absolutePosition) {}
		static void keyInputHandler(UISubsystem* subsystem, Control* triggeringControl, void* localData, uint32_t keyID,
									windowing::KeyModifierFlags modifierFlags, windowing::KeyState keyStateFlags) {
		}
		static void charInputHandler(UISubsystem* subsystem, Control* triggeringControl, void* localData, uint32_t codepoint) {}

		struct LocalData : public FunctionalityLocalData {};
	};
} // namespace vanadium::ui::functionalities

namespace vanadium::ui {
	template <typename Layout> struct IsCompatible<functionalities::NullFunctionality, Layout> {
		static constexpr bool value = true;
	};
} // namespace vanadium::ui
