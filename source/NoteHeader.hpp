#pragma once
#include <tesla.hpp>

namespace tsl::elm {
class NoteHeader : public CompactDescription {
public:
    NoteHeader(const std::string& text, bool separator = false, Color barColor = {0xF, 0xF, 0xF, 0xF})
        : CompactDescription(text), m_separator(separator), m_barColor(barColor) {}
    void draw(gfx::Renderer* renderer) override {
        CompactDescription::draw(renderer);
        renderer->drawRect(getX() - 2, getY() + 6, 3, std::max<int>(0, getHeight() - 12), renderer->a(m_barColor));
        if (m_separator) renderer->drawRect(getX(), getBottomBound(), getWidth(), 1, renderer->a(style::color::ColorFrame));
    }
private:
    bool m_separator;
    Color m_barColor;
};
}
