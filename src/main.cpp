#include "game_logic.hpp"
#include "renderer.hpp"
#include "types.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <iostream>

int main() {
    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        toSfString("20 Minučių iki Paskaitos")
    );
    window.setFramerateLimit(60);

    // Įkeliame šriftą (su patikrinimu, kad išvengtume tylių klaidų)
    sf::Font font;
    if (!font.openFromFile("assets/fonts/CascadiaCode.ttf")) {
        std::cerr << "Klaida: nepavyko įkelti šrifto 'assets/fonts/CascadiaCode.ttf'!" << std::endl;
        return 1;
    }

    GameData game;
    initScenery(game);
    resetGame(game);
    game.state = GameState::Menu;

    sf::Clock frameClock;

    while (window.isOpen()) {
        // Apribojame dt (delta time), kad perkeliant langą objektai „neperšoktų“
        const float dt = std::min(frameClock.restart().asSeconds(), 0.05f);

        handleEvents(window, game);
        updateGame(game, dt);
        renderGame(window, game, font);
    }

    return 0;
}
