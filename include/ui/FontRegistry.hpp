#include <ft2build.h>
#include <hb.h>
#include <hb-ft.h>
#include <string_view>

namespace vanadium::ui
{
    class FontRegistry
    {
    public:
        FontRegistry(const std::string_view& registryFileName);
        ~FontRegistry();
    private:
        FT_Library m_library;
    };
    
} // namespace vanadium::ui
