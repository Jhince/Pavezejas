#include "game_logic.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace {

// Atsitiktinių skaičių generatorius (vienas bendras visai logikai)
std::mt19937& getRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

int randomInt(int minVal, int maxVal) {
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(getRng());
}

float randomFloat(float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(getRng());
}

sf::FloatRect shrinkRect(const sf::FloatRect& rect, float margin) {
    return sf::FloatRect(
        {rect.position.x + margin, rect.position.y + margin},
        {std::max(1.0f, rect.size.x - margin * 2.0f), std::max(1.0f, rect.size.y - margin * 2.0f)}
    );
}

bool isLaneClearNearY(const GameData& game, int lane, float targetY, float minGap) {
    const float laneCenter = getLaneCenterX(lane);
    for (const Obstacle& obs : game.obstacles) {
        const float obsCenterX = obs.position.x + obs.size.x * 0.5f;
        const bool sameLane = std::abs(obsCenterX - laneCenter) < LANE_WIDTH * 0.6f;
        const bool nearY = std::abs(obs.position.y - targetY) < minGap;
        if (sameLane && nearY) {
            return false;
        }
    }
    return true;
}

std::string getRandomAmbientMessage(float distanceKm) {
    if (distanceKm >= 4.0f) {
        static const std::vector<std::string> lateMessages = {
            "Jau matosi universitetas! Spausk, jau čia pat!",
            "Studentai jau prie durų – liko mažiau nei 1 km!",
            "GPS sako į dešinę... bet važiuok tiesiai!"
        };
        return lateMessages[static_cast<std::size_t>(randomInt(0, static_cast<int>(lateMessages.size()) - 1))];
    }

    static const std::vector<std::string> messages = {
        "Ar tikrai čia trumpiausias kelias?",
        "GPS vėl perskaičiuoja maršrutą...",
        "Studentai jau rašo, ar šiandien bus paskaita!",
        "Piko valanda! Visi staiga pamiršo taisykles.",
        "Tikiuosi, ne vėluosim daugiau nei 10 min...",
        "Jei laisva – paspausk W, kad greičiau nuvažiuotume!"
    };
    return messages[static_cast<std::size_t>(randomInt(0, static_cast<int>(messages.size()) - 1))];
}

} // namespace

float getLaneCenterX(int laneIndex) {
    return ROAD_LEFT + (static_cast<float>(laneIndex) + 0.5f) * LANE_WIDTH;
}

ButtonRect getPrimaryButton(GameState state) {
    const float btnW = 300.0f;
    const float btnH = 54.0f;
    const float btnX = (static_cast<float>(WINDOW_WIDTH) - btnW) * 0.5f;

    switch (state) {
        case GameState::Menu:
            return {sf::FloatRect({btnX, 465.0f}, {btnW, btnH}), "PRADĖTI ŽAIDIMĄ"};
        case GameState::Paused:
            return {sf::FloatRect({btnX, 320.0f}, {btnW, btnH}), "TĘSTI (P / ESC)"};
        case GameState::Victory:
        case GameState::GameOver:
            return {sf::FloatRect({btnX, 470.0f}, {btnW, btnH}), "ŽAISTI IŠ NAUJO"};
        default:
            return {sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f}), ""};
    }
}

ButtonRect getSecondaryButton(GameState state) {
    const float btnW = 300.0f;
    const float btnH = 54.0f;
    const float btnX = (static_cast<float>(WINDOW_WIDTH) - btnW) * 0.5f;

    switch (state) {
        case GameState::Menu:
            return {sf::FloatRect({btnX, 535.0f}, {btnW, btnH}), "IŠEITI"};
        case GameState::Paused:
            return {sf::FloatRect({btnX, 390.0f}, {btnW, btnH}), "PRADĖTI IŠ NAUJO (R)"};
        case GameState::Victory:
        case GameState::GameOver:
            return {sf::FloatRect({btnX, 540.0f}, {btnW, btnH}), "GRĮŽTI Į MENIU"};
        default:
            return {sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f}), ""};
    }
}

ButtonRect getTertiaryButton(GameState state) {
    const float btnW = 300.0f;
    const float btnH = 54.0f;
    const float btnX = (static_cast<float>(WINDOW_WIDTH) - btnW) * 0.5f;

    if (state == GameState::Paused) {
        return {sf::FloatRect({btnX, 460.0f}, {btnW, btnH}), "GRĮŽTI Į MENIU (M)"};
    }
    return {sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f}), ""};
}

void initScenery(GameData& game) {
    game.scenery.clear();

    // Medžiai ir pastatai kairėje bei dešinėje kelio pusėje
    const int countPerSide = 8;
    const float spacing = static_cast<float>(WINDOW_HEIGHT + 160) / static_cast<float>(countPerSide);

    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < countPerSide; ++i) {
            SceneryProp prop;
            const float y = static_cast<float>(i) * spacing - 80.0f + randomFloat(-20.0f, 20.0f);
            prop.isBuilding = (i % 3 == 0);

            if (prop.isBuilding) {
                prop.rectSize = {randomFloat(75.0f, 115.0f), randomFloat(95.0f, 145.0f)};
                const float x = (side == 0)
                    ? (ROAD_LEFT - prop.rectSize.x - randomFloat(45.0f, 80.0f))
                    : (ROAD_RIGHT + randomFloat(45.0f, 80.0f));
                prop.position = {x, y};
                const std::uint8_t shade = static_cast<std::uint8_t>(randomInt(55, 85));
                prop.color = sf::Color(shade, static_cast<std::uint8_t>(shade + 6), static_cast<std::uint8_t>(shade + 16));
            } else {
                prop.radius = randomFloat(18.0f, 28.0f);
                const float x = (side == 0)
                    ? (ROAD_LEFT - randomFloat(45.0f, 115.0f))
                    : (ROAD_RIGHT + randomFloat(45.0f, 115.0f));
                prop.position = {x, y};
                prop.color = sf::Color(
                    static_cast<std::uint8_t>(randomInt(34, 58)),
                    static_cast<std::uint8_t>(randomInt(115, 155)),
                    static_cast<std::uint8_t>(randomInt(42, 68))
                );
            }
            game.scenery.push_back(prop);
        }
    }
}

void resetGame(GameData& game) {
    game.gameOverReason = GameOverReason::None;

    game.player.size = {PLAYER_WIDTH, PLAYER_HEIGHT};
    game.player.position = {
        getLaneCenterX(1) - PLAYER_WIDTH * 0.5f,
        static_cast<float>(WINDOW_HEIGHT) - PLAYER_HEIGHT - 55.0f
    };
    game.player.lives = PLAYER_MAX_LIVES;
    game.player.invincibilityTimer = 0.0f;

    game.obstacles.clear();
    game.bonuses.clear();

    game.roadOffset = 0.0f;
    game.currentRoadSpeed = BASE_ROAD_SPEED;
    game.speedMultiplier = 1.0f;
    game.distanceKm = 0.0f;
    game.delaySeconds = 0.0f;
    game.score = 0;
    game.dodgedCount = 0;
    game.coffeesCollected = 0;

    game.obstacleSpawnTimer = 0.9f;
    game.bonusSpawnTimer = 7.0f;
    game.ambientMessageTimer = 5.5f;
    game.screenFlashTimer = 0.0f;

    setStatusMessage(game, "Labas! Tik greičiau į universitetą – vėluoju į paskaitą!", 4.2f);
}

void setStatusMessage(GameData& game, const std::string& message, float duration) {
    game.statusMessage = message;
    game.statusMessageTimer = duration;
}

void handleEvents(sf::RenderWindow& window, GameData& game) {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }

        // Jei naudotojas perjungia kitą langą – automatiškai įjungiame pauzę
        if (event->is<sf::Event::FocusLost>() && game.state == GameState::Playing) {
            game.state = GameState::Paused;
            continue;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            const sf::Keyboard::Key code = keyPressed->code;

            if (game.state == GameState::Menu) {
                if (code == sf::Keyboard::Key::Enter || code == sf::Keyboard::Key::Space) {
                    resetGame(game);
                    game.state = GameState::Playing;
                } else if (code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            } else if (game.state == GameState::Playing) {
                if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::P) {
                    game.state = GameState::Paused;
                }
            } else if (game.state == GameState::Paused) {
                if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::P) {
                    game.state = GameState::Playing;
                } else if (code == sf::Keyboard::Key::R) {
                    resetGame(game);
                    game.state = GameState::Playing;
                } else if (code == sf::Keyboard::Key::M) {
                    game.state = GameState::Menu;
                }
            } else if (game.state == GameState::Victory || game.state == GameState::GameOver) {
                if (code == sf::Keyboard::Key::Enter || code == sf::Keyboard::Key::Space || code == sf::Keyboard::Key::R) {
                    resetGame(game);
                    game.state = GameState::Playing;
                } else if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::M) {
                    game.state = GameState::Menu;
                }
            }
            continue;
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button != sf::Mouse::Button::Left) {
                continue;
            }

            const sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position);
            const ButtonRect primary = getPrimaryButton(game.state);
            const ButtonRect secondary = getSecondaryButton(game.state);
            const ButtonRect tertiary = getTertiaryButton(game.state);

            if (game.state == GameState::Menu) {
                if (primary.bounds.contains(mousePos)) {
                    resetGame(game);
                    game.state = GameState::Playing;
                } else if (secondary.bounds.contains(mousePos)) {
                    window.close();
                }
            } else if (game.state == GameState::Paused) {
                if (primary.bounds.contains(mousePos)) {
                    game.state = GameState::Playing;
                } else if (secondary.bounds.contains(mousePos)) {
                    resetGame(game);
                    game.state = GameState::Playing;
                } else if (tertiary.bounds.contains(mousePos)) {
                    game.state = GameState::Menu;
                }
            } else if (game.state == GameState::Victory || game.state == GameState::GameOver) {
                if (primary.bounds.contains(mousePos)) {
                    resetGame(game);
                    game.state = GameState::Playing;
                } else if (secondary.bounds.contains(mousePos)) {
                    game.state = GameState::Menu;
                }
            }
        }
    }
}

void updatePlayer(GameData& game, float dt) {
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        moveX -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        moveX += 1.0f;
    }

    const bool accelerating = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) ||
                              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);
    const bool braking = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

    if (accelerating && !braking) {
        moveY -= 1.0f;
        game.speedMultiplier = 1.35f;
    } else if (braking && !accelerating) {
        moveY += 1.0f;
        game.speedMultiplier = 0.72f;
    } else {
        game.speedMultiplier = 1.0f;
    }

    game.player.position.x += moveX * PLAYER_SIDE_SPEED * dt;
    game.player.position.y += moveY * PLAYER_VERTICAL_SPEED * dt;

    // Neleidžiame išvažiuoti už kelio ribų
    const float minX = ROAD_LEFT + 14.0f;
    const float maxX = ROAD_RIGHT - game.player.size.x - 14.0f;
    const float minY = 140.0f;
    const float maxY = static_cast<float>(WINDOW_HEIGHT) - game.player.size.y - 24.0f;

    game.player.position.x = std::clamp(game.player.position.x, minX, maxX);
    game.player.position.y = std::clamp(game.player.position.y, minY, maxY);

    if (game.player.invincibilityTimer > 0.0f) {
        game.player.invincibilityTimer = std::max(0.0f, game.player.invincibilityTimer - dt);
    }
}

void spawnObstacle(GameData& game) {
    // Surandame laisvas juostas viršutinėje dalyje, kad niekada nesukurtume neįveikiamos sienos
    std::vector<int> availableLanes;
    int busyTopLanes = 0;

    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        if (isLaneClearNearY(game, lane, -110.0f, 220.0f)) {
            availableLanes.push_back(lane);
        } else {
            ++busyTopLanes;
        }
    }

    // Visada paliekame bent 1 pravažiuojamą juostą
    if (availableLanes.empty() || busyTopLanes >= LANE_COUNT - 1) {
        return;
    }

    const int lane = availableLanes[static_cast<std::size_t>(randomInt(0, static_cast<int>(availableLanes.size()) - 1))];
    const int roll = randomInt(1, 100);

    Obstacle obs;
    obs.currentLane = lane;
    obs.targetLane = lane;

    if (roll <= 36) {
        // Skubantis miesto chuliganas – staigiai persirikiuoja į gretimą juostą!
        obs.type = ObstacleType::ChaoticDriver;
        obs.size = {62.0f, 108.0f};
        obs.speed = randomFloat(110.0f, 165.0f);
        obs.color = sf::Color(44, 52, 64); // Tamsus sportinis automobilis

        obs.willSwitchLane = true;
        if (lane == 0) {
            obs.targetLane = 1;
        } else if (lane == LANE_COUNT - 1) {
            obs.targetLane = LANE_COUNT - 2;
        } else {
            obs.targetLane = (randomInt(0, 1) == 0) ? (lane - 1) : (lane + 1);
        }
        obs.signalTimer = 0.65f;
        obs.triggerY = randomFloat(55.0f, 220.0f);
    } else if (roll <= 56) {
        // Stacionarūs kelio darbai
        obs.type = ObstacleType::Roadworks;
        obs.size = {86.0f, 48.0f};
        obs.speed = 0.0f; // Juda kartu su kelio danga
        obs.color = sf::Color(243, 156, 18);
    } else {
        // Įprastas miesto automobilis
        obs.type = ObstacleType::NormalCar;
        obs.size = {60.0f, 106.0f};
        obs.speed = randomFloat(130.0f, 200.0f);

        static const std::vector<sf::Color> carColors = {
            sf::Color(220, 68, 68),
            sf::Color(189, 195, 199),
            sf::Color(241, 196, 15),
            sf::Color(155, 89, 182),
            sf::Color(52, 152, 219)
        };
        obs.color = carColors[static_cast<std::size_t>(randomInt(0, static_cast<int>(carColors.size()) - 1))];
    }

    obs.position = {
        getLaneCenterX(lane) - obs.size.x * 0.5f,
        -obs.size.y - 20.0f
    };

    game.obstacles.push_back(obs);
}

void spawnCoffeeBonus(GameData& game) {
    std::vector<int> clearLanes;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        if (isLaneClearNearY(game, lane, -60.0f, 160.0f)) {
            clearLanes.push_back(lane);
        }
    }
    if (clearLanes.empty()) {
        return;
    }

    const int lane = clearLanes[static_cast<std::size_t>(randomInt(0, static_cast<int>(clearLanes.size()) - 1))];
    CoffeeBonus coffee;
    coffee.radius = 21.0f;
    coffee.position = {getLaneCenterX(lane), -45.0f};
    coffee.active = true;
    game.bonuses.push_back(coffee);
}

void updateObstacles(GameData& game, float dt) {
    const float effectiveRoadSpeed = game.currentRoadSpeed * game.speedMultiplier;

    for (Obstacle& obs : game.obstacles) {
        // Santykinis greitis žemyn: stacionarios kliūtys juda kelio greičiu, automobiliai lėčiau
        const float relativeSpeedY = (obs.type == ObstacleType::Roadworks)
            ? effectiveRoadSpeed
            : std::max(145.0f, effectiveRoadSpeed - obs.speed);

        obs.position.y += relativeSpeedY * dt;

        // Skubančio vairuotojo persirikiavimo logika su posūkio signalu
        if (obs.type == ObstacleType::ChaoticDriver && obs.willSwitchLane) {
            if (!obs.isSignaling && !obs.isSwitching && obs.position.y >= obs.triggerY) {
                if (isLaneClearNearY(game, obs.targetLane, obs.position.y, 150.0f)) {
                    obs.isSignaling = true;
                    if (game.statusMessageTimer <= 0.2f) {
                        setStatusMessage(game, "Atsargiai! Tas priekyje staigiai keičia juostą!", 2.5f);
                    }
                } else {
                    obs.willSwitchLane = false; // Greta užimta – šįkart nepersirikiuoja
                }
            } else if (obs.isSignaling) {
                obs.signalTimer -= dt;
                if (obs.signalTimer <= 0.0f) {
                    obs.isSignaling = false;
                    obs.isSwitching = true;
                }
            } else if (obs.isSwitching) {
                const float targetX = getLaneCenterX(obs.targetLane) - obs.size.x * 0.5f;
                const float diff = targetX - obs.position.x;
                const float step = 240.0f * dt;

                if (std::abs(diff) <= step) {
                    obs.position.x = targetX;
                    obs.currentLane = obs.targetLane;
                    obs.isSwitching = false;
                    obs.willSwitchLane = false;
                } else {
                    obs.position.x += (diff > 0.0f ? step : -step);
                }
            }
        }

        // Taškai už sėkmingai pralenktą kliūtį
        if (!obs.passedPlayer && obs.position.y > game.player.position.y + game.player.size.y) {
            obs.passedPlayer = true;
            game.dodgedCount += 1;
            game.score += (obs.type == ObstacleType::ChaoticDriver) ? 40 : 25;
        }
    }

    game.obstacles.erase(
        std::remove_if(game.obstacles.begin(), game.obstacles.end(), [](const Obstacle& obs) {
            return obs.position.y > static_cast<float>(WINDOW_HEIGHT) + 140.0f;
        }),
        game.obstacles.end()
    );
}

void updateBonuses(GameData& game, float dt) {
    const float bonusSpeed = game.currentRoadSpeed * game.speedMultiplier * 0.85f;
    for (CoffeeBonus& bonus : game.bonuses) {
        bonus.position.y += bonusSpeed * dt;
    }

    game.bonuses.erase(
        std::remove_if(game.bonuses.begin(), game.bonuses.end(), [](const CoffeeBonus& bonus) {
            return !bonus.active || bonus.position.y > static_cast<float>(WINDOW_HEIGHT) + 60.0f;
        }),
        game.bonuses.end()
    );
}

void updateScenery(GameData& game, float dt) {
    const float scrollSpeed = game.currentRoadSpeed * game.speedMultiplier;
    for (SceneryProp& prop : game.scenery) {
        prop.position.y += scrollSpeed * dt;
        const float height = prop.isBuilding ? prop.rectSize.y : prop.radius * 2.0f;
        if (prop.position.y > static_cast<float>(WINDOW_HEIGHT) + 80.0f) {
            prop.position.y = -height - randomFloat(20.0f, 90.0f);
        }
    }
}

void checkCollisions(GameData& game) {
    const sf::FloatRect playerHitbox = shrinkRect(sf::FloatRect(game.player.position, game.player.size), 7.0f);

    // 1. Tikriname susidūrimus su kliūtimis
    if (game.player.invincibilityTimer <= 0.0f) {
        for (const Obstacle& obs : game.obstacles) {
            const sf::FloatRect obsHitbox = shrinkRect(sf::FloatRect(obs.position, obs.size), 6.0f);
            if (playerHitbox.findIntersection(obsHitbox).has_value()) {
                game.player.lives -= 1;
                game.player.invincibilityTimer = INVINCIBILITY_DURATION;
                game.screenFlashTimer = 0.35f;
                game.delaySeconds += 60.0f; // +1 minutė baudos už avariją

                if (obs.type == ObstacleType::ChaoticDriver) {
                    setStatusMessage(game, "BUM! Oho, čia tikrai bus 1 žvaigždutė! (+1 min)", 3.5f);
                } else if (obs.type == ObstacleType::Roadworks) {
                    setStatusMessage(game, "BUM! Įvažiavai į kelio darbus! Atsargiau! (+1 min)", 3.5f);
                } else {
                    setStatusMessage(game, "BUM! Mano kava išsipylė ant skaidrių! (+1 min)", 3.5f);
                }

                if (game.player.lives <= 0) {
                    game.state = GameState::GameOver;
                    game.gameOverReason = GameOverReason::OutOfLives;
                }
                break;
            }
        }
    }

    // 2. Tikriname kavos bonusų surinkimą
    for (CoffeeBonus& bonus : game.bonuses) {
        if (!bonus.active) {
            continue;
        }
        const sf::FloatRect bonusRect(
            {bonus.position.x - bonus.radius, bonus.position.y - bonus.radius},
            {bonus.radius * 2.0f, bonus.radius * 2.0f}
        );

        if (playerHitbox.findIntersection(bonusRect).has_value()) {
            bonus.active = false;
            game.coffeesCollected += 1;
            game.score += 150;
            game.delaySeconds = std::max(0.0f, game.delaySeconds - 30.0f);

            if (game.player.lives < PLAYER_MAX_LIVES) {
                game.player.lives += 1;
                setStatusMessage(game, "O, karšta kava! Ačiū! (+1 kantrybės, -0.5 min)", 3.2f);
            } else {
                setStatusMessage(game, "Puikus espresso! (+150 tšk. ir -0.5 min)", 3.0f);
            }
        }
    }
}

void updateGame(GameData& game, float dt) {
    // Meniu fone švelniai animuojame kelią, kad ekranas būtų gyvas
    if (game.state == GameState::Menu) {
        game.roadOffset = std::fmod(game.roadOffset + 150.0f * dt, 90.0f);
        for (SceneryProp& prop : game.scenery) {
            prop.position.y += 150.0f * dt;
            const float height = prop.isBuilding ? prop.rectSize.y : prop.radius * 2.0f;
            if (prop.position.y > static_cast<float>(WINDOW_HEIGHT) + 80.0f) {
                prop.position.y = -height - 40.0f;
            }
        }
        return;
    }

    if (game.state != GameState::Playing) {
        return;
    }

    // 1. Progresas ir greičio didėjimas
    const float progress = std::clamp(game.distanceKm / TARGET_DISTANCE_KM, 0.0f, 1.0f);
    game.currentRoadSpeed = BASE_ROAD_SPEED + MAX_EXTRA_SPEED * progress;

    // 2. Žaidėjas, kelias ir aplinka
    updatePlayer(game, dt);
    const float effectiveRoadSpeed = game.currentRoadSpeed * game.speedMultiplier;
    game.roadOffset = std::fmod(game.roadOffset + effectiveRoadSpeed * dt, 90.0f);
    updateScenery(game, dt);

    // 3. Atstumas ir paskaitos vėlavimo laikmatis
    constexpr float KM_PER_PIXEL = 0.000155f;
    game.distanceKm += effectiveRoadSpeed * KM_PER_PIXEL * dt;
    game.delaySeconds += dt * DELAY_TIME_SCALE;
    game.score += static_cast<int>(effectiveRoadSpeed * dt * 0.05f);

    // 4. Kliūčių ir kavos generavimas
    game.obstacleSpawnTimer -= dt;
    if (game.obstacleSpawnTimer <= 0.0f) {
        spawnObstacle(game);
        const float minInterval = 0.42f;
        const float maxInterval = 1.05f - 0.42f * progress;
        game.obstacleSpawnTimer = randomFloat(minInterval, std::max(minInterval + 0.1f, maxInterval));
    }

    game.bonusSpawnTimer -= dt;
    if (game.bonusSpawnTimer <= 0.0f) {
        spawnCoffeeBonus(game);
        game.bonusSpawnTimer = randomFloat(8.5f, 13.5f);
    }

    // 5. Objektų judėjimas ir susidūrimai
    updateObstacles(game, dt);
    updateBonuses(game, dt);
    checkCollisions(game);

    // 6. Žinučių ir efektų laikmačiai
    if (game.statusMessageTimer > 0.0f) {
        game.statusMessageTimer = std::max(0.0f, game.statusMessageTimer - dt);
    }
    if (game.screenFlashTimer > 0.0f) {
        game.screenFlashTimer = std::max(0.0f, game.screenFlashTimer - dt);
    }

    game.ambientMessageTimer -= dt;
    if (game.ambientMessageTimer <= 0.0f) {
        if (game.statusMessageTimer <= 0.0f) {
            setStatusMessage(game, getRandomAmbientMessage(game.distanceKm), 3.4f);
        }
        game.ambientMessageTimer = randomFloat(6.5f, 10.0f);
    }

    // 7. Pergalės arba pralaimėjimo sąlygos
    if (game.state == GameState::Playing) {
        if (game.distanceKm >= TARGET_DISTANCE_KM) {
            game.distanceKm = TARGET_DISTANCE_KM;
            game.score += 500;
            game.state = GameState::Victory;
        } else if (game.delaySeconds >= MAX_DELAY_MINUTES * 60.0f) {
            game.state = GameState::GameOver;
            game.gameOverReason = GameOverReason::TooLate;
        }
    }
}
