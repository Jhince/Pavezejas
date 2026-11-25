#include "resources.hpp"

#include <iostream>

namespace {

bool loadTextureSafe(sf::Texture& tex, const std::string& path) {
    if (!tex.loadFromFile(path)) {
        std::cerr << "Įspėjimas: nepavyko įkelti tekstūros '" << path
                  << "', bus naudojamas atsarginis piešimas." << std::endl;
        return false;
    }
    tex.setSmooth(true);
    return true;
}

bool loadSoundSafe(sf::SoundBuffer& buf, const std::string& path) {
    if (!buf.loadFromFile(path)) {
        std::cerr << "Įspėjimas: nepavyko įkelti garso '" << path << "'." << std::endl;
        return false;
    }
    return true;
}

} // namespace

bool loadResources(GameResources& res) {
    // Šriftas yra būtinas sąsajai
    if (!res.font.openFromFile("assets/fonts/CascadiaCode.ttf")) {
        std::cerr << "Klaida: nepavyko įkelti šrifto 'assets/fonts/CascadiaCode.ttf'!" << std::endl;
        return false;
    }

    // Tekstūros (paveikslėliai)
    res.hasPlayerCar = loadTextureSafe(res.playerCar, "assets/textures/player_car.png");
    res.hasSportCar = loadTextureSafe(res.sportCar, "assets/textures/car_sport.png");
    res.hasTrolleybus = loadTextureSafe(res.trolleybus, "assets/textures/trolleybus.png");
    res.hasScooter = loadTextureSafe(res.scooter, "assets/textures/scooter.png");
    res.hasRoadworks = loadTextureSafe(res.roadworks, "assets/textures/roadworks.png");
    res.hasCoffee = loadTextureSafe(res.coffee, "assets/textures/bonus_coffee.png");
    res.hasGreenWave = loadTextureSafe(res.greenWave, "assets/textures/bonus_greenwave.png");
    res.hasUsb = loadTextureSafe(res.usb, "assets/textures/bonus_usb.png");

    const std::vector<std::string> carPaths = {
        "assets/textures/car_red.png",
        "assets/textures/car_silver.png",
        "assets/textures/car_blue.png",
        "assets/textures/car_yellow.png",
        "assets/textures/car_purple.png"
    };
    res.trafficCars.clear();
    for (const std::string& path : carPaths) {
        sf::Texture tex;
        if (loadTextureSafe(tex, path)) {
            res.trafficCars.push_back(std::move(tex));
        }
    }

    const bool h1 = loadTextureSafe(res.artasHappy, "assets/textures/artas_happy.png");
    const bool h2 = loadTextureSafe(res.artasNeutral, "assets/textures/artas_neutral.png");
    const bool h3 = loadTextureSafe(res.artasAngry, "assets/textures/artas_angry.png");
    res.hasArtasFaces = h1 && h2 && h3;

    // Garso efektai
    res.hasClickSound = loadSoundSafe(res.clickBuffer, "assets/audio/click.wav");
    res.hasCountdownSound = loadSoundSafe(res.countdownBuffer, "assets/audio/countdown.wav");
    res.hasGoSound = loadSoundSafe(res.goBuffer, "assets/audio/go.wav");
    res.hasCoffeeSound = loadSoundSafe(res.coffeeBuffer, "assets/audio/coffee.wav");
    res.hasBonusSound = loadSoundSafe(res.bonusBuffer, "assets/audio/bonus.wav");
    res.hasHornSound = loadSoundSafe(res.hornBuffer, "assets/audio/horn.wav");
    res.hasCrashSound = loadSoundSafe(res.crashBuffer, "assets/audio/crash.wav");
    res.hasVictorySound = loadSoundSafe(res.victoryBuffer, "assets/audio/victory.wav");
    res.hasGameOverSound = loadSoundSafe(res.gameOverBuffer, "assets/audio/gameover.wav");

    // Foninė muzika
    if (res.music.openFromFile("assets/audio/music.wav")) {
        res.hasMusic = true;
        res.music.setLooping(true);
        res.music.setVolume(24.0f);
    } else {
        std::cerr << "Įspėjimas: nepavyko įkelti muzikos 'assets/audio/music.wav'." << std::endl;
    }

    return true;
}

void playSound(GameResources& res, const GameData& game, SoundId id) {
    if (game.soundMuted) {
        return;
    }

    const sf::SoundBuffer* buffer = nullptr;
    float volume = 65.0f;

    switch (id) {
        case SoundId::Click:
            if (res.hasClickSound) { buffer = &res.clickBuffer; volume = 50.0f; }
            break;
        case SoundId::Countdown:
            if (res.hasCountdownSound) { buffer = &res.countdownBuffer; volume = 60.0f; }
            break;
        case SoundId::Go:
            if (res.hasGoSound) { buffer = &res.goBuffer; volume = 70.0f; }
            break;
        case SoundId::Coffee:
            if (res.hasCoffeeSound) { buffer = &res.coffeeBuffer; volume = 65.0f; }
            break;
        case SoundId::Bonus:
            if (res.hasBonusSound) { buffer = &res.bonusBuffer; volume = 70.0f; }
            break;
        case SoundId::Horn:
            if (res.hasHornSound) { buffer = &res.hornBuffer; volume = 48.0f; }
            break;
        case SoundId::Crash:
            if (res.hasCrashSound) { buffer = &res.crashBuffer; volume = 80.0f; }
            break;
        case SoundId::Victory:
            if (res.hasVictorySound) { buffer = &res.victoryBuffer; volume = 72.0f; }
            break;
        case SoundId::GameOver:
            if (res.hasGameOverSound) { buffer = &res.gameOverBuffer; volume = 70.0f; }
            break;
    }

    if (buffer == nullptr) {
        return;
    }

    // Neleidžiame prikaupti per daug vienu metu grojančių kanalų
    if (res.activeSounds.size() >= 16) {
        res.activeSounds.pop_front();
    }

    res.activeSounds.emplace_back(*buffer);
    res.activeSounds.back().setVolume(volume);
    res.activeSounds.back().play();
}

void updateAudio(GameResources& res, const GameData& game) {
    while (!res.activeSounds.empty() && res.activeSounds.front().getStatus() == sf::Sound::Status::Stopped) {
        res.activeSounds.pop_front();
    }

    if (!res.hasMusic) {
        return;
    }

    const bool shouldPlayMusic = !game.soundMuted &&
        (game.state == GameState::Playing || game.state == GameState::Countdown);

    if (shouldPlayMusic) {
        if (res.music.getStatus() != sf::SoundSource::Status::Playing) {
            res.music.play();
        }
    } else {
        if (res.music.getStatus() == sf::SoundSource::Status::Playing) {
            res.music.pause();
        }
    }
}
