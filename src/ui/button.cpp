#include "ui/button.hpp"

Button::Button(sf::Vector2f p, sf::Vector2f s, const std::string& l)
    : pos(p), size(s), label(l) {}

void Button::setLabel(const std::string& l) {
    label = l;
}

bool Button::contains(sf::Vector2f point) const {
    if (point.x < pos.x) return false;
    if (point.x > pos.x + size.x) return false;
    if (point.y < pos.y) return false;
    if (point.y > pos.y + size.y) return false;
    return true;
}

void Button::updateHover(sf::Vector2f mousePos) {
    hover = contains(mousePos);
}

void Button::draw(sf::RenderWindow& window, sf::Font& font, bool fontLoaded) {
    sf::RectangleShape rect(size);
    rect.setPosition(pos);
    if (hover) {
        rect.setFillColor(sf::Color(95, 105, 125));
    } else {
        rect.setFillColor(sf::Color(60, 68, 85));
    }
    rect.setOutlineColor(sf::Color(150, 160, 180));
    rect.setOutlineThickness(1.5f);
    window.draw(rect);

    if (fontLoaded) {
        sf::Text text(font);
        text.setString(label);
        text.setCharacterSize(15);
        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        text.setPosition({pos.x + size.x / 2.f, pos.y + size.y / 2.f});
        window.draw(text);
    }
}