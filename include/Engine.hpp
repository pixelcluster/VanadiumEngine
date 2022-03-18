#pragma once
#include <cstdint>
#include <graphics/GraphicsSubsystem.hpp>

namespace vanadium {
    enum class EngineStartupFlags {
        Disable3D = 1
    };

    class Engine
    {
    public:
        Engine(uint32_t engineStartupFlags);
    private:
        uint32_t m_startupFlags;

        windowing::WindowInterface m_windowInterface;
        graphics::GraphicsSubsystem m_graphicsSubsystem;
    };
}