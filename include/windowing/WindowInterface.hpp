#pragma once
// needed because this header is included as part of PCH, and other parts of this project need GLFW with Vulkan
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

	using KeyStateFlags = uint8_t;

	enum class KeyState { Pressed = 2, Held = 4, Released = 1 };

	inline KeyStateFlags operator|(const KeyState& one, const KeyState& other) {
		return static_cast<KeyStateFlags>(one) | static_cast<KeyStateFlags>(other);
	}

	inline KeyStateFlags operator&(const KeyState& one, const KeyState& other) {
		return static_cast<KeyStateFlags>(one) & static_cast<KeyStateFlags>(other);
	}

	inline KeyStateFlags operator^(const KeyState& one, const KeyState& other) {
		return static_cast<KeyStateFlags>(one) ^ static_cast<KeyStateFlags>(other);
	}

	inline KeyStateFlags operator|(const KeyStateFlags& one, const KeyState& other) {
		return one | static_cast<KeyStateFlags>(other);
	}

	inline KeyStateFlags operator&(const KeyStateFlags& one, const KeyState& other) {
		return one & static_cast<KeyStateFlags>(other);
	}

	inline KeyStateFlags operator^(const KeyStateFlags& one, const KeyState& other) {
		return one ^ static_cast<KeyStateFlags>(other);
	}

	inline KeyStateFlags operator|(const KeyState& one, const KeyStateFlags& other) {
		return static_cast<KeyStateFlags>(one) | other;
	}

	inline KeyStateFlags operator&(const KeyState& one, const KeyStateFlags& other) {
		return static_cast<KeyStateFlags>(one) & other;
	}

	inline KeyStateFlags operator^(const KeyState& one, const KeyStateFlags& other) {
		return static_cast<KeyStateFlags>(one) ^ other;
	}

	using KeyModifierFlags = uint8_t;

	enum class KeyModifier {
		Shift = GLFW_MOD_SHIFT,
		Ctrl = GLFW_MOD_CONTROL,
		Alt = GLFW_MOD_ALT,
		Super = GLFW_MOD_SUPER,
		CapsLock = GLFW_MOD_CAPS_LOCK,
		NumLock = GLFW_MOD_NUM_LOCK
	};

	inline KeyModifierFlags operator|(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<KeyModifierFlags>(one) | static_cast<KeyModifierFlags>(other);
	}

	inline KeyModifierFlags operator&(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<KeyModifierFlags>(one) & static_cast<KeyModifierFlags>(other);
	}

	inline KeyModifierFlags operator^(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<KeyModifierFlags>(one) ^ static_cast<KeyModifierFlags>(other);
	}

	inline KeyModifierFlags operator|(const KeyModifierFlags& one, const KeyModifier& other) {
		return one | static_cast<KeyModifierFlags>(other);
	}

	inline KeyModifierFlags operator&(const KeyModifierFlags& one, const KeyModifier& other) {
		return one & static_cast<KeyModifierFlags>(other);
	}

	inline KeyModifierFlags operator^(const KeyModifierFlags& one, const KeyModifier& other) {
		return one ^ static_cast<KeyModifierFlags>(other);
	}

	inline KeyModifierFlags operator|(const KeyModifier& one, const KeyModifierFlags& other) {
		return static_cast<KeyModifierFlags>(one) | other;
	}

	inline KeyModifierFlags operator&(const KeyModifier& one, const KeyModifierFlags& other) {
		return static_cast<KeyModifierFlags>(one) & other;
	}

	inline KeyModifierFlags operator^(const KeyModifier& one, const KeyModifierFlags& other) {
		return static_cast<KeyModifierFlags>(one) ^ other;
	}

	using ListenerDestroyCallback = void (*)(void* userData);

	using KeyEventCallback = void (*)(uint32_t keyCode, uint32_t modifiers, KeyState state, void* userData);
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
		uint32_t modifierMask;
		KeyStateFlags keyStateMask;

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
			return hashCombine(hash<uint32_t>()(data.keyCode), hash<uint32_t>()(data.modifierMask),
							   hash<vanadium::windowing::KeyStateFlags>()(data.keyStateMask));
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

		void addKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
							const KeyListenerParams& params);
		void removeKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
							   const KeyListenerParams& params);

		void addMouseKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
								 const KeyListenerParams& params);
		void removeMouseKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
									const KeyListenerParams& params);

		void addCharacterListener(const CharacterListenerParams& params);
		void removeCharacterListener(const CharacterListenerParams& params);

		void addSizeListener(const SizeListenerParams& params);
		void removeSizeListener(const SizeListenerParams& params);

		void addMouseMoveListener(const MouseListenerParams& params);
		void removeMouseMoveListener(const MouseListenerParams& params);

		void addScrollListener(const MouseListenerParams& params);
		void removeScrollListener(const MouseListenerParams& params);

		void invokeKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state);
		void invokeMouseKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state);
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