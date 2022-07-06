#pragma once

#include <cstdint>

namespace vanadium::windowing {
	struct WindowingSettingOverride {
		uint32_t width;
		uint32_t height;
		bool createFullScreen;
	};
} // namespace vanadium::windowing