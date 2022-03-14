#include <cstdint>

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
    };
}