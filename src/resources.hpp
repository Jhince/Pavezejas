#pragma once

#include "types.hpp"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <deque>
#include <vector>

struct GameResources {
    sf::Font font;

    // Tekstūros (jei kurios nors nepavyktų įkelti – turime atsarginį piešimą)
    bool hasPlayerCar = false;
    sf::Texture playerCar;

    std::vector<sf::Texture> trafficCars;

    bool hasSportCar = false;
    sf::Texture sportCar;

    bool hasTrolleybus = false;
    sf::Texture trolleybus;

    bool hasScooter = false;
    sf::Texture scooter;

    bool hasRoadworks = false;
    sf::Texture roadworks;

    bool hasCoffee = false;
    sf::Texture coffee;

    bool hasGreenWave = false;
    sf::Texture greenWave;

    bool hasUsb = false;
    sf::Texture usb;

    bool hasArtasFaces = false;
    sf::Texture artasHappy;
    sf::Texture artasNeutral;
    sf::Texture artasAngry;

    // Garsai ir muzika
    bool hasClickSound = false;
    sf::SoundBuffer clickBuffer;

    bool hasCountdownSound = false;
    sf::SoundBuffer countdownBuffer;

    bool hasGoSound = false;
    sf::SoundBuffer goBuffer;

    bool hasCoffeeSound = false;
    sf::SoundBuffer coffeeBuffer;

    bool hasBonusSound = false;
    sf::SoundBuffer bonusBuffer;

    bool hasHornSound = false;
    sf::SoundBuffer hornBuffer;

    bool hasCrashSound = false;
    sf::SoundBuffer crashBuffer;

    bool hasVictorySound = false;
    sf::SoundBuffer victoryBuffer;

    bool hasGameOverSound = false;
    sf::SoundBuffer gameOverBuffer;

    bool hasMusic = false;
    sf::Music music;

    // Aktyvūs vienkartiniai garsai (std::deque neiškelia elementų atmintyje)
    std::deque<sf::Sound> activeSounds;
};

// Įkelia šriftą, tekstūras ir garsus; grąžina false tik jei nepavyko įkelti šrifto
bool loadResources(GameResources& res);

// Groja pasirinktą garso efektą (jei garsas neišjungtas)
void playSound(GameResources& res, const GameData& game, SoundId id);

// Atnaujina foninės muzikos būseną ir išvalo pasibaigusius garsus
void updateAudio(GameResources& res, const GameData& game);
