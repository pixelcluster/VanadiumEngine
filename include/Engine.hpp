#pragma once
#include <cstdint>
#include <graphics/GraphicsSubsystem.hpp>
#include <optional>
#include <windowing/WindowInterface.hpp>

namespace vanadium {
	enum class EngineStartupFlag { Disable3D = 1 };

	class EngineConfig {
	  public:
		uint32_t startupFlags() const { return m_startupFlags; }
		std::string_view appName() const { return m_appName; }
		uint32_t appVersion() const { return m_appVersion; }
		const std::optional<windowing::WindowingSettingOverride>& settingsOverrides() const {
			return m_windowingSettingOverride;
		}

		void setAppName(const std::string_view& name) { m_appName = name; }
		void setAppVersion(uint32_t version) { m_appVersion = version; }
		void addStartupFlag(EngineStartupFlag flag) { m_startupFlags |= static_cast<uint32_t>(flag); }
		void overrideWindowSettings(const windowing::WindowingSettingOverride& override) {
			m_windowingSettingOverride = override;
		}

	  private:
		uint32_t m_startupFlags;
		std::string_view m_appName;
		uint32_t m_appVersion;

		std::optional<windowing::WindowingSettingOverride> m_windowingSettingOverride;
	};

	class Engine {
	  public:
		Engine(const EngineConfig& config);

        void tickFrame();

	  private:
		uint32_t m_startupFlags;
        bool m_lastRenderSuccessful;

		windowing::WindowInterface m_windowInterface;
		graphics::GraphicsSubsystem m_graphicsSubsystem;
	};
} // namespace vanadium