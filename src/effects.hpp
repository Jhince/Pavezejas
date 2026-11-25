#pragma once

#include "types.hpp"
#include <SFML/Graphics.hpp>

// Dalelių generavimas po susidūrimo arba paėmus bonusą
void spawnCrashParticles(GameData& game, sf::Vector2f origin);
void spawnBonusParticles(GameData& game, sf::Vector2f origin, sf::Color baseColor);

// Ekrano drebėjimo paleidimas
void triggerScreenShake(GameData& game, float duration, float intensity);

// Dalelių ir ekrano efektų atnaujinimas kiekvieną kadrą
void updateEffects(GameData& game, float dt);

// Skaičiuoja ekrano drebėjimo poslinkį sf::View kamerai
sf::Vector2f getScreenShakeOffset(const GameData& game);
