#pragma once
//needed because this header is included as part of PCH, and other parts of this project need GLFW with Vulkan
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <robin_hood.h>
#include <vector>

namespace vanadium::windowing {

	struct WindowingSettingOverride {
		uint32_t width;
		uint32_t height;
		bool createFullScreen;
	};

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

	inline void emptyListenerDestroyCallback(void*) {}

} // namespace vanadium::windowing

namespace robin_hood {
	template <> struct hash<vanadium::windowing::KeyListenerData> {
		size_t operator()(const vanadium::windowing::KeyListenerData& data) const {
			return hash<uint32_t>()(data.keyCode) ^
				   hash<uint32_t>()(data.modifierMask) * hash<vanadium::windowing::KeyStateFlags>()(data.keyStateMask);
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
		void removeKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask);

		void addSizeListener(const SizeListenerParams& params);
		void removeSizeListener(const SizeListenerParams& params);

		void invokeKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state);

		GLFWwindow* internalHandle() { return m_window; }

		void windowSize(uint32_t& width, uint32_t& height);

		bool shouldClose();

	  private:
		GLFWwindow* m_window;

		robin_hood::unordered_flat_map<KeyListenerData, KeyListenerParams> m_keyListeners;
		std::vector<SizeListenerParams> m_sizeListeners;

		static uint32_t m_glfwWindowCount;
	};
} // namespace vanadium::windowing