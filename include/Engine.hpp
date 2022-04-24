#pragma once
#include <cstdint>
#include <graphics/GraphicsSubsystem.hpp>
#include <optional>
#include <ui/UISubsystem.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium {
	enum class EngineStartupFlag { UIOnly = 1 };

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
		const std::vector<graphics::FramegraphNode*>& customFramegraphNodes() const { return m_customFramegraphNodes; }
		void* userPointer() const { return m_userPointer; }

		void setAppName(const std::string_view& name) { m_appName = name; }
		void setAppVersion(uint32_t version) { m_appVersion = version; }
		void addStartupFlag(EngineStartupFlag flag) { m_startupFlags |= static_cast<uint32_t>(flag); }
		void overrideWindowSettings(const windowing::WindowingSettingOverride& override) {
			m_windowingSettingOverride = override;
		}
		void setPipelineLibraryFileName(const std::string_view& name) { m_pipelineLibraryFileName = name; }
		void setFontLibraryFileName(const std::string_view& name) { m_fontLibraryFileName = name; }
		void setUserPointer(void* userPointer) { m_userPointer = userPointer; }

		template <std::derived_from<graphics::FramegraphNode> T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* addCustomFramegraphNode(Args&&... constructorArgs) {
			T* node = new T(constructorArgs...);
			m_customFramegraphNodes.push_back(node);
			return node;
		}

	  private:
		uint32_t m_startupFlags = 0;
		std::string_view m_appName;
		std::string_view m_pipelineLibraryFileName = "./shaders.vcp";
		std::string_view m_fontLibraryFileName = "./fonts.fcfg";
		uint32_t m_appVersion;

		std::optional<windowing::WindowingSettingOverride> m_windowingSettingOverride;
		std::vector<graphics::FramegraphNode*> m_customFramegraphNodes;
		void* m_userPointer;
	};

	class Engine {
	  public:
		Engine(const EngineConfig& config);

		void initFramegraph();

		bool tickFrame();

		float deltaTime() const { return m_windowInterface.deltaTime(); }
		float elapsedTime() const { return m_windowInterface.elapsedTime(); }

		graphics::GraphicsSubsystem& graphicsSubsystem() { return m_graphicsSubsystem; }
		windowing::WindowInterface& windowInterface() { return m_windowInterface; }
		ui::UISubsystem& uiSubsystem() { return m_uiSubsystem; }

		void* userPointer() const { return m_userPointer; }
		void setUserPointer(void* userPointer) { m_userPointer = userPointer; }

	  private:
		uint32_t m_startupFlags;
		bool m_lastRenderSuccessful = true;
		void* m_userPointer;

		windowing::WindowInterface m_windowInterface;
		graphics::GraphicsSubsystem m_graphicsSubsystem;
		ui::UISubsystem m_uiSubsystem;
	};
} // namespace vanadium