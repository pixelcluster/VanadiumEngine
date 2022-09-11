/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
// needed because this header is included as part of PCH, and other parts of this project need GLFW with Vulkan
#include "graphics/helper/EnumClassFlags.hpp"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <optional>
#include <robin_hood.h>
#include <vector>

#include <math/Vector.hpp>
#include <windowing/WindowSettingsOverride.hpp>
#include <util/HashCombine.hpp>

namespace vanadium::windowing {

	constexpr uint32_t platformDefaultDPI = 72;

	#undef None

	enum class KeyState { Pressed = 2, Held = 4, Released = 1, None = 0 };
	
	DEFINE_FLAGS(KeyState)

	enum class KeyModifier {
		None = 0,
		Shift = GLFW_MOD_SHIFT,
		Ctrl = GLFW_MOD_CONTROL,
		Alt = GLFW_MOD_ALT,
		Super = GLFW_MOD_SUPER,
		CapsLock = GLFW_MOD_CAPS_LOCK,
		NumLock = GLFW_MOD_NUM_LOCK
	};

	DEFINE_FLAGS(KeyModifier)

	using ListenerDestroyCallback = void (*)(void* userData);

	using KeyEventCallback = void (*)(uint32_t keyCode, KeyModifier modifiers, KeyState state, void* userData);
	struct KeyListenerParams {
		KeyEventCallback eventCallback;
		ListenerDestroyCallback listenerDestroyCallback;
		void* userData;

		bool operator==(const KeyListenerParams& other) const {
			return eventCallback == other.eventCallback && listenerDestroyCallback == other.listenerDestroyCallback &&
				   userData == other.userData;
		}
	};


	using CharacterEventCallback = void (*)(uint32_t keyCode, void* userData);
	struct CharacterListenerParams {
		CharacterEventCallback eventCallback;
		ListenerDestroyCallback listenerDestroyCallback;
		void* userData;

		bool operator==(const CharacterListenerParams& other) const {
			return eventCallback == other.eventCallback && listenerDestroyCallback == other.listenerDestroyCallback &&
				   userData == other.userData;
		}
	};

	struct KeyListenerData {
		uint32_t keyCode;
		KeyModifier modifierMask;
		KeyState keyStateMask;

		bool operator==(const KeyListenerData& other) const {
			return keyCode == other.keyCode && modifierMask == other.modifierMask && keyStateMask == other.keyStateMask;
		}
	};

	using SizeEventCallback = void (*)(uint32_t width, uint32_t height, void* userData);
	struct SizeListenerParams {
		SizeEventCallback eventCallback;
		ListenerDestroyCallback listenerDestroyCallback;
		void* userData;

		bool operator==(const SizeListenerParams& other) const {
			return eventCallback == other.eventCallback && listenerDestroyCallback == other.listenerDestroyCallback &&
				   userData == other.userData;
		}
	};

	using MouseEventCallback = void (*)(const Vector2& position, void* userData);
	struct MouseListenerParams {
		MouseEventCallback eventCallback;
		ListenerDestroyCallback listenerDestroyCallback;
		void* userData;

		bool operator==(const MouseListenerParams& other) const {
			return eventCallback == other.eventCallback && listenerDestroyCallback == other.listenerDestroyCallback &&
				   userData == other.userData;
		}
	};

	inline void emptyListenerDestroyCallback(void*) {}

} // namespace vanadium::windowing

namespace robin_hood {
	template <> struct hash<vanadium::windowing::KeyListenerData> {
		size_t operator()(const vanadium::windowing::KeyListenerData& data) const {
			return hashCombine(hash<uint32_t>()(data.keyCode), hash<vanadium::windowing::KeyModifier>()(data.modifierMask),
							   hash<vanadium::windowing::KeyState>()(data.keyStateMask));
		}
	};
} // namespace robin_hood

namespace vanadium::windowing {
	class WindowInterface {
	  public:
		WindowInterface(const std::optional<WindowingSettingOverride>& override, const char* name);
		// creates window with width/height of primary monitor
		WindowInterface(const char* name);
		WindowInterface(const WindowInterface&) = delete;
		WindowInterface& operator=(const WindowInterface&) = delete;
		WindowInterface(WindowInterface&&) = delete;
		WindowInterface& operator=(WindowInterface&&) = delete;
		~WindowInterface();

		void pollEvents();
		void waitEvents();

		void addKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
							const KeyListenerParams& params);
		void removeKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
							   const KeyListenerParams& params);

		void addMouseKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
								 const KeyListenerParams& params);
		void removeMouseKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
									const KeyListenerParams& params);

		void addCharacterListener(const CharacterListenerParams& params);
		void removeCharacterListener(const CharacterListenerParams& params);

		void addSizeListener(const SizeListenerParams& params);
		void removeSizeListener(const SizeListenerParams& params);

		void addMouseMoveListener(const MouseListenerParams& params);
		void removeMouseMoveListener(const MouseListenerParams& params);

		void addScrollListener(const MouseListenerParams& params);
		void removeScrollListener(const MouseListenerParams& params);

		void invokeKeyListeners(uint32_t keyCode, KeyModifier modifiers, KeyState state);
		void invokeMouseKeyListeners(uint32_t keyCode, KeyModifier modifiers, KeyState state);
		void invokeCharacterListeners(uint32_t keyCode);
		void invokeSizeListeners(uint32_t newWidth, uint32_t newHeight);
		void invokeMouseMoveListeners(double x, double y);
		void invokeScrollListeners(double x, double y);

		Vector2 mousePos() const;

		GLFWwindow* internalHandle() { return m_window; }

		void windowSize(uint32_t& width, uint32_t& height);

		bool shouldClose();

		float deltaTime() const { return m_deltaTime; }
		float elapsedTime() const { return m_elapsedTime; }

		uint32_t contentScaleDPIX() const { return m_contentScaleDPIX; }
		uint32_t contentScaleDPIY() const { return m_contentScaleDPIY; }

	  private:
		GLFWwindow* m_window;

		float m_deltaTime;
		float m_elapsedTime;

		uint32_t m_contentScaleDPIX;
		uint32_t m_contentScaleDPIY;

		robin_hood::unordered_map<KeyListenerData, std::vector<KeyListenerParams>> m_keyListeners;
		robin_hood::unordered_map<KeyListenerData, std::vector<KeyListenerParams>> m_mouseKeyListeners;
		std::vector<CharacterListenerParams> m_characterListeners;
		std::vector<SizeListenerParams> m_sizeListeners;
		std::vector<MouseListenerParams> m_mouseMoveListeners;
		std::vector<MouseListenerParams> m_scrollListeners;

		static uint32_t m_glfwWindowCount;
	};
} // namespace vanadium::windowing
