#pragma once

#include "resources.hpp"
#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <string>

// Teksto konvertavimo ir centravimo pagalbinės funkcijos
sf::String toSfString(const std::string& utf8Text);
void drawCenteredText(sf::RenderWindow& window,
                      const sf::Font& font,
                      const std::string& text,
                      unsigned int fontSize,
                      sf::Color color,
                      sf::Vector2f centerPos);

// Pagrindinės piešimo funkcijos
void renderRoad(sf::RenderWindow& window, const GameData& game);
void renderEntities(sf::RenderWindow& window, const GameData& game, const GameResources& res);
void renderParticles(sf::RenderWindow& window, const GameData& game);
void renderHUD(sf::RenderWindow& window, const GameData& game, const GameResources& res);
void renderGame(sf::RenderWindow& window, const GameData& game, const GameResources& res);
