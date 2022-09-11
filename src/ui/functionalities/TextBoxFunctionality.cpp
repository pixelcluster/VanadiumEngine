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
#include <ui/functionalities/TextBoxFunctionality.hpp>

namespace vanadium::ui::functionalities {
	void codepointToUTF8(uint32_t codepoint, char dst[4], uint32_t& numChars) {
		if (codepoint <= 0x7F) {
			dst[0] = static_cast<char>(codepoint);
			numChars = 1;
		} else {
			if (codepoint < 0x7FF)
				numChars = 2;
			else if (codepoint < 0xFFFF)
				numChars = 3;
			else if (codepoint < 0x10FFFF)
				numChars = 4;
			else {
				numChars = 0;
				return;
			}
			uint32_t currentBitpos = 0;
			for (uint32_t i = 0; i < numChars - 1; ++i) {
				dst[i] = (codepoint >> currentBitpos) & 0x3F;
				dst[i] |= 0x80;
				currentBitpos &= 6;
			}
			switch (numChars) {
				case 2:
					dst[numChars - 1] = (codepoint >> currentBitpos) & 0x1F;
					dst[numChars - 1] |= 0xC0;
					break;
				case 3:
					dst[numChars - 1] = (codepoint >> currentBitpos) & 0xF;
					dst[numChars - 1] |= 0xE0;
					break;
				case 4:
					dst[numChars - 1] = (codepoint >> currentBitpos) & 0x7;
					dst[numChars - 1] |= 0xF0;
					break;
				default:
					break;
			}
		}
	}

	void timerCallback(void* userData) {
		reinterpret_cast<styles::TextBoxStyle*>(reinterpret_cast<Control*>(userData)->style())->toggleCursor();
	}

	TextBoxFunctionality::~TextBoxFunctionality() {
		if (m_timer != ~0U)
			m_engine->timerManager().removeTimer(m_timer);
	}

	void TextBoxFunctionality::keyInputHandler(UISubsystem* subsystem, Control* triggeringControl, uint32_t keyID,
											   windowing::KeyModifier modifierFlags,
											   windowing::KeyState keyState) {
		styles::TextBoxStyle* style = reinterpret_cast<styles::TextBoxStyle*>(triggeringControl->style());
		switch (keyID)
		{
		case GLFW_KEY_BACKSPACE:
			style->eraseLetterBefore();
			break;
		case GLFW_KEY_DELETE:
			style->eraseLetterAfter();
			break;
		case GLFW_KEY_LEFT:
			style->decrementCursorGlyphIndex();
			break;
		case GLFW_KEY_RIGHT:
			style->incrementCursorGlyphIndex();
			break;
		default:
			break;
		}
	}

	void TextBoxFunctionality::charInputHandler(UISubsystem* subsystem, Control* triggeringControl,
												uint32_t codepoint) {
		styles::TextBoxStyle* style = reinterpret_cast<styles::TextBoxStyle*>(triggeringControl->style());
		std::string text = std::string(style->inputTextShape()->text());
		char newText[4];
		uint32_t charCount = 0;
		codepointToUTF8(codepoint, newText, charCount);
		for (uint32_t i = 0; i < charCount; ++i)
			text.push_back(newText[i]);
		style->inputTextShape()->setText(text);
		style->incrementCursorGlyphIndex();
		style->recalculateTextPosition();
	}

	KeyMask TextBoxFunctionality::keyInputMask() const {
		return { .stateMask = static_cast<windowing::KeyState>(windowing::KeyState::Pressed),
				 .modifierMask = static_cast<windowing::KeyModifier>(~0U) };
	}
	std::vector<uint32_t> TextBoxFunctionality::keyCodes() const { return { GLFW_KEY_BACKSPACE, GLFW_KEY_DELETE, GLFW_KEY_LEFT, GLFW_KEY_RIGHT }; }

	void TextBoxFunctionality::inputFocusGained(UISubsystem* subsystem, Control* triggeringControl) {
		m_timer = m_engine->timerManager().addTimer(0.5f, timerCallback, timers::emptyTimerDestroyCallback,
													triggeringControl);
		styles::TextBoxStyle* style = reinterpret_cast<styles::TextBoxStyle*>(triggeringControl->style());
		//cursor will be disabled, this enables it
		style->toggleCursor();
	}
	void TextBoxFunctionality::inputFocusLost(UISubsystem* subsystem, Control* triggeringControl) {
		m_engine->timerManager().removeTimer(m_timer);
		m_timer = ~0U;
		styles::TextBoxStyle* style = reinterpret_cast<styles::TextBoxStyle*>(triggeringControl->style());
		style->disableCursor();
	}

} // namespace vanadium::ui::functionalities
