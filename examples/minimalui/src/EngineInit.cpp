#include <Engine.hpp>
#include <SwapchainClearNode.hpp>

void configureEngine(vanadium::EngineConfig& config){
    config.setAppName("Vanadium Minimal UI");
}

void init(vanadium::Engine& engine) {
    engine.addFramegraphPass<SwapchainClearNode>();
}

bool onFrame(vanadium::Engine& engine) {
    return true;
}