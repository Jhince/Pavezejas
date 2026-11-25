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
constexpr float PLAYER_SIDE_SPEED = 435.0f;
constexpr float PLAYER_VERTICAL_SPEED = 295.0f;
constexpr int PLAYER_MAX_LIVES = 3;
constexpr float INVINCIBILITY_DURATION = 1.6f;
constexpr float GREEN_WAVE_DURATION = 3.5f;

constexpr float KM_PER_PIXEL = 0.000155f;
constexpr float DELAY_TIME_SCALE = 7.0f;       // 1 žaidimo s = 7 paskaitos vėlavimo s
constexpr int ROUTE_COUNT = 3;
constexpr std::size_t MAX_NAME_LENGTH = 12;
constexpr int MAX_SCORES_PER_ROUTE = 5;

// ============================================================================
// Būsenos ir tipai (enum class)
// ============================================================================
enum class GameState {
    Menu,
    RouteSelect,
    HowToPlay,
    HighScores,
    Countdown,
    Playing,
    Paused,
    NameInput,
    Victory,
    GameOver
};

enum class ObstacleType {
    NormalCar,      // Įprastas miesto automobilis
    ChaoticDriver,  // Sportinis automobilis, staigiai keičiantis juostas
    Roadworks,      // Kelio darbai / užtvara
    Scooter,        // Paspirtukininkas – siauras, greitas, vingiuoja
    Trolleybus      // Vilniaus troleibusas – ilgas ir lėtas
};

enum class BonusType {
    Coffee,         // +1 kantrybės (arba +150 tšk.) ir -0.5 min vėlavimo
    GreenWave,      // 3.5 s turbo greitis + imunitetas
    UsbDrive        // Dėstytojo skaidrės: +300 tšk.
};

enum class GameOverReason {
    None,
    OutOfLives,
    TooLate
};

enum class SoundId {
    Click,
    Countdown,
    Go,
    Coffee,
    Bonus,
    Horn,
    Crash,
    Victory,
    GameOver
};

// ============================================================================
// Duomenų struktūros (griežtai procedūrinis stilius – tik duomenys)
// ============================================================================
struct RouteConfig {
    int id = 0;
    std::string name;
    std::string difficulty;
    std::string description;
    float targetDistanceKm = 5.0f;
    float maxDelayMinutes = 10.0f;
    float baseRoadSpeed = 360.0f;
    float maxExtraSpeed = 260.0f;
    float minSpawnInterval = 0.44f;
    float maxSpawnInterval = 1.05f;
    int chaoticChance = 30;
    int trolleyChance = 15;
    int scooterChance = 15;
    int roadworksChance = 12;
};

struct Player {
    sf::Vector2f position;
    sf::Vector2f size;
    int lives = PLAYER_MAX_LIVES;
    float invincibilityTimer = 0.0f;
    float greenWaveTimer = 0.0f;
};

struct Obstacle {
    ObstacleType type = ObstacleType::NormalCar;
    sf::Vector2f position;
    sf::Vector2f size;
    float speed = 200.0f;
    sf::Color color = sf::Color::White;
    int textureIndex = 0;

    // Skubančio vairuotojo persirikiavimo duomenys
    int currentLane = 0;
    int targetLane = 0;
    bool willSwitchLane = false;
    bool isSignaling = false;
    bool isSwitching = false;
    float signalTimer = 0.0f;
    float triggerY = 0.0f;

    // Paspirtukininko vingiavimo duomenys
    float weaveBaseX = 0.0f;
    float weavePhase = 0.0f;

    bool passedPlayer = false;
};

struct BonusItem {
    BonusType type = BonusType::Coffee;
    sf::Vector2f position;
    float radius = 22.0f;
    bool active = true;
    float pulsePhase = 0.0f;
};

struct SceneryProp {
    sf::Vector2f position;
    float radius = 22.0f;
    bool isBuilding = false;
    sf::Vector2f rectSize;
    sf::Color color;
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float size = 5.0f;
    sf::Color color = sf::Color::White;
    float lifetime = 0.0f;
    float maxLifetime = 0.5f;
};

struct HighScoreEntry {
    std::string playerName;
    int routeIndex = 0;
    int score = 0;
    float delaySeconds = 0.0f;
    int stars = 3;
};

struct ButtonRect {
    sf::FloatRect bounds;
    std::string label;
    int actionId = 0;
    sf::Color baseColor = sf::Color(46, 125, 190);
};

struct GameData {
    GameState state = GameState::Menu;
    GameOverReason gameOverReason = GameOverReason::None;

    int selectedRoute = 1;           // Numatytasis: Centras -> Saulėtekis (5 km, 10 min)
    int highScoreTab = 1;            // Kurio maršruto rekordai rodomi

    Player player;
    std::vector<Obstacle> obstacles;
    std::vector<BonusItem> bonuses;
    std::vector<SceneryProp> scenery;
    std::vector<Particle> particles;
    std::vector<HighScoreEntry> highScores;

    // Žaidimo eigos rodikliai
    float roadOffset = 0.0f;
    float currentRoadSpeed = 360.0f;
    float speedMultiplier = 1.0f;
    float distanceKm = 0.0f;
    float delaySeconds = 0.0f;
    float totalElapsed = 0.0f;       // Bendras animacijų laikmatis
    int score = 0;
    int dodgedCount = 0;
    int coffeesCollected = 0;
    int usbCollected = 0;
    float cleanDriveTimer = 0.0f;    // Kiek laiko važiuojama be avarijų

    // 3-2-1 starto atskaita
    float countdownTimer = 0.0f;
    int lastCountdownBeep = 4;

    // Generavimo laikmačiai
    float obstacleSpawnTimer = 0.8f;
    float bonusSpawnTimer = 6.5f;
    float ambientMessageTimer = 5.0f;

    // Dėstytojo kalbos burbulas ir vizualiniai efektai
    std::string statusMessage;
    float statusMessageTimer = 0.0f;
    float screenFlashTimer = 0.0f;
    float screenShakeTimer = 0.0f;
    float screenShakeIntensity = 0.0f;

    // Vardo įvedimas po pergalės
    std::string playerNameInput;
    std::string nameValidationError;
    bool scoreSavedThisRun = false;

    // Garso nustatymai
    bool soundMuted = false;
};
