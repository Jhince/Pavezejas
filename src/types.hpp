#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// ============================================================================
// Pagrindinės žaidimo konstantos (lengva keisti gynimo metu!)
// ============================================================================
constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;

constexpr int LANE_COUNT = 4;
constexpr float LANE_WIDTH = 140.0f;
constexpr float ROAD_WIDTH = LANE_COUNT * LANE_WIDTH;
constexpr float ROAD_LEFT = (static_cast<float>(WINDOW_WIDTH) - ROAD_WIDTH) * 0.5f;
constexpr float ROAD_RIGHT = ROAD_LEFT + ROAD_WIDTH;

constexpr float PLAYER_WIDTH = 62.0f;
constexpr float PLAYER_HEIGHT = 110.0f;
constexpr float PLAYER_SIDE_SPEED = 420.0f;
constexpr float PLAYER_VERTICAL_SPEED = 290.0f;
constexpr int PLAYER_MAX_LIVES = 3;
constexpr float INVINCIBILITY_DURATION = 1.6f;

constexpr float BASE_ROAD_SPEED = 360.0f;      // Bazinis kelio judėjimo greitis (px/s)
constexpr float MAX_EXTRA_SPEED = 260.0f;      // Papildomas greitis augant sudėtingumui
constexpr float TARGET_DISTANCE_KM = 5.0f;     // Atstumas iki universiteto (km)
constexpr float MAX_DELAY_MINUTES = 10.0f;     // Po 10 min. vėlavimo paskaita atšaukiama
constexpr float DELAY_TIME_SCALE = 7.0f;       // 1 žaidimo sekundė = 7 „paskaitos vėlavimo“ sekundės

// ============================================================================
// Būsenos ir tipai (enum class)
// ============================================================================
enum class GameState {
    Menu,
    Playing,
    Paused,
    Victory,
    GameOver
};

enum class ObstacleType {
    NormalCar,      // Įprastas miesto automobilis
    ChaoticDriver,  // Skubantis vairuotojas, staigiai persirikiuojantis į gretimą juostą
    Roadworks       // Kelio darbai / užtvara
};

enum class GameOverReason {
    None,
    OutOfLives,
    TooLate
};

// ============================================================================
// Duomenų struktūros (griežtai procedūrinis stilius – tik duomenys)
// ============================================================================
struct Player {
    sf::Vector2f position;
    sf::Vector2f size;
    int lives = PLAYER_MAX_LIVES;
    float invincibilityTimer = 0.0f;
};

struct Obstacle {
    ObstacleType type = ObstacleType::NormalCar;
    sf::Vector2f position;
    sf::Vector2f size;
    float speed = 200.0f;
    sf::Color color = sf::Color::White;

    // Skubančio vairuotojo persirikiavimo duomenys
    int currentLane = 0;
    int targetLane = 0;
    bool willSwitchLane = false;
    bool isSignaling = false;
    bool isSwitching = false;
    float signalTimer = 0.0f;
    float triggerY = 0.0f;

    bool passedPlayer = false;
};

struct CoffeeBonus {
    sf::Vector2f position;
    float radius = 20.0f;
    bool active = true;
};

struct SceneryProp {
    sf::Vector2f position;
    float radius = 22.0f;
    bool isBuilding = false;
    sf::Vector2f rectSize;
    sf::Color color;
};

struct ButtonRect {
    sf::FloatRect bounds;
    std::string label;
};

struct GameData {
    GameState state = GameState::Menu;
    GameOverReason gameOverReason = GameOverReason::None;

    Player player;
    std::vector<Obstacle> obstacles;
    std::vector<CoffeeBonus> bonuses;
    std::vector<SceneryProp> scenery;

    // Žaidimo eigos rodikliai
    float roadOffset = 0.0f;
    float currentRoadSpeed = BASE_ROAD_SPEED;
    float speedMultiplier = 1.0f;      // Keičiamas W / S klavišais
    float distanceKm = 0.0f;
    float delaySeconds = 0.0f;         // Vėlavimas „paskaitos sekundėmis“
    int score = 0;
    int dodgedCount = 0;
    int coffeesCollected = 0;

    // Generavimo laikmačiai
    float obstacleSpawnTimer = 0.8f;
    float bonusSpawnTimer = 6.5f;
    float ambientMessageTimer = 5.0f;

    // Ekrano žinutė (GPS / bajeriai)
    std::string statusMessage;
    float statusMessageTimer = 0.0f;
    float screenFlashTimer = 0.0f;     // Trumpas raudonas blyksnis po susidūrimo
};
