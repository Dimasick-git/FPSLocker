#pragma once
#include <tesla.hpp>
#include <functional>

namespace tsl::elm {
using ListItem2 = CompactListItem;

// Text remains live while the game changes state; wrapping follows the compact UI.
class LiveDescription : public CompactDescription {
public:
    explicit LiveDescription(std::function<std::string()> text)
        : CompactDescription(text()), m_textProvider(std::move(text)) {}
    void draw(gfx::Renderer* renderer) override {
        setText(m_textProvider());
        CompactDescription::draw(renderer);
    }
private:
    std::function<std::string()> m_textProvider;
};
}
