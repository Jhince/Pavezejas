#pragma once

#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <string>

// Teksto ir mygtukų pagalbinės funkcijos
sf::String toSfString(const std::string& utf8Text);
void drawCenteredText(sf::RenderWindow& window,
                      const sf::Font& font,
                      const std::string& text,
                      unsigned int fontSize,
                      sf::Color color,
                      sf::Vector2f centerPos);

// Pagrindinės piešimo funkcijos
void renderRoad(sf::RenderWindow& window, const GameData& game);
void renderEntities(sf::RenderWindow& window, const GameData& game);
void renderHUD(sf::RenderWindow& window, const GameData& game, const sf::Font& font);
void renderMenu(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f mousePos);
void renderPauseOverlay(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f mousePos);
void renderEndOverlay(sf::RenderWindow& window, const GameData& game, const sf::Font& font, sf::Vector2f mousePos);
void renderGame(sf::RenderWindow& window, const GameData& game, const sf::Font& font);
