#include <ui/styles/TextBoxStyle.hpp>

namespace vanadium::ui::styles {
	void RoundedTextBoxStyle::createShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
										   const Vector2& size) {
		Vector2 foregroundOrigin = position + Vector2(1.0f);
		m_contentOrigin = foregroundOrigin + Vector2(0.1) * Vector2(std::min(size.x, size.y));
		m_contentSize = size - Vector2(2.0f) * (Vector2(0.1) * Vector2(std::min(size.x, size.y)) + Vector2(1.0f));

		m_cursorShape = subsystem->addShape<shapes::RectShape>(m_contentOrigin, layerID + 2,
															   Vector2(0.1f, m_contentSize.y), 0.0f, Vector4(0.0f));
		m_textShape = subsystem->addShape<shapes::TextShape>(m_contentOrigin, layerID + 2, -1.0f, 0.0f, "",
															 m_contentSize.y / subsystem->monitorDPIY() * 72.0f,
															 m_fontID, Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		m_backgroundShape = subsystem->addShape<shapes::FilledRoundedRectShape>(
			foregroundOrigin, layerID + 1, size - Vector2(2.0f) * Vector2(1.0f), 0.0f, m_edgeSize, Vector4(1.0f));
		m_outlineShape = subsystem->addShape<shapes::FilledRoundedRectShape>(position, layerID, size, 0.0f, m_edgeSize,
																			 Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		m_layerIndex = layerID;
	}

	void RoundedTextBoxStyle::repositionShapes(UISubsystem* subsystem, uint32_t layerID, const Vector2& position,
											   const Vector2& size) {
		Vector2 foregroundOrigin = position + Vector2(1.0f);
		m_contentOrigin = foregroundOrigin + Vector2(0.1) * Vector2(std::min(size.x, size.y));
		Vector2 foregroundSize = size - Vector2(2.0f) * (Vector2(1.0f));
		m_contentSize = size - Vector2(2.0f) * (Vector2(0.1) * Vector2(std::min(size.x, size.y)) + Vector2(1.0f));
		Vector2 fontBaselineOrigin = Vector2(m_contentOrigin.x + m_contentSize.x - m_textShape->size().x,
											 m_contentOrigin.y + 0.8 * m_contentSize.y);

		if (m_textShape->size().x < m_contentSize.x) {
			fontBaselineOrigin.x = m_contentOrigin.x;
		}

		m_cursorShape->setPosition(m_contentOrigin + m_currentCursorOffset);
		m_cursorShape->setSize(Vector2(1.0f, m_contentSize.y));
		m_cursorShape->setLayerIndex(layerID + 2);
		m_textShape->setPosition(fontBaselineOrigin - Vector2(0.0f, m_textShape->baselineOffset()));
		m_textShape->setPointSize(m_contentSize.y / subsystem->monitorDPIY() * 72.0f);
		m_textShape->setLayerIndex(layerID + 2);
		m_backgroundShape->setPosition(foregroundOrigin);
		m_backgroundShape->setSize(foregroundSize);
		m_backgroundShape->setLayerIndex(layerID + 1);
		m_outlineShape->setPosition(position);
		m_outlineShape->setSize(size);
		m_outlineShape->setLayerIndex(layerID);
		subsystem->setLayerScissor(m_layerIndex + 2, {});
		m_layerIndex = layerID;
		subsystem->setLayerScissor(m_layerIndex + 2, { .offset = { .x = static_cast<int32_t>(floor(position.x)),
																   .y = static_cast<int32_t>(floor(position.y)) },
													   .extent = { .width = static_cast<uint32_t>(ceil(size.x)),
																   .height = static_cast<uint32_t>(ceil(size.y)) } });
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
			recalculateTextPosition();
		}
	}
	void TextBoxStyle::eraseLetterAfter() {
		if (m_cursorGlyphIndex < m_textShape->glyphCount()) {
			m_textShape->deleteClusters(m_cursorGlyphIndex);
			recalculateCursorOffset();
			recalculateTextPosition();
		}
	}

	void TextBoxStyle::toggleCursor() {
		Vector4 color = m_cursorShape->color();
		m_cursorShape->setColor(Vector4(color.r, color.g, color.b, 1.0f - color.a));
	}

	void TextBoxStyle::recalculateCursorOffset() {
		Vector2 fontOrigin = Vector2(m_contentOrigin.x + m_contentSize.x - m_textShape->size().x, m_contentOrigin.y);
		if (m_textShape->size().x < m_contentSize.x) {
			fontOrigin.x = m_contentOrigin.x;
		}
		if (!m_textShape->glyphCount()) {
			m_currentCursorOffset = Vector2(0.0f);
		} else if (m_cursorGlyphIndex < m_textShape->glyphCount()) {
			m_currentCursorOffset = m_textShape->glyphPosition(m_cursorGlyphIndex);
		} else {
			m_currentCursorOffset = m_textShape->glyphPosition(m_textShape->glyphCount() - 1) +
									Vector2(m_textShape->glyphWidth(m_textShape->glyphCount() - 1), 0.0f);
		}
		m_cursorShape->setPosition(fontOrigin + m_currentCursorOffset);
	}

	void TextBoxStyle::recalculateTextPosition() {
		Vector2 fontBaselineOrigin = Vector2(m_contentOrigin.x + m_contentSize.x - m_textShape->size().x,
											 m_contentOrigin.y + 0.8 * m_contentSize.y);
		if (m_textShape->size().x < m_contentSize.x) {
			fontBaselineOrigin.x = m_contentOrigin.x;
		}
		m_textShape->setPosition(fontBaselineOrigin - Vector2(0.0f, m_textShape->baselineOffset()));
	}

} // namespace vanadium::ui::styles
