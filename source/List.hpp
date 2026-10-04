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
        const auto previousHeight = getHeight();
        setText(m_textProvider());
        // setText lays out this block immediately. Move the following rows too,
        // before drawing the new lines, when wrapping changes its height.
        if (getHeight() != previousHeight && getParent()) getParent()->invalidate();
        CompactDescription::draw(renderer);
    }
private:
    std::function<std::string()> m_textProvider;
};
}
