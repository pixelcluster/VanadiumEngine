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
#include <ui/Shape.hpp>
#include <ui/util/ControlPosition.hpp>
#include <vector>
#include <windowing/WindowInterface.hpp>
namespace vanadium::ui {

	class UISubsystem;
	class Control;

	class Style {
	  public:
		virtual ~Style() {}
		virtual void createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
								  const Vector2& size) {}
		virtual void repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
									  const Vector2& size) {}
		virtual void hoverStart(UISubsystem* subsystem) {}
		virtual void hoverEnd(UISubsystem* subsystem) {}

		virtual uint32_t layerCount() const { return 1; }
	};

	class Layout {
	  public:
		virtual ~Layout() {}
		virtual void regenerateLayout(const std::vector<Control*>& children, const Vector2& controlPosition,
									  const Vector2& controlSize) {}
	};

	struct KeyMask {
		windowing::KeyStateFlags stateMask;
		windowing::KeyModifierFlags modifierMask;
	};

	class Functionality {
	  public:
		virtual ~Functionality() {}
		virtual void mouseButtonHandler(UISubsystem* subsystem, Control* triggerControl,
										const Vector2& absolutePosition, uint32_t buttonID) {}
		virtual void mouseHoverHandler(UISubsystem* subsystem, Control* triggeringControl,
									   const Vector2& absolutePosition) {}
		virtual void keyInputHandler(UISubsystem* subsystem, Control* triggeringControl, uint32_t keyID,
									 windowing::KeyModifierFlags modifierFlags, windowing::KeyState keyState) {}
		virtual void charInputHandler(UISubsystem* subsystem, Control* triggeringControl, uint32_t codepoint) {}

		virtual KeyMask keyInputMask() const { return { 0, 0 }; }
		virtual std::vector<uint32_t> keyCodes() const { return {}; }

		virtual void inputFocusGained(UISubsystem* subsystem, Control* triggeringControl) {}
		virtual void inputFocusLost(UISubsystem* subsystem, Control* triggeringControl) {}
	};

	// to be specialized for each functionality and/or layout
	// must define constexpr static boolean member named "value" that defines if the layout and functionality are
	// compatible
	template <typename Functionality, typename Style> struct IsCompatible { static constexpr bool value = false; };

	template <typename Style> struct IsCompatible<Functionality, Style> { static constexpr bool value = true; };

	template <typename Functionality, typename Style>
	concept CompatibleWith = IsCompatible<Functionality, Layout>::value;

	template <std::derived_from<Style> T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* createStyle(Args&&... args) { return new T(args...); }

	template <std::derived_from<Layout> T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* createLayout(Args&&... args) { return new T(args...); }

	template <std::derived_from<Style> Style, std::derived_from<Functionality> T, typename... Args>
	requires(std::constructible_from<T, Args...>&& IsCompatible<T, Style>::value) T* createFunctionality(
		Args&&... args) {
		return new T(args...);
	}

	class Control {
	  public:
		Control(UISubsystem* subsystem, Control* parent, ControlPosition m_position, const Vector2& m_size,
				Style* style, Layout* layout, Functionality* functionality);
		Control(const Control&) = delete;
		Control& operator=(const Control&) = delete;
		Control(Control&&) = default;
		Control& operator=(Control&&) = default;
		~Control();

		void invokeMouseButtonHandler(UISubsystem* subsystem, const Vector2& absolutePosition, uint32_t buttonID);
		void invokeHoverHandler(UISubsystem* subsystem, const Vector2& absolutePosition);
		void invokeKeyInputHandler(UISubsystem* subsystem, uint32_t keyID, windowing::KeyModifierFlags modifierFlags,
								   windowing::KeyState keyStateFlags);
		void invokeCharInputHandler(UISubsystem* subsystem, uint32_t unicodeCodepoint);

		void releaseInputFocus(UISubsystem* subsystem) { m_functionality->inputFocusLost(subsystem, this); }

		Vector2 topLeftPosition() const;
		const ControlPosition& position() const { return m_position; }
		const Vector2& size() const { return m_size; }

		void setPosition(const ControlPosition& position);
		void setSize(const Vector2& size);

		uint32_t layerID() const { return m_layerID; }

		// This method must only be called for the root control
		void internalRecalculateLayerIndex(uint32_t& layerID);

		// TODO: Child lifetime handling is a mess (move memory management to parent or UI subsystem?)
		void addChild(Control* newChild) { m_children.push_back(newChild); }

		Style* style() { return m_style; }
		Layout* layout() { return m_layout; }
		Functionality* functionality() { return m_functionality; }

	  private:
		void reposition();

		Control* m_parent;
		std::vector<Control*> m_children;
		uint32_t m_layerID;

		UISubsystem* m_subsystem;

		ControlPosition m_position;
		Vector2 m_size;

		Style* m_style;
		Layout* m_layout;
		Functionality* m_functionality;

		bool boxTest(const Vector2& position, const Vector2& boxOrigin, const Vector2& boxExtent) {
			return position.x >= boxOrigin.x && position.x <= (boxOrigin.x + boxExtent.x) &&
				   position.y >= boxOrigin.y && position.y <= (boxOrigin.y + boxExtent.y);
		}
	};
} // namespace vanadium::ui
