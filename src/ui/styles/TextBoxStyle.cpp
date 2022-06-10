#include <ui/styles/TextBoxStyle.hpp>

namespace vanadium::ui::styles {
	void RoundedTextBoxStyle::createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
										   const Vector2& size) {
		Vector2 foregroundOrigin = position + Vector2(1.0f);
		m_contentOrigin = foregroundOrigin + Vector2(0.1) * Vector2(std::min(size.x, size.y));
		Vector2 contentSize = size - Vector2(2.0f) * (Vector2(0.1) * Vector2(std::min(size.x, size.y)) + Vector2(1.0f));
		m_cursorShape = subsystem->addShape<shapes::RectShape>(m_contentOrigin, layerID + 2,
															   Vector2(0.1f, contentSize.y), 0.0f, Vector4(0.0f));
		m_textShape = subsystem->addShape<shapes::TextShape>(m_contentOrigin, layerID + 2, size.x, 0.0f, "",
															 contentSize.y / subsystem->monitorDPIY() * 72.0f, m_fontID,
															 Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		m_backgroundShape = subsystem->addShape<shapes::FilledRoundedRectShape>(
			foregroundOrigin, layerID + 1, size - Vector2(2.0f) * Vector2(1.0f), 0.0f, m_edgeSize, Vector4(1.0f));
		m_outlineShape = subsystem->addShape<shapes::FilledRoundedRectShape>(position, layerID, size, 0.0f, m_edgeSize,
																			 Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	void RoundedTextBoxStyle::repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
											   const Vector2& size) {
		Vector2 foregroundOrigin = position + Vector2(1.0f);
		m_contentOrigin = foregroundOrigin + Vector2(0.1) * Vector2(std::min(size.x, size.y));
		Vector2 contentSize = size - Vector2(2.0f) * (Vector2(0.1) * Vector2(std::min(size.x, size.y)) + Vector2(1.0f));

		m_cursorShape->setPosition(m_contentOrigin + m_currentCursorOffset);
		m_cursorShape->setSize(Vector2(1.0f, contentSize.y));
		m_cursorShape->setLayerIndex(layerID + 2);
		m_textShape->setPosition(m_contentOrigin);
		m_textShape->setMaxWidth(contentSize.x);
		m_textShape->setPointSize(contentSize.y / subsystem->monitorDPIY() * 72.0f);
		m_textShape->setLayerIndex(layerID + 2);
		m_backgroundShape->setPosition(foregroundOrigin);
		m_backgroundShape->setSize(contentSize);
		m_backgroundShape->setLayerIndex(layerID + 1);
		m_outlineShape->setPosition(position);
		m_outlineShape->setSize(size);
		m_outlineShape->setLayerIndex(layerID + 3);
	}

	void TextBoxStyle::incrementCursorGlyphIndex() {
		++m_cursorGlyphIndex;
		recalculateCursorOffset();
	}

	void TextBoxStyle::decrementCursorGlyphIndex() {
		if (m_cursorGlyphIndex > 0)
			--m_cursorGlyphIndex;
		recalculateCursorOffset();
	}

	void TextBoxStyle::eraseLetterBefore() {
		if (m_cursorGlyphIndex > 0) {
			m_textShape->deleteClusters(--m_cursorGlyphIndex);
			recalculateCursorOffset();
		}
	}
	void TextBoxStyle::eraseLetterAfter() {
		if (m_cursorGlyphIndex < m_textShape->glyphCount()) {
			m_textShape->deleteClusters(m_cursorGlyphIndex);
			recalculateCursorOffset();
		}
	}

	void TextBoxStyle::toggleCursor() {
		Vector4 color = m_cursorShape->color();
		m_cursorShape->setColor(Vector4(color.r, color.g, color.b, 1.0f - color.a));
	}

	void TextBoxStyle::recalculateCursorOffset() {
		if (!m_textShape->glyphCount()) {
			m_currentCursorOffset = Vector2(0.0f);
		} else if (m_cursorGlyphIndex < m_textShape->glyphCount()) {
			m_currentCursorOffset = m_textShape->glyphPosition(m_cursorGlyphIndex);
		} else {
			m_currentCursorOffset = m_textShape->glyphPosition(m_textShape->glyphCount() - 1) +
									Vector2(m_textShape->glyphWidth(m_textShape->glyphCount() - 1), 0.0f);
		}
		m_cursorShape->setPosition(m_contentOrigin + m_currentCursorOffset);
	}

} // namespace vanadium::ui::styles
