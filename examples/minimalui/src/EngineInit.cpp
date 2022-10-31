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
#include <Engine.hpp>
#include <Log.hpp>
#include <cmath>
#include <ui/functionalities/ButtonFunctionality.hpp>
#include <ui/functionalities/TextBoxFunctionality.hpp>
#include <ui/shapes/DropShadowRect.hpp>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>
#include <ui/styles/RectStyle.hpp>
#include <ui/styles/TextBoxStyle.hpp>

vanadium::ui::shapes::RectShape* shapes[25 * 25];
vanadium::ui::Control* buttonControl;

void configureEngine(vanadium::EngineConfig& config) {
	config.setAppName("Vanadium Minimal UI");
	config.setUIBackgroundColor(vanadium::Vector4(1.0f));
}

void mouseButtonOutput(vanadium::ui::Control* triggerControl, const vanadium::Vector2& absolutePosition,
					   uint32_t buttonID) {
	vanadium::logInfo(vanadium::SubsystemID::Unknown, "Button was clicked!!");
	reinterpret_cast<vanadium::ui::styles::TextRoundedRectStyle*>(triggerControl->style())->setText("I was clicked!");
}

void init(vanadium::Engine& engine) {
	/*float x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			shapes[i * 25 + j] = engine.uiSubsystem().addShape<vanadium::ui::shapes::RectShape>(
				vanadium::Vector2(x, y), 0, vanadium::Vector2(30.0f, 10.0f), 13.0f,
				vanadium::Vector4(sinf(x * M_PI) * 0.5 + 0.5, cosf(y * M_PI) * 0.5 + 0.5, 0.0f, 1.0f));
			y += 30.0f;
		}
		x += 50.0f;
	}*/

	buttonControl = new vanadium::ui::Control(
		&engine.uiSubsystem(), engine.uiSubsystem().rootControl(),
		vanadium::ui::ControlPosition(vanadium::ui::PositionOffsetType::BottomRight, vanadium::Vector2(0.05, 0.05)),
		vanadium::Vector2(220, 30), vanadium::ui::createStyle<vanadium::ui::styles::RoundedTextBoxStyle>(0, 0.1),
		vanadium::ui::createLayout<vanadium::ui::Layout>(),
		vanadium::ui::createFunctionality<vanadium::ui::styles::RoundedTextBoxStyle,
										  vanadium::ui::functionalities::TextBoxFunctionality>(&engine));
}

bool onFrame(vanadium::Engine& engine) {
	/*float x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			shapes[i * 25 + j]->setPosition(
				vanadium::Vector2(x + sinf(engine.elapsedTime() + x / (x + y)) * 50,
								  y + sinf(engine.elapsedTime() + (y + x) / y + M_PI / 2.0f) * 50));
			shapes[i * 25 + j]->setRotation(shapes[i * 25 + j]->rotation() + 6.0f * engine.deltaTime());
			y += 10.0f;
		}
		x += 30.0f;
	}*/
	return true;
}

void destroy(vanadium::Engine& engine) { delete buttonControl; }
