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
	template <typename Style> struct IsCompatible<functionalities::ButtonFunctionality, Style> {
		static constexpr bool value = true;
	};
} // namespace vanadium::ui
