#include <Engine.hpp>
#include <cmath>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>

vanadium::ui::shapes::RectShape* shapes[25 * 25];

void configureEngine(vanadium::EngineConfig& config) {
	config.setAppName("Vanadium Minimal UI");
	config.addStartupFlag(vanadium::EngineStartupFlag::UIOnly);
	/*config.overrideWindowSettings(vanadium::windowing::WindowingSettingOverride {
		.createFullScreen = true
	});*/
}

void preFramegraphInit(vanadium::Engine& engine) {}

void init(vanadium::Engine& engine) {
	float x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			shapes[i * 25 + j] = engine.uiSubsystem().addShape<vanadium::ui::shapes::RectShape>(
				vanadium::Vector3(x, y, 0.99f), vanadium::Vector2(30.0f, 10.0f), 0.0f,
				vanadium::Vector4(sinf(x * M_PI) * 0.5 + 0.5, cosf(y * M_PI) * 0.5 + 0.5, 0.0f, 1.0f));
			y += 10.0f;
		}
		x += 30.0f;
	}

	engine.uiSubsystem().addShape<vanadium::ui::shapes::TextShape>(
		vanadium::Vector3(0, 0, 0.9f), 640.0f, 0.0f, "test???", 48.0f, 0, vanadium::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

	x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			shapes[i * 25 + j]->setPosition(
				vanadium::Vector3(x + sinf(engine.elapsedTime() * (1.0f / (y + 0.2))) * 0.5,
								  y + sinf(engine.elapsedTime() * (1.0f / (x + 0.2)) + M_PI / 2.0f) * 0.5, 1.0f));
			y += 10.0f;
		}
		x += 30.0f;
	}
}

bool onFrame(vanadium::Engine& engine) { return true; }

void destroy(vanadium::Engine& engine) {}