#pragma once

#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <string>

// Pagalbinės koordinatės ir mygtukų išdėstymas (bendra logikai ir piešimui)
float getLaneCenterX(int laneIndex);
ButtonRect getPrimaryButton(GameState state);
ButtonRect getSecondaryButton(GameState state);
ButtonRect getTertiaryButton(GameState state);

// Žaidimo inicijavimas ir būsenų valdymas
void initScenery(GameData& game);
void resetGame(GameData& game);
void setStatusMessage(GameData& game, const std::string& message, float duration = 3.2f);

// Įvesties apdorojimas ir atnaujinimo funkcijos
void handleEvents(sf::RenderWindow& window, GameData& game);
void updatePlayer(GameData& game, float dt);
void spawnObstacle(GameData& game);
void spawnCoffeeBonus(GameData& game);
void updateObstacles(GameData& game, float dt);
void updateBonuses(GameData& game, float dt);
void updateScenery(GameData& game, float dt);
void checkCollisions(GameData& game);
void updateGame(GameData& game, float dt);
