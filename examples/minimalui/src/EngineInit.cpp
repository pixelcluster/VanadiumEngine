#include <Engine.hpp>
#include <Log.hpp>
#include <cmath>
#include <ui/functionalities/ButtonFunctionality.hpp>
#include <ui/shapes/DropShadowRect.hpp>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>
#include <ui/styles/RectStyle.hpp>

vanadium::ui::shapes::RectShape* shapes[25 * 25];
vanadium::ui::Control* buttonControl;

void configureEngine(vanadium::EngineConfig& config) {
	config.setAppName("Vanadium Minimal UI");
}

void preFramegraphInit(vanadium::Engine& engine) {}

void mouseButtonOutput(vanadium::ui::Control* triggerControl, const vanadium::Vector2& absolutePosition,
					   uint32_t buttonID) {
	vanadium::logInfo("Button was clicked!!");
	reinterpret_cast<vanadium::ui::styles::TextDropShadowStyle*>(triggerControl->style())->setText("I was clicked!");
}

void init(vanadium::Engine& engine) {
	float x = 0.0f;
	/*for (uint32_t i = 0; i < 25; ++i) {
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
		vanadium::Vector2(80, 50),
		vanadium::ui::createStyle<vanadium::ui::styles::TextDropShadowStyle>("OK", 0, 24.0f,
																			 vanadium::Vector4(0.0f, 0.0f, 0.0f, 1.0f),
																			 vanadium::Vector2(0.12f, 0.14f), 0.7f),
		vanadium::ui::createLayout<vanadium::ui::Layout>(),
		vanadium::ui::createFunctionality<vanadium::ui::functionalities::ButtonFunctionality>(mouseButtonOutput));
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
	}
	textShape->setRotation(textShape->rotation() + 38.0f * engine.deltaTime());*/
	return true;
}

void destroy(vanadium::Engine& engine) { delete buttonControl; }
