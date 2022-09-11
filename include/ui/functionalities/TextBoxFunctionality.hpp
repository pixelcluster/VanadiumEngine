/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
									 windowing::KeyModifier modifierFlags, windowing::KeyState keyState) override;
		virtual void charInputHandler(UISubsystem* subsystem, Control* triggeringControl, uint32_t codepoint) override;

		virtual KeyMask keyInputMask() const override;
		std::vector<uint32_t> keyCodes() const override;

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
