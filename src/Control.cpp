#include <ui/Control.hpp>
#include <ui/UISubsystem.hpp>

namespace vanadium::ui {
	Control::Control(UISubsystem* subsystem, Control* parent, ControlPosition position, const Vector2& size,
					 Style* style, Layout* layout, Functionality* functionality)
		: m_parent(parent), m_subsystem(subsystem), m_position(position), m_size(size), m_style(style),
		  m_layout(layout), m_functionality(functionality) {
		if (m_parent) {
			m_parent->addChild(this);
		}
		m_subsystem->recalculateLayerIndices();
		m_style->createShapes(subsystem, m_layerID, topLeftPosition(), m_size);
	}

	Control::~Control() {
		delete m_style;
		delete m_layout;
		delete m_functionality;
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
			m_functionality->mouseButtonHandler(subsystem, this, absolutePosition, buttonID);
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
			m_functionality->mouseHoverHandler(subsystem, this, absolutePosition);
		}
	}

	void Control::invokeKeyInputHandler(UISubsystem* subsystem, uint32_t keyID,
										windowing::KeyModifierFlags modifierFlags, windowing::KeyState keyState) {
		m_functionality->keyInputHandler(subsystem, this, keyID, modifierFlags, keyState);
	}

	void Control::invokeCharInputHandler(UISubsystem* subsystem, uint32_t unicodeCodepoint) {
		m_functionality->charInputHandler(subsystem, this, unicodeCodepoint);
	}

	Vector2 Control::topLeftPosition() const {
		Vector2 parentTopLeftPosition = m_parent != nullptr ? m_parent->topLeftPosition() : Vector2(0.0f);
		Vector2 parentSize = m_parent != nullptr ? m_parent->size() : Vector2(1.0f);
		return m_position.absoluteTopLeft(parentTopLeftPosition, parentSize, m_size);
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
		m_style->repositionShapes(m_subsystem, m_layerID, topLeftPosition(), m_size);
		for (auto& child : m_children) {
			child->reposition();
		}
	}

	void Control::internalRecalculateLayerIndex(uint32_t& layerID) {
		m_layerID = layerID;
		layerID += m_style->layerCount();
		for (auto& child : m_children) {
			child->internalRecalculateLayerIndex(layerID);
		}
	}
} // namespace vanadium::ui