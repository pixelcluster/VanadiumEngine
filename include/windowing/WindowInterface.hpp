#include <GLFW/glfw3.h>
#include <robin_hood.h>
#include <vector>

namespace vanadium::windowing {

	using KeyStateFlags = uint8_t;

	enum class KeyState { Pressed = 2, Held = 4, Released = 1 };

	KeyStateFlags operator|(const KeyState& one, const KeyState& other) {
		return static_cast<KeyStateFlags>(one) | static_cast<KeyStateFlags>(other);
	}

	KeyStateFlags operator&(const KeyState& one, const KeyState& other) {
		return static_cast<KeyStateFlags>(one) & static_cast<KeyStateFlags>(other);
	}

	KeyStateFlags operator^(const KeyState& one, const KeyState& other) {
		return static_cast<KeyStateFlags>(one) ^ static_cast<KeyStateFlags>(other);
	}

	KeyStateFlags operator|(const KeyStateFlags& one, const KeyState& other) {
		return one | static_cast<KeyStateFlags>(other);
	}

	KeyStateFlags operator&(const KeyStateFlags& one, const KeyState& other) {
		return one & static_cast<KeyStateFlags>(other);
	}

	KeyStateFlags operator^(const KeyStateFlags& one, const KeyState& other) {
		return one ^ static_cast<KeyStateFlags>(other);
	}

	KeyStateFlags operator|(const KeyState& one, const KeyStateFlags& other) {
		return static_cast<KeyStateFlags>(one) | other;
	}

	KeyStateFlags operator&(const KeyState& one, const KeyStateFlags& other) {
		return static_cast<KeyStateFlags>(one) & other;
	}

	KeyStateFlags operator^(const KeyState& one, const KeyStateFlags& other) {
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

	KeyModifierFlags operator|(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<KeyModifierFlags>(one) | static_cast<KeyModifierFlags>(other);
	}

	KeyModifierFlags operator&(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<KeyModifierFlags>(one) & static_cast<KeyModifierFlags>(other);
	}

	KeyModifierFlags operator^(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<KeyModifierFlags>(one) ^ static_cast<KeyModifierFlags>(other);
	}

	KeyModifierFlags operator|(const KeyModifierFlags& one, const KeyModifier& other) {
		return one | static_cast<KeyModifierFlags>(other);
	}

	KeyModifierFlags operator&(const KeyModifierFlags& one, const KeyModifier& other) {
		return one & static_cast<KeyModifierFlags>(other);
	}

	KeyModifierFlags operator^(const KeyModifierFlags& one, const KeyModifier& other) {
		return one ^ static_cast<KeyModifierFlags>(other);
	}

	KeyModifierFlags operator|(const KeyModifier& one, const KeyModifierFlags& other) {
		return static_cast<KeyModifierFlags>(one) | other;
	}

	KeyModifierFlags operator&(const KeyModifier& one, const KeyModifierFlags& other) {
		return static_cast<KeyModifierFlags>(one) & other;
	}

	KeyModifierFlags operator^(const KeyModifier& one, const KeyModifierFlags& other) {
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
	};

	using SizeEventCallback = void (*)(uint32_t width, uint32_t height, void* userData);
	struct SizeListenerParams {
		SizeEventCallback eventCallback;
		ListenerDestroyCallback listenerDestroyCallback;
		void* userData;
	};

	void emptyListenerDestroyCallback(void*) {}

} // namespace vanadium::windowing

namespace robin_hood {
	template <> struct hash<vanadium::windowing::KeyListenerData> {
		size_t operator()(const vanadium::windowing::KeyListenerData& data) {
			return hash<uint32_t>()(data.keyCode) ^
				   hash<uint32_t>()(data.modifierMask) * hash<vanadium::windowing::KeyStateFlags>()(data.keyStateMask);
		}
	};
} // namespace robin_hood

namespace vanadium::windowing {
	class WindowInterface {
	  public:
		WindowInterface(bool createFullscreen, uint32_t width, uint32_t height, const char* name);
		// creates window with width/height of primary monitor
		WindowInterface(const char* name);
		WindowInterface(const WindowInterface&) = delete;
		WindowInterface& operator=(const WindowInterface&) = delete;
		WindowInterface(WindowInterface&&) = delete;
		WindowInterface& operator=(WindowInterface&&) = delete;
		~WindowInterface();

		void pollEvents();

		void addKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
							const KeyListenerParams& params);
		void removeKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask);

		void addSizeListener(const SizeListenerParams& params);
		void removeSizeListener(const SizeListenerParams& params);

		void invokeKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state);

		GLFWwindow* internalHandle() { return m_window; }

		void windowSize(uint32_t& width, uint32_t& height);

	  private:
		GLFWwindow* m_window;

		robin_hood::unordered_flat_map<KeyListenerData, KeyListenerParams> m_keyListeners;
		std::vector<SizeListenerParams> m_sizeListeners;

		static uint32_t glfwWindowCount;
	};
} // namespace vanadium::windowing