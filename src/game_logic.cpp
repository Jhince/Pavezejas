#include "game_logic.hpp"
#include "effects.hpp"
#include "storage.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr const char* HIGHSCORE_FILE = "highscores.txt";

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
        const bool sameLane = std::abs(obsCenterX - laneCenter) < LANE_WIDTH * 0.62f;
        const float obsCenterY = obs.position.y + obs.size.y * 0.5f;
        const bool nearY = std::abs(obsCenterY - targetY) < (minGap + obs.size.y * 0.5f);
        if (sameLane && nearY) {
            return false;
        }
    }
    return true;
}

std::string getRandomAmbientMessage(const GameData& game, const RouteConfig& route) {
    const float remainingKm = route.targetDistanceKm - game.distanceKm;
    if (remainingKm <= 1.0f) {
        static const std::vector<std::string> lateMessages = {
            "Jau matosi Saulėtekio rūmai! Spausk, jau čia pat!",
            "Studentai jau prie durų – liko mažiau nei 1 km!",
            "GPS sako į dešinę... bet važiuok tiesiai!"
        };
        return lateMessages[static_cast<std::size_t>(randomInt(0, static_cast<int>(lateMessages.size()) - 1))];
    }

    if (game.cleanDriveTimer > 16.0f && randomInt(0, 2) == 0) {
        return "Oho, važiuoji taip sklandžiai! Tikrai verta 5 žvaigždučių!";
    }

    if (game.speedMultiplier < 0.85f) {
        return "Kodėl taip lėtai?! Paskaita nelauks!";
    }

    static const std::vector<std::string> messages = {
        "Ar tikrai čia trumpiausias kelias?",
        "GPS vėl perskaičiuoja maršrutą...",
        "Studentai jau rašo, ar šiandien bus paskaita!",
        "Piko valanda! Visi staiga pamiršo taisykles.",
        "Saugokis raudono troleibuso – jis užima visą juostą!",
        "Tie paspirtukininkai visur nardo kaip žaibai!"
    };
    return messages[static_cast<std::size_t>(randomInt(0, static_cast<int>(messages.size()) - 1))];
}

void trySubmitPlayerName(GameData& game, GameResources& res) {
    const std::string cleaned = trimWhitespace(game.playerNameInput);
    if (cleaned.empty()) {
        game.nameValidationError = "Įvesk bent 1 simbolį (raidę arba skaičių)!";
        playSound(res, game, SoundId::Click);
        return;
    }

    const RouteConfig route = getRouteConfig(game.selectedRoute);
    HighScoreEntry entry;
    entry.playerName = cleaned;
    entry.routeIndex = game.selectedRoute;
    entry.score = game.score;
    entry.delaySeconds = game.delaySeconds;
    entry.stars = getStarsForRun(game.delaySeconds, route);

    addHighScore(game.highScores, entry);
    saveHighScores(HIGHSCORE_FILE, game.highScores);
    game.scoreSavedThisRun = true;
    game.highScoreTab = game.selectedRoute;
    game.nameValidationError.clear();
    playSound(res, game, SoundId::Bonus);
    game.state = GameState::Victory;
}

} // namespace

RouteConfig getRouteConfig(int routeIndex) {
    static const RouteConfig routes[ROUTE_COUNT] = {
        {
            0,
            "Antakalnis -> Saulėtekis",
            "LENGVAS",
            "3.0 km | Riba: 8 min | Ramesnis rytinis eismas",
            3.0f, 8.0f, 335.0f, 200.0f, 0.56f, 1.15f,
            22, 12, 14, 12
        },
        {
            1,
            "Centras -> Saulėtekis",
            "VIDUTINIS",
            "5.0 km | Riba: 10 min | Troleibusai ir paspirtukai",
            5.0f, 10.0f, 365.0f, 250.0f, 0.44f, 1.00f,
            28, 18, 16, 12
        },
        {
            2,
            "Pašilaičiai -> Saulėtekis",
            "SUNKUS (PIKAS)",
            "7.0 km | Riba: 12 min | Chaotiška piko valanda!",
            7.0f, 12.0f, 395.0f, 290.0f, 0.36f, 0.86f,
            34, 20, 18, 10
        }
    };

    const int safeIdx = std::clamp(routeIndex, 0, ROUTE_COUNT - 1);
    return routes[safeIdx];
}

int getStarsForRun(float delaySeconds, const RouteConfig& route) {
    const float ratio = (delaySeconds / 60.0f) / std::max(1.0f, route.maxDelayMinutes);
    if (ratio < 0.62f) {
        return 5;
    }
    if (ratio <= 0.84f) {
        return 4;
    }
    return 3;
}

float getLaneCenterX(int laneIndex) {
    return ROAD_LEFT + (static_cast<float>(laneIndex) + 0.5f) * LANE_WIDTH;
}

sf::FloatRect getMuteButtonBounds() {
    return sf::FloatRect({1110.0f, 660.0f}, {150.0f, 42.0f});
}

sf::FloatRect getHighScoreTabBounds(int routeIndex) {
    const float tabW = 220.0f;
    const float gap = 18.0f;
    const float totalW = tabW * ROUTE_COUNT + gap * (ROUTE_COUNT - 1);
    const float startX = (static_cast<float>(WINDOW_WIDTH) - totalW) * 0.5f;
    return sf::FloatRect({startX + static_cast<float>(routeIndex) * (tabW + gap), 165.0f}, {tabW, 44.0f});
}

std::vector<ButtonRect> getScreenButtons(GameState state) {
    std::vector<ButtonRect> buttons;
    const float btnW = 310.0f;
    const float btnH = 50.0f;
    const float centerX = (static_cast<float>(WINDOW_WIDTH) - btnW) * 0.5f;

    switch (state) {
        case GameState::Menu:
            buttons.push_back({sf::FloatRect({centerX, 342.0f}, {btnW, btnH}), "PRADĖTI (MARŠRUTAI)", 1, sf::Color(0, 196, 114)});
            buttons.push_back({sf::FloatRect({centerX, 406.0f}, {btnW, btnH}), "REKORDAI (TOP 5)", 2, sf::Color(37, 99, 235)});
            buttons.push_back({sf::FloatRect({centerX, 470.0f}, {btnW, btnH}), "KAIP ŽAISTI", 3, sf::Color(124, 58, 237)});
            buttons.push_back({sf::FloatRect({centerX, 534.0f}, {btnW, btnH}), "IŠEITI", 4, sf::Color(225, 29, 72)});
            break;

        case GameState::RouteSelect: {
            const float cardW = 600.0f;
            const float cardH = 88.0f;
            const float cardX = (static_cast<float>(WINDOW_WIDTH) - cardW) * 0.5f;
            buttons.push_back({sf::FloatRect({cardX, 200.0f}, {cardW, cardH}), "1. Antakalnis -> Saulėtekis", 10, sf::Color(16, 152, 98)});
            buttons.push_back({sf::FloatRect({cardX, 306.0f}, {cardW, cardH}), "2. Centras -> Saulėtekis", 11, sf::Color(37, 99, 235)});
            buttons.push_back({sf::FloatRect({cardX, 412.0f}, {cardW, cardH}), "3. Pašilaičiai -> Saulėtekis", 12, sf::Color(217, 56, 72)});
            buttons.push_back({sf::FloatRect({centerX, 535.0f}, {btnW, btnH}), "ATGAL Į MENIU", 0, sf::Color(30, 41, 59)});
            break;
        }

        case GameState::HowToPlay:
        case GameState::HighScores:
            buttons.push_back({sf::FloatRect({centerX, 580.0f}, {btnW, btnH}), "ATGAL Į MENIU", 0, sf::Color(30, 41, 59)});
            break;

        case GameState::Paused:
            buttons.push_back({sf::FloatRect({centerX, 315.0f}, {btnW, btnH}), "TĘSTI (P / ESC)", 1, sf::Color(0, 196, 114)});
            buttons.push_back({sf::FloatRect({centerX, 382.0f}, {btnW, btnH}), "PRADĖTI IŠ NAUJO (R)", 2, sf::Color(37, 99, 235)});
            buttons.push_back({sf::FloatRect({centerX, 449.0f}, {btnW, btnH}), "GRĮŽTI Į MENIU", 0, sf::Color(225, 29, 72)});
            break;

        case GameState::NameInput:
            buttons.push_back({sf::FloatRect({centerX, 455.0f}, {btnW, btnH}), "IŠSAUGOTI REZULTATĄ", 1, sf::Color(0, 196, 114)});
            buttons.push_back({sf::FloatRect({centerX, 520.0f}, {btnW, btnH}), "PRALEISTI", 2, sf::Color(30, 41, 59)});
            break;

        case GameState::Victory:
        case GameState::GameOver:
            buttons.push_back({sf::FloatRect({centerX, 455.0f}, {btnW, btnH}), "ŽAISTI IŠ NAUJO", 1, sf::Color(0, 196, 114)});
            buttons.push_back({sf::FloatRect({centerX, 518.0f}, {btnW, btnH}), "REKORDAI (TOP 5)", 2, sf::Color(37, 99, 235)});
            buttons.push_back({sf::FloatRect({centerX, 581.0f}, {btnW, btnH}), "GRĮŽTI Į MENIU", 0, sf::Color(30, 41, 59)});
            break;

        default:
            break;
    }

    return buttons;
}

void initScenery(GameData& game) {
    game.scenery.clear();

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

void startRouteRun(GameData& game, GameResources& res, int routeIndex) {
    game.selectedRoute = std::clamp(routeIndex, 0, ROUTE_COUNT - 1);
    const RouteConfig route = getRouteConfig(game.selectedRoute);

    game.gameOverReason = GameOverReason::None;
    game.player.size = {PLAYER_WIDTH, PLAYER_HEIGHT};
    game.player.position = {
        getLaneCenterX(1) - PLAYER_WIDTH * 0.5f,
        static_cast<float>(WINDOW_HEIGHT) - PLAYER_HEIGHT - 55.0f
    };
    game.player.lives = PLAYER_MAX_LIVES;
    game.player.invincibilityTimer = 0.0f;
    game.player.greenWaveTimer = 0.0f;

    game.obstacles.clear();
    game.bonuses.clear();
    game.particles.clear();

    game.roadOffset = 0.0f;
    game.currentRoadSpeed = route.baseRoadSpeed;
    game.speedMultiplier = 1.0f;
    game.distanceKm = 0.0f;
    game.delaySeconds = 0.0f;
    game.score = 0;
    game.dodgedCount = 0;
    game.coffeesCollected = 0;
    game.usbCollected = 0;
    game.cleanDriveTimer = 0.0f;

    game.obstacleSpawnTimer = 0.8f;
    game.bonusSpawnTimer = 5.5f;
    game.ambientMessageTimer = 5.0f;
    game.screenFlashTimer = 0.0f;
    game.screenShakeTimer = 0.0f;
    game.screenShakeIntensity = 0.0f;

    game.scoreSavedThisRun = false;
    game.nameValidationError.clear();

    // Pradedame nuo 3-2-1 starto atskaitos
    game.countdownTimer = 2.7f;
    game.lastCountdownBeep = 4;
    game.state = GameState::Countdown;

    setStatusMessage(game, "Labas! Maršrutas: " + route.name + ". Tik greičiau!", 4.5f);
    playSound(res, game, SoundId::Click);
}

void setStatusMessage(GameData& game, const std::string& message, float duration) {
    game.statusMessage = message;
    game.statusMessageTimer = duration;
}

void handleEvents(sf::RenderWindow& window, GameData& game, GameResources& res) {
    const sf::View baseView(sf::FloatRect({0.0f, 0.0f}, {static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)}));

    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            return;
        }

        if (event->is<sf::Event::FocusLost>() && game.state == GameState::Playing) {
            game.state = GameState::Paused;
            continue;
        }

        // Teksto įvedimas vardo įvedimo ekrane (su griežta validacija)
        if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
            if (game.state != GameState::NameInput) {
                continue;
            }
            const char32_t u = textEntered->unicode;
            if (u == 8) { // Backspace
                if (!game.playerNameInput.empty()) {
                    game.playerNameInput.pop_back();
                }
                game.nameValidationError.clear();
            } else if (u == 13 || u == 10) { // Enter
                trySubmitPlayerName(game, res);
            } else if (isAllowedNameChar(u)) {
                if (game.playerNameInput.size() < MAX_NAME_LENGTH) {
                    game.playerNameInput.push_back(static_cast<char>(u));
                    game.nameValidationError.clear();
                } else {
                    game.nameValidationError = "Maksimalus ilgis – 12 simbolių!";
                }
            } else if (u >= 32) {
                game.nameValidationError = "Leidžiamos tik raidės (A-Z), skaičiai, tarpas, _ ir -!";
            }
            continue;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            const sf::Keyboard::Key code = keyPressed->code;

            // Garsų nutildymas M klavišu (išskyrus kai vedamas vardas!)
            if (code == sf::Keyboard::Key::M && game.state != GameState::NameInput) {
                game.soundMuted = !game.soundMuted;
                continue;
            }

            if (game.state == GameState::Menu) {
                if (code == sf::Keyboard::Key::Enter || code == sf::Keyboard::Key::Space) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::RouteSelect;
                } else if (code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            } else if (game.state == GameState::RouteSelect) {
                if (code == sf::Keyboard::Key::Num1) {
                    startRouteRun(game, res, 0);
                } else if (code == sf::Keyboard::Key::Num2) {
                    startRouteRun(game, res, 1);
                } else if (code == sf::Keyboard::Key::Num3) {
                    startRouteRun(game, res, 2);
                } else if (code == sf::Keyboard::Key::Escape) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Menu;
                }
            } else if (game.state == GameState::HowToPlay) {
                if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::Enter) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Menu;
                }
            } else if (game.state == GameState::HighScores) {
                if (code == sf::Keyboard::Key::Num1) { game.highScoreTab = 0; playSound(res, game, SoundId::Click); }
                else if (code == sf::Keyboard::Key::Num2) { game.highScoreTab = 1; playSound(res, game, SoundId::Click); }
                else if (code == sf::Keyboard::Key::Num3) { game.highScoreTab = 2; playSound(res, game, SoundId::Click); }
                else if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::Enter) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Menu;
                }
            } else if (game.state == GameState::Playing) {
                if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::P) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Paused;
                }
            } else if (game.state == GameState::Paused) {
                if (code == sf::Keyboard::Key::Escape || code == sf::Keyboard::Key::P) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Playing;
                } else if (code == sf::Keyboard::Key::R) {
                    startRouteRun(game, res, game.selectedRoute);
                }
            } else if (game.state == GameState::NameInput) {
                if (code == sf::Keyboard::Key::Escape) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Victory;
                }
            } else if (game.state == GameState::Victory || game.state == GameState::GameOver) {
                if (code == sf::Keyboard::Key::Enter || code == sf::Keyboard::Key::Space || code == sf::Keyboard::Key::R) {
                    startRouteRun(game, res, game.selectedRoute);
                } else if (code == sf::Keyboard::Key::Escape) {
                    playSound(res, game, SoundId::Click);
                    game.state = GameState::Menu;
                }
            }
            continue;
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mousePressed->button != sf::Mouse::Button::Left) {
                continue;
            }

            const sf::Vector2f mousePos = window.mapPixelToCoords(mousePressed->position, baseView);

            // Garsų išjungimo mygtukas apatiniame dešiniajame kampe
            if (getMuteButtonBounds().contains(mousePos)) {
                game.soundMuted = !game.soundMuted;
                playSound(res, game, SoundId::Click);
                continue;
            }

            // Rekordų maršrutų skirtukai
            if (game.state == GameState::HighScores) {
                for (int r = 0; r < ROUTE_COUNT; ++r) {
                    if (getHighScoreTabBounds(r).contains(mousePos)) {
                        game.highScoreTab = r;
                        playSound(res, game, SoundId::Click);
                        break;
                    }
                }
            }

            const std::vector<ButtonRect> buttons = getScreenButtons(game.state);
            for (const ButtonRect& btn : buttons) {
                if (!btn.bounds.contains(mousePos)) {
                    continue;
                }

                playSound(res, game, SoundId::Click);

                if (game.state == GameState::Menu) {
                    if (btn.actionId == 1) game.state = GameState::RouteSelect;
                    else if (btn.actionId == 2) game.state = GameState::HighScores;
                    else if (btn.actionId == 3) game.state = GameState::HowToPlay;
                    else if (btn.actionId == 4) window.close();
                } else if (game.state == GameState::RouteSelect) {
                    if (btn.actionId >= 10 && btn.actionId <= 12) {
                        startRouteRun(game, res, btn.actionId - 10);
                    } else {
                        game.state = GameState::Menu;
                    }
                } else if (game.state == GameState::HowToPlay || game.state == GameState::HighScores) {
                    game.state = GameState::Menu;
                } else if (game.state == GameState::Paused) {
                    if (btn.actionId == 1) game.state = GameState::Playing;
                    else if (btn.actionId == 2) startRouteRun(game, res, game.selectedRoute);
                    else game.state = GameState::Menu;
                } else if (game.state == GameState::NameInput) {
                    if (btn.actionId == 1) trySubmitPlayerName(game, res);
                    else game.state = GameState::Victory;
                } else if (game.state == GameState::Victory || game.state == GameState::GameOver) {
                    if (btn.actionId == 1) startRouteRun(game, res, game.selectedRoute);
                    else if (btn.actionId == 2) {
                        game.highScoreTab = game.selectedRoute;
                        game.state = GameState::HighScores;
                    } else {
                        game.state = GameState::Menu;
                    }
                }
                break;
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

    if (game.player.greenWaveTimer > 0.0f) {
        game.player.greenWaveTimer = std::max(0.0f, game.player.greenWaveTimer - dt);
        game.speedMultiplier = 1.55f; // Žalios bangos turbo greitis!
        if (accelerating) moveY -= 0.7f;
        else if (braking) moveY += 0.7f;
    } else if (accelerating && !braking) {
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
    const RouteConfig route = getRouteConfig(game.selectedRoute);

    std::vector<int> availableLanes;
    int busyTopLanes = 0;

    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        if (isLaneClearNearY(game, lane, -120.0f, 220.0f)) {
            availableLanes.push_back(lane);
        } else {
            ++busyTopLanes;
        }
    }

    if (availableLanes.empty() || busyTopLanes >= LANE_COUNT - 1) {
        return;
    }

    const int lane = availableLanes[static_cast<std::size_t>(randomInt(0, static_cast<int>(availableLanes.size()) - 1))];
    const int roll = randomInt(1, 100);

    Obstacle obs;
    obs.currentLane = lane;
    obs.targetLane = lane;

    const int tChaotic = route.chaoticChance;
    const int tTrolley = tChaotic + route.trolleyChance;
    const int tScooter = tTrolley + route.scooterChance;
    const int tRoadworks = tScooter + route.roadworksChance;

    if (roll <= tChaotic) {
        obs.type = ObstacleType::ChaoticDriver;
        obs.size = {62.0f, 108.0f};
        obs.speed = randomFloat(115.0f, 165.0f);
        obs.color = sf::Color(44, 52, 64);

        obs.willSwitchLane = true;
        if (lane == 0) obs.targetLane = 1;
        else if (lane == LANE_COUNT - 1) obs.targetLane = LANE_COUNT - 2;
        else obs.targetLane = (randomInt(0, 1) == 0) ? (lane - 1) : (lane + 1);

        obs.signalTimer = 0.65f;
        obs.triggerY = randomFloat(55.0f, 220.0f);
    } else if (roll <= tTrolley) {
        // Ilgas ir lėtas Vilniaus troleibusas
        obs.type = ObstacleType::Trolleybus;
        obs.size = {74.0f, 208.0f};
        obs.speed = randomFloat(70.0f, 105.0f);
        obs.color = sf::Color(208, 44, 48);
    } else if (roll <= tScooter) {
        // Greitas, siauras, vingiuojantis paspirtukininkas
        obs.type = ObstacleType::Scooter;
        obs.size = {34.0f, 64.0f};
        obs.speed = randomFloat(40.0f, 80.0f); // Santykinai greičiau artėja žemyn
        obs.color = sf::Color(245, 160, 35);
        obs.weavePhase = randomFloat(0.0f, 6.28f);
    } else if (roll <= tRoadworks) {
        obs.type = ObstacleType::Roadworks;
        obs.size = {86.0f, 48.0f};
        obs.speed = 0.0f;
        obs.color = sf::Color(243, 156, 18);
    } else {
        obs.type = ObstacleType::NormalCar;
        obs.size = {60.0f, 106.0f};
        obs.speed = randomFloat(130.0f, 195.0f);
        obs.textureIndex = randomInt(0, 4);

        static const std::vector<sf::Color> carColors = {
            sf::Color(220, 68, 68),
            sf::Color(189, 195, 199),
            sf::Color(52, 152, 219),
            sf::Color(241, 196, 15),
            sf::Color(155, 89, 182)
        };
        obs.color = carColors[static_cast<std::size_t>(obs.textureIndex)];
    }

    obs.position = {
        getLaneCenterX(lane) - obs.size.x * 0.5f,
        -obs.size.y - 24.0f
    };
    obs.weaveBaseX = obs.position.x;

    game.obstacles.push_back(obs);
}

void spawnBonus(GameData& game) {
    std::vector<int> clearLanes;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        if (isLaneClearNearY(game, lane, -60.0f, 170.0f)) {
            clearLanes.push_back(lane);
        }
    }
    if (clearLanes.empty()) {
        return;
    }

    const int lane = clearLanes[static_cast<std::size_t>(randomInt(0, static_cast<int>(clearLanes.size()) - 1))];
    const int roll = randomInt(1, 100);

    BonusItem item;
    item.radius = 23.0f;
    item.position = {getLaneCenterX(lane), -48.0f};
    item.active = true;
    item.pulsePhase = randomFloat(0.0f, 6.28f);

    if (roll <= 50) {
        item.type = BonusType::Coffee;
    } else if (roll <= 78) {
        item.type = BonusType::UsbDrive;
    } else {
        item.type = BonusType::GreenWave;
    }

    game.bonuses.push_back(item);
}

void updateObstacles(GameData& game, GameResources& res, float dt) {
    const float effectiveRoadSpeed = game.currentRoadSpeed * game.speedMultiplier;

    for (Obstacle& obs : game.obstacles) {
        const float relativeSpeedY = (obs.type == ObstacleType::Roadworks)
            ? effectiveRoadSpeed
            : std::max(145.0f, effectiveRoadSpeed - obs.speed);

        obs.position.y += relativeSpeedY * dt;

        // Paspirtukininko vingiavimas juostoje
        if (obs.type == ObstacleType::Scooter) {
            obs.weavePhase += dt * 4.2f;
            obs.position.x = obs.weaveBaseX + std::sin(obs.weavePhase) * 28.0f;
        }

        // Skubančio vairuotojo persirikiavimas su posūkio signalu ir pypimu
        if (obs.type == ObstacleType::ChaoticDriver && obs.willSwitchLane) {
            if (!obs.isSignaling && !obs.isSwitching && obs.position.y >= obs.triggerY) {
                if (isLaneClearNearY(game, obs.targetLane, obs.position.y, 165.0f)) {
                    obs.isSignaling = true;
                    playSound(res, game, SoundId::Horn);
                    if (game.statusMessageTimer <= 0.2f) {
                        setStatusMessage(game, "Atsargiai! Tas priekyje staigiai keičia juostą!", 2.5f);
                    }
                } else {
                    obs.willSwitchLane = false;
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
                const float step = 245.0f * dt;

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

        if (!obs.passedPlayer && obs.position.y > game.player.position.y + game.player.size.y) {
            obs.passedPlayer = true;
            game.dodgedCount += 1;
            if (obs.type == ObstacleType::ChaoticDriver || obs.type == ObstacleType::Trolleybus) {
                game.score += 40;
            } else {
                game.score += 25;
            }
        }
    }

    game.obstacles.erase(
        std::remove_if(game.obstacles.begin(), game.obstacles.end(), [](const Obstacle& obs) {
            return obs.position.y > static_cast<float>(WINDOW_HEIGHT) + 160.0f;
        }),
        game.obstacles.end()
    );
}

void updateBonuses(GameData& game, float dt) {
    const float bonusSpeed = game.currentRoadSpeed * game.speedMultiplier * 0.85f;
    for (BonusItem& bonus : game.bonuses) {
        bonus.position.y += bonusSpeed * dt;
        bonus.pulsePhase += dt * 4.5f;
    }

    game.bonuses.erase(
        std::remove_if(game.bonuses.begin(), game.bonuses.end(), [](const BonusItem& bonus) {
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

void checkCollisions(GameData& game, GameResources& res) {
    const sf::FloatRect playerHitbox = shrinkRect(sf::FloatRect(game.player.position, game.player.size), 7.0f);
    const bool immune = (game.player.invincibilityTimer > 0.0f) || (game.player.greenWaveTimer > 0.0f);

    // 1. Susidūrimai su kliūtimis
    if (!immune) {
        for (const Obstacle& obs : game.obstacles) {
            const float margin = (obs.type == ObstacleType::Scooter) ? 4.0f : 6.0f;
            const sf::FloatRect obsHitbox = shrinkRect(sf::FloatRect(obs.position, obs.size), margin);
            if (playerHitbox.findIntersection(obsHitbox).has_value()) {
                game.player.lives -= 1;
                game.player.invincibilityTimer = INVINCIBILITY_DURATION;
                game.screenFlashTimer = 0.35f;
                game.delaySeconds += 60.0f;
                game.cleanDriveTimer = 0.0f;

                const sf::Vector2f hitCenter = {
                    game.player.position.x + game.player.size.x * 0.5f,
                    game.player.position.y + 18.0f
                };
                spawnCrashParticles(game, hitCenter);
                triggerScreenShake(game, 0.35f, 9.0f);
                playSound(res, game, SoundId::Crash);

                if (obs.type == ObstacleType::ChaoticDriver) {
                    setStatusMessage(game, "BUM! Oho, čia tikrai bus 1 žvaigždutė! (+1 min)", 3.5f);
                } else if (obs.type == ObstacleType::Trolleybus) {
                    setStatusMessage(game, "BUM! Troleibusas visada turi pirmumą! (+1 min)", 3.5f);
                } else if (obs.type == ObstacleType::Scooter) {
                    setStatusMessage(game, "BUM! Paspirtukininkas išlindo iš niekur! (+1 min)", 3.5f);
                } else if (obs.type == ObstacleType::Roadworks) {
                    setStatusMessage(game, "BUM! Įvažiavai į kelio darbus! Atsargiau! (+1 min)", 3.5f);
                } else {
                    setStatusMessage(game, "BUM! Mano kava išsipylė ant skaidrių! (+1 min)", 3.5f);
                }

                if (game.player.lives <= 0) {
                    game.state = GameState::GameOver;
                    game.gameOverReason = GameOverReason::OutOfLives;
                    playSound(res, game, SoundId::GameOver);
                }
                break;
            }
        }
    }

    // 2. Bonusų surinkimas
    for (BonusItem& bonus : game.bonuses) {
        if (!bonus.active) {
            continue;
        }
        const sf::FloatRect bonusRect(
            {bonus.position.x - bonus.radius, bonus.position.y - bonus.radius},
            {bonus.radius * 2.0f, bonus.radius * 2.0f}
        );

        if (playerHitbox.findIntersection(bonusRect).has_value()) {
            bonus.active = false;

            if (bonus.type == BonusType::Coffee) {
                game.coffeesCollected += 1;
                game.score += 150;
                game.delaySeconds = std::max(0.0f, game.delaySeconds - 30.0f);
                spawnBonusParticles(game, bonus.position, sf::Color(250, 214, 92));
                playSound(res, game, SoundId::Coffee);

                if (game.player.lives < PLAYER_MAX_LIVES) {
                    game.player.lives += 1;
                    setStatusMessage(game, "O, karšta kava! Ačiū! (+1 kantrybės, -0.5 min)", 3.2f);
                } else {
                    setStatusMessage(game, "Puikus espresso! (+150 tšk. ir -0.5 min)", 3.0f);
                }
            } else if (bonus.type == BonusType::GreenWave) {
                game.player.greenWaveTimer = GREEN_WAVE_DURATION;
                game.score += 200;
                spawnBonusParticles(game, bonus.position, sf::Color(88, 240, 145));
                playSound(res, game, SoundId::Bonus);
                setStatusMessage(game, "ŽALIA BANGA! Turbo greitis ir apsauga 3.5 s!", 3.2f);
            } else if (bonus.type == BonusType::UsbDrive) {
                game.usbCollected += 1;
                game.score += 300;
                spawnBonusParticles(game, bonus.position, sf::Color(110, 195, 255));
                playSound(res, game, SoundId::Bonus);
                setStatusMessage(game, "O, radai mano USB su skaidrėmis! (+300 tšk.)", 3.2f);
            }
        }
    }
}

void updateGame(GameData& game, GameResources& res, float dt) {
    game.totalElapsed += dt;
    updateAudio(res, game);

    // Meniu ir papildomuose ekranuose fone švelniai juda kelias
    if (game.state == GameState::Menu ||
        game.state == GameState::RouteSelect ||
        game.state == GameState::HowToPlay ||
        game.state == GameState::HighScores) {
        game.roadOffset = std::fmod(game.roadOffset + 145.0f * dt, 90.0f);
        for (SceneryProp& prop : game.scenery) {
            prop.position.y += 145.0f * dt;
            const float height = prop.isBuilding ? prop.rectSize.y : prop.radius * 2.0f;
            if (prop.position.y > static_cast<float>(WINDOW_HEIGHT) + 80.0f) {
                prop.position.y = -height - 40.0f;
            }
        }
        return;
    }

    // 3-2-1 starto atskaita
    if (game.state == GameState::Countdown) {
        game.countdownTimer -= dt;
        const int currentSec = static_cast<int>(std::ceil(game.countdownTimer));
        if (currentSec < game.lastCountdownBeep && currentSec >= 1) {
            game.lastCountdownBeep = currentSec;
            playSound(res, game, SoundId::Countdown);
        }
        if (game.countdownTimer <= 0.0f) {
            game.state = GameState::Playing;
            playSound(res, game, SoundId::Go);
        }
        return;
    }

    if (game.state != GameState::Playing) {
        updateEffects(game, dt);
        return;
    }

    const RouteConfig route = getRouteConfig(game.selectedRoute);

    // 1. Progresas ir greičio didėjimas
    const float progress = std::clamp(game.distanceKm / route.targetDistanceKm, 0.0f, 1.0f);
    game.currentRoadSpeed = route.baseRoadSpeed + route.maxExtraSpeed * progress;

    // 2. Žaidėjas, kelias, aplinka ir dalelės
    updatePlayer(game, dt);
    const float effectiveRoadSpeed = game.currentRoadSpeed * game.speedMultiplier;
    game.roadOffset = std::fmod(game.roadOffset + effectiveRoadSpeed * dt, 90.0f);
    updateScenery(game, dt);
    updateEffects(game, dt);

    // 3. Atstumas, laikas ir švaraus vairavimo serija
    game.distanceKm += effectiveRoadSpeed * KM_PER_PIXEL * dt;
    game.delaySeconds += dt * DELAY_TIME_SCALE;
    game.cleanDriveTimer += dt;
    game.score += static_cast<int>(effectiveRoadSpeed * dt * 0.055f);

    // 4. Kliūčių ir bonusų generavimas
    game.obstacleSpawnTimer -= dt;
    if (game.obstacleSpawnTimer <= 0.0f) {
        spawnObstacle(game);
        const float currentMax = route.maxSpawnInterval - (route.maxSpawnInterval - route.minSpawnInterval) * 0.65f * progress;
        game.obstacleSpawnTimer = randomFloat(route.minSpawnInterval, std::max(route.minSpawnInterval + 0.08f, currentMax));
    }

    game.bonusSpawnTimer -= dt;
    if (game.bonusSpawnTimer <= 0.0f) {
        spawnBonus(game);
        game.bonusSpawnTimer = randomFloat(7.5f, 12.0f);
    }

    // 5. Objektų judėjimas ir susidūrimai
    updateObstacles(game, res, dt);
    updateBonuses(game, dt);
    checkCollisions(game, res);

    // 6. Kalbos burbulo laikmačiai
    if (game.statusMessageTimer > 0.0f) {
        game.statusMessageTimer = std::max(0.0f, game.statusMessageTimer - dt);
    }

    game.ambientMessageTimer -= dt;
    if (game.ambientMessageTimer <= 0.0f) {
        if (game.statusMessageTimer <= 0.0f) {
            setStatusMessage(game, getRandomAmbientMessage(game, route), 3.4f);
        }
        game.ambientMessageTimer = randomFloat(6.5f, 10.0f);
    }

    // 7. Pergalė arba pralaimėjimas
    if (game.state == GameState::Playing) {
        if (game.distanceKm >= route.targetDistanceKm) {
            game.distanceKm = route.targetDistanceKm;
            game.score += 500 + game.player.lives * 100;
            playSound(res, game, SoundId::Victory);
            game.state = GameState::NameInput; // Pereiname į vardo įvedimą rekordų lentelei
        } else if (game.delaySeconds >= route.maxDelayMinutes * 60.0f) {
            game.state = GameState::GameOver;
            game.gameOverReason = GameOverReason::TooLate;
            playSound(res, game, SoundId::GameOver);
        }
    }
}
