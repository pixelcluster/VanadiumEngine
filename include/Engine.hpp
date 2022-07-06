#pragma once
#include <cstdint>
#include <optional>
#include <math/Vector.hpp>
#include <timer/TimerManager.hpp>
#include <windowing/WindowSettingsOverride.hpp>

namespace vanadium {
	enum class EngineStartupFlag {};

	namespace graphics {
		class GraphicsSubsystem;
	}
	namespace ui {
		class UISubsystem;
	}
	namespace windowing {
		class WindowInterface;
	}

	class EngineConfig {
	  public:
		uint32_t startupFlags() const { return m_startupFlags; }
		std::string_view appName() const { return m_appName; }
		std::string_view pipelineLibraryFileName() const { return m_pipelineLibraryFileName; }
		std::string_view fontLibraryFileName() const { return m_fontLibraryFileName; }
		uint32_t appVersion() const { return m_appVersion; }
		const std::optional<windowing::WindowingSettingOverride>& settingsOverrides() const {
			return m_windowingSettingOverride;
		}
		void* userPointer() const { return m_userPointer; }
		const Vector4& uiBackgroundColor() const { return m_uiBackgroundColor; }

		void setAppName(const std::string_view& name) { m_appName = name; }
		void setAppVersion(uint32_t version) { m_appVersion = version; }
		void addStartupFlag(EngineStartupFlag flag) { m_startupFlags |= static_cast<uint32_t>(flag); }
		void overrideWindowSettings(const windowing::WindowingSettingOverride& override) {
			m_windowingSettingOverride = override;
		}
		void setPipelineLibraryFileName(const std::string_view& name) { m_pipelineLibraryFileName = name; }
		void setFontLibraryFileName(const std::string_view& name) { m_fontLibraryFileName = name; }
		void setUserPointer(void* userPointer) { m_userPointer = userPointer; }
		void setUIBackgroundColor(const Vector4& uiBackgroundColor) { m_uiBackgroundColor = uiBackgroundColor; }

	  private:
		uint32_t m_startupFlags = 0;
		std::string_view m_appName;
		std::string_view m_pipelineLibraryFileName = "./shaders.vcp";
		std::string_view m_fontLibraryFileName = "./fonts.fcfg";
		uint32_t m_appVersion;
		Vector4 m_uiBackgroundColor = Vector4(0.0f);

		std::optional<windowing::WindowingSettingOverride> m_windowingSettingOverride;
		void* m_userPointer;
	};

	class Engine {
	  public:
		Engine(const EngineConfig& config);
		~Engine();

		bool tickFrame();

		float deltaTime() const;
		float elapsedTime() const;

		graphics::GraphicsSubsystem& graphicsSubsystem() { return *m_graphicsSubsystem; }
		windowing::WindowInterface& windowInterface() { return *m_windowInterface; }
		ui::UISubsystem& uiSubsystem() { return *m_uiSubsystem; }
		timers::TimerManager& timerManager() { return m_timerManager; }

		void* userPointer() const { return m_userPointer; }
		void setUserPointer(void* userPointer) { m_userPointer = userPointer; }

	  private:
		uint32_t m_startupFlags;
		bool m_lastRenderSuccessful = true;
		void* m_userPointer;

		windowing::WindowInterface* m_windowInterface;
		graphics::GraphicsSubsystem* m_graphicsSubsystem;
		ui::UISubsystem* m_uiSubsystem;
		timers::TimerManager m_timerManager;
	};
} // namespace vanadium