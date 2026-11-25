#include "effects.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

std::mt19937& getEffectsRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

float randRange(float a, float b) {
    std::uniform_real_distribution<float> dist(a, b);
    return dist(getEffectsRng());
}

} // namespace

void spawnCrashParticles(GameData& game, sf::Vector2f origin) {
    // Kibirkštys ir dūmų debesėliai susidūrimo vietoje
    for (int i = 0; i < 24; ++i) {
        const float angle = randRange(0.0f, 6.283185f);
        const float speed = randRange(70.0f, 270.0f);

        Particle p;
        p.position = origin;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.maxLifetime = randRange(0.32f, 0.72f);
        p.lifetime = p.maxLifetime;

        if (i % 3 == 0) {
            // Dūmai
            p.size = randRange(8.0f, 15.0f);
            p.color = sf::Color(165, 172, 182);
        } else {
            // Oranžinės / geltonos kibirkštys
            p.size = randRange(4.0f, 8.0f);
            p.color = (i % 2 == 0) ? sf::Color(255, 165, 40) : sf::Color(255, 226, 85);
        }
        game.particles.push_back(p);
    }
}

void spawnBonusParticles(GameData& game, sf::Vector2f origin, sf::Color baseColor) {
    for (int i = 0; i < 16; ++i) {
        const float angle = randRange(0.0f, 6.283185f);
        const float speed = randRange(60.0f, 185.0f);

        Particle p;
        p.position = origin;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed - 40.0f};
        p.size = randRange(4.0f, 9.0f);
        p.color = baseColor;
        p.maxLifetime = randRange(0.35f, 0.65f);
        p.lifetime = p.maxLifetime;
        game.particles.push_back(p);
    }
}

void triggerScreenShake(GameData& game, float duration, float intensity) {
    game.screenShakeTimer = std::max(game.screenShakeTimer, duration);
    game.screenShakeIntensity = std::max(game.screenShakeIntensity, intensity);
}

void updateEffects(GameData& game, float dt) {
    if (game.screenFlashTimer > 0.0f) {
        game.screenFlashTimer = std::max(0.0f, game.screenFlashTimer - dt);
    }
    if (game.screenShakeTimer > 0.0f) {
        game.screenShakeTimer = std::max(0.0f, game.screenShakeTimer - dt);
        if (game.screenShakeTimer <= 0.0f) {
            game.screenShakeIntensity = 0.0f;
        }
    }

    for (Particle& p : game.particles) {
        p.lifetime -= dt;
        p.position += p.velocity * dt;
        p.velocity *= (1.0f - std::min(0.95f, 2.2f * dt));
    }

    game.particles.erase(
        std::remove_if(game.particles.begin(), game.particles.end(), [](const Particle& p) {
            return p.lifetime <= 0.0f;
        }),
        game.particles.end()
    );
}

sf::Vector2f getScreenShakeOffset(const GameData& game) {
    if (game.screenShakeTimer <= 0.0f || game.screenShakeIntensity <= 0.0f) {
        return {0.0f, 0.0f};
    }
    const float strength = game.screenShakeIntensity * std::min(1.0f, game.screenShakeTimer * 3.0f);
    return {randRange(-strength, strength), randRange(-strength, strength)};
}
