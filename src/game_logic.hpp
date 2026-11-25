#pragma once

#include "resources.hpp"
#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// Maršrutų konfigūracija ir žvaigždučių skaičiavimas
RouteConfig getRouteConfig(int routeIndex);
int getStarsForRun(float delaySeconds, const RouteConfig& route);

// Pagalbinės koordinatės ir vieningas mygtukų išdėstymas (be dubliavimo)
float getLaneCenterX(int laneIndex);
std::vector<ButtonRect> getScreenButtons(GameState state);
sf::FloatRect getMuteButtonBounds();
sf::FloatRect getHighScoreTabBounds(int routeIndex);

// Žaidimo inicijavimas ir paleidimas
void initScenery(GameData& game);
void startRouteRun(GameData& game, GameResources& res, int routeIndex);
void setStatusMessage(GameData& game, const std::string& message, float duration = 3.2f);

// Įvesties apdorojimas ir pagrindinis atnaujinimas
void handleEvents(sf::RenderWindow& window, GameData& game, GameResources& res);
void updatePlayer(GameData& game, float dt);
void spawnObstacle(GameData& game);
void spawnBonus(GameData& game);
void updateObstacles(GameData& game, GameResources& res, float dt);
void updateBonuses(GameData& game, float dt);
void updateScenery(GameData& game, float dt);
void checkCollisions(GameData& game, GameResources& res);
void updateGame(GameData& game, GameResources& res, float dt);
