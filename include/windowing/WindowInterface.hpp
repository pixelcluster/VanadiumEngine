#include <GLFW/glfw3.h>
#include <robin_hood.h>

namespace vanadium::windowing {

	using KeyStateFlags = uint8_t;

	enum class KeyState { Pressed = 2, Held = 4, Released = 1 };

	KeyStateFlags operator|(const KeyState& one, const KeyState& other) {
		return static_cast<uint8_t>(one) | static_cast<uint8_t>(other);
	}

	KeyStateFlags operator&(const KeyState& one, const KeyState& other) {
		return static_cast<uint8_t>(one) & static_cast<uint8_t>(other);
	}

	KeyStateFlags operator^(const KeyState& one, const KeyState& other) {
		return static_cast<uint8_t>(one) ^ static_cast<uint8_t>(other);
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
		return static_cast<uint8_t>(one) | static_cast<uint8_t>(other);
	}

	KeyModifierFlags operator&(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<uint8_t>(one) & static_cast<uint8_t>(other);
	}

	KeyModifierFlags operator^(const KeyModifier& one, const KeyModifier& other) {
		return static_cast<uint8_t>(one) ^ static_cast<uint8_t>(other);
	}

	using KeyEventCallback = void (*)(uint32_t keyCode, uint32_t modifierMask, KeyState state, void* userData);
	using KeyListenerDestroyCallback = void (*)(void* userData);
	struct KeyListenerParams {
		KeyEventCallback eventCallback;
		KeyListenerDestroyCallback listenerDestroyCallback;
		void* userData;
	};

	struct KeyListenerData {
		uint32_t keyCode;
		uint32_t modifierMask;
		KeyStateFlags keyStateMask;
	};


} // namespace vanadium::windowing

namespace robin_hood {
	template <> struct hash<vanadium::windowing::KeyListenerData> {
		size_t operator()(const vanadium::windowing::KeyListenerData& data) {
			return hash<uint32_t>()(data.keyCode) ^
				   hash<uint32_t>()(data.modifierMask) * hash<KeyStateFlags>()(data.keyStateMask);
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

		void addKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask, const KeyListenerParams& params);

	  private:
		GLFWwindow* m_window;

		robin_hood::unordered_flat_map<KeyListenerData, KeyListenerParams> m_keyListeners;

		static uint32_t glfwWindowCount;
	};
} // namespace vanadium::windowing