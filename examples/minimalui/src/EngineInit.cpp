#include <Engine.hpp>
#include <cmath>
#include <ui/shapes/Rect.hpp>
#include <ui/shapes/Text.hpp>

vanadium::ui::shapes::RectShape* shapes[25 * 25];
vanadium::ui::shapes::TextShape* textShape;

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
				vanadium::Vector2(x, y), 0, vanadium::Vector2(30.0f, 10.0f), 13.0f,
				vanadium::Vector4(sinf(x * M_PI) * 0.5 + 0.5, cosf(y * M_PI) * 0.5 + 0.5, 0.0f, 1.0f));
			y += 30.0f;
		}
		x += 50.0f;
	}

	textShape = engine.uiSubsystem().addShape<vanadium::ui::shapes::TextShape>(
		vanadium::Vector2(300, 300), 1, 640.0f, 0.0f, "spin f  a  s  t", 48.0f, 0,
		vanadium::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	engine.uiSubsystem().addShape<vanadium::ui::shapes::TextShape>(
		vanadium::Vector2(400, 300), 2, 640.0f, 0.0f, "bingus", 48.0f, 0,
		vanadium::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
	engine.uiSubsystem().addShape<vanadium::ui::shapes::TextShape>(
		vanadium::Vector2(350, 300), 2, 640.0f, 0.0f, "aaaaaaaa aaaa", 48.0f, 0,
		vanadium::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
}

bool onFrame(vanadium::Engine& engine) {
	float x = 0.0f;
	for (uint32_t i = 0; i < 25; ++i) {
		float y = 0.0f;
		for (uint32_t j = 0; j < 25; ++j) {
			/*shapes[i * 25 + j]->setPosition(
				vanadium::Vector3(x + sinf(engine.elapsedTime() + x / (x + y)) * 50,
								  y + sinf(engine.elapsedTime() + (y + x) / y + M_PI / 2.0f) * 50, 0.99f));*/
			shapes[i * 25 + j]->setRotation(shapes[i * 25 + j]->rotation() + 6.0f * engine.deltaTime());
			y += 10.0f;
		}
		x += 30.0f;
	}
	//textShape->setRotation(textShape->rotation() + 38.0f * engine.deltaTime());
	return true;
}

void destroy(vanadium::Engine& engine) {}
