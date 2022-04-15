#include <Engine.hpp>
#include <cmath>
#include <ui/shapes/Rect.hpp>

vanadium::ui::shapes::RectShape* shapes[25 * 25];

void configureEngine(vanadium::EngineConfig& config) {
	config.setAppName("Vanadium Minimal UI");
	config.addStartupFlag(vanadium::EngineStartupFlag::UIOnly);
    /*config.overrideWindowSettings(vanadium::windowing::WindowingSettingOverride {
        .createFullScreen = true
    });*/
}

void preFramegraphInit(vanadium::Engine& engine) {

}

void init(vanadium::Engine& engine) {
	float x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			shapes[i * 25 + j] = engine.uiSubsystem().addShape<vanadium::ui::shapes::RectShape>(
				0U, vanadium::Vector2(x, y), vanadium::Vector2(1.0f / 26.0f, 1.0f / 26.0f),
				vanadium::Vector4(sinf(x * M_PI) * 0.5 + 0.5, cosf(y * M_PI) * 0.5 + 0.5, 0.0f, 1.0f));
			y += 1.0f / 26.0f;
		}
		x += 1.0f / 26.0f;
	}
}

bool onFrame(vanadium::Engine& engine) {
	float x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			shapes[i * 25 + j]->position() =
				vanadium::Vector2(x + sinf(engine.elapsedTime() * (1.0f / (y + 0.2))) * 0.5,
								  y + sinf(engine.elapsedTime() * (1.0f / (x + 0.2)) + M_PI / 2.0f) * 0.5);
			y += 1.0f / 26.0f;
		}
		x += 1.0f / 26.0f;
	}
	return true;
}

void destroy(vanadium::Engine& engine) {}