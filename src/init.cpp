#include <Engine.hpp>

extern void configureEngine(vanadium::EngineConfig& config);

extern void preFramegraphInit(vanadium::Engine& engine);
extern void init(vanadium::Engine& engine);

extern bool onFrame(vanadium::Engine& engine);

extern void destroy(vanadium::Engine& engine);

int main() {
    vanadium::EngineConfig config;
    configureEngine(config);
    vanadium::Engine engine = vanadium::Engine(config);
    preFramegraphInit(engine);
    engine.initFramegraph();
    init(engine);
    while(onFrame(engine) && engine.tickFrame()) {}
    destroy(engine);
}