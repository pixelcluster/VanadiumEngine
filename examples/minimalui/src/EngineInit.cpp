#include <Engine.hpp>

void configureEngine(vanadium::EngineConfig& config){
    config.setAppName("Vanadium Minimal UI");
}

void init(vanadium::Engine& engine) {

}

bool onFrame(vanadium::Engine& engine) {
    return true;
}