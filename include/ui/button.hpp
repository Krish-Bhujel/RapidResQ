#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class Button {
public:
    Button(sf::Vector2f pos, sf::Vector2f size, const std::string& label);

    void setLabel(const std::string& label);
    void updateHover(sf::Vector2f mousePos);
    bool contains(sf::Vector2f point) const;
    void draw(sf::RenderWindow& window, sf::Font& font, bool fontLoaded);

private:
    sf::Vector2f pos_;
    sf::Vector2f size_;
    std::string label_;
    bool hover_ = false;
};