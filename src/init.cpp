#include <Engine.hpp>

extern void configureEngine(vanadium::EngineConfig& config);
extern void init(vanadium::Engine& engine);

extern bool onFrame(vanadium::Engine& engine);

int main() {
    vanadium::EngineConfig config;
    configureEngine(config);
    vanadium::Engine engine = vanadium::Engine(config);
    init(engine);
    engine.afterUserInit();
    while(onFrame(engine) && engine.tickFrame()) {}
}