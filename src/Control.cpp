#include <ui/Control.hpp>
#include <ui/UISubsystem.hpp>

namespace vanadium::ui {
	Control::Control(ControlStyleLayoutParameters&& parameters)
		: m_parent(parameters.parent), m_subsystem(parameters.subsystem), m_shapes(parameters.shapes),
		  m_functionalityLocalData(parameters.functionalityLocalData), m_position(parameters.requestedPosition),
		  m_size(parameters.requestedSize), m_repositionPFN(parameters.repositionPFN),
		  m_hoverStartPFN(parameters.hoverStartPFN), m_hoverEndPFN(parameters.hoverEndPFN),
		  m_regeneratePFN(parameters.regeneratePFN), m_mouseButtonHandlerPFN(parameters.mouseButtonHandlerPFN),
		  m_mouseHoverHandlerPFN(parameters.mouseHoverHandlerPFN), m_keyInputHandlerPFN(parameters.keyInputHandlerPFN),
		  m_charInputHandlerPFN(parameters.charInputHandlerPFN) {
		if (m_parent)
			m_parent->addChild(this);
	}

	Control::~Control() {
		for (auto& shape : m_shapes) {
			if (shape) {
				m_subsystem->removeShape(shape);
			}
		}
		delete m_functionalityLocalData;
	}

	void Control::invokeMouseButtonHandler(UISubsystem* subsystem, const Vector2& absolutePosition, uint32_t buttonID) {
		bool hitChild = false;
		for (auto& child : m_children) {
			if (boxTest(absolutePosition, child->position().absoluteTopLeft(topLeftPosition(), m_size, child->size()),
						child->size())) {
				child->invokeMouseButtonHandler(subsystem, absolutePosition, buttonID);
				hitChild = true;
			}
		}
		if (!hitChild) {
			m_mouseButtonHandlerPFN(subsystem, this, m_functionalityLocalData, absolutePosition, buttonID);
		}
	}

	void Control::invokeHoverHandler(UISubsystem* subsystem, const Vector2& absolutePosition) {
		bool hitChild = false;
		for (auto& child : m_children) {
			if (boxTest(absolutePosition, child->position().absoluteTopLeft(topLeftPosition(), m_size, child->size()),
						child->size())) {
				child->invokeHoverHandler(subsystem, absolutePosition);
				hitChild = true;
			}
		}
		if (!hitChild) {
			m_mouseHoverHandlerPFN(subsystem, this, m_functionalityLocalData, absolutePosition);
		}
	}

	void Control::invokeKeyInputHandler(UISubsystem* subsystem, uint32_t keyID,
										windowing::KeyModifierFlags modifierFlags,
										windowing::KeyState keyState) {
		m_keyInputHandlerPFN(subsystem, this, m_functionalityLocalData, keyID, modifierFlags, keyState);
	}

	void Control::invokeCharInputHandler(UISubsystem* subsystem, uint32_t unicodeCodepoint) {
		m_charInputHandlerPFN(subsystem, this, m_functionalityLocalData, unicodeCodepoint);
	}

	Vector2 Control::topLeftPosition() const {
		Vector2 parentTopLeftPosition = m_parent != nullptr ? m_parent->topLeftPosition() : Vector2(0.0f);
		Vector2 parentSize = m_parent != nullptr ? m_parent->size() : Vector2(1.0f);
		return m_position.absoluteTopLeft(parentTopLeftPosition, parentSize, m_size);
	}

	uint32_t Control::treeLevel() const {
		uint32_t level = 0U;
		seekTreeLevel(level);
		return level;
	}

	void Control::setPosition(const ControlPosition& position) {
		m_position = position;
		reposition();
	}

	void Control::setSize(const Vector2& size) {
		m_size = size;
		reposition();
	}

	void Control::reposition() {
		m_repositionPFN(m_subsystem, treeLevel(), topLeftPosition(), m_size, m_shapes);
		for (auto& child : m_children) {
			child->reposition();
		}
	}
} // namespace vanadium::ui