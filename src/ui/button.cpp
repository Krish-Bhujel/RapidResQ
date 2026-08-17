#include "ui/button.hpp"

Button::Button(sf::Vector2f pos, sf::Vector2f size, const std::string& label)
    : pos_(pos), size_(size), label_(label) {}

void Button::setLabel(const std::string& label) {
    label_ = label;
}

bool Button::contains(sf::Vector2f point) const {
    return point.x >= pos_.x && point.x <= pos_.x + size_.x &&
           point.y >= pos_.y && point.y <= pos_.y + size_.y;
}

void Button::updateHover(sf::Vector2f mousePos) {
    hover_ = contains(mousePos);
}

void Button::draw(sf::RenderWindow& window, sf::Font& font, bool fontLoaded) {
    sf::RectangleShape rect(size_);
    rect.setPosition(pos_);
    rect.setFillColor(hover_ ? sf::Color(95, 105, 125) : sf::Color(60, 68, 85));
    rect.setOutlineColor(sf::Color(150, 160, 180));
    rect.setOutlineThickness(1.5f);
    window.draw(rect);

    if (fontLoaded) {
        sf::Text text(font);
        text.setString(label_);
        text.setCharacterSize(15);
        text.setFillColor(sf::Color::White);
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        text.setPosition({pos_.x + size_.x / 2.f, pos_.y + size_.y / 2.f});
        window.draw(text);
    }
}