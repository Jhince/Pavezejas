#include "game_logic.hpp"
#include "renderer.hpp"
#include "resources.hpp"
#include "storage.hpp"
#include "types.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>

int main() {
    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        toSfString("20 Minučių iki Paskaitos")
    );
    window.setFramerateLimit(60);

    // Įkeliame šriftą, paveikslėlius ir garsus (su saugiu klaidų apdorojimu)
    GameResources resources;
    if (!loadResources(resources)) {
        return 1;
    }

    GameData game;
    initScenery(game);
    game.highScores = loadHighScores("highscores.txt");
    game.state = GameState::Menu;

    sf::Clock frameClock;

    while (window.isOpen()) {
        // Apribojame dt (delta time), kad perkeliant langą objektai „neperšoktų“
        const float dt = std::min(frameClock.restart().asSeconds(), 0.05f);

        handleEvents(window, game, resources);
        updateGame(game, resources, dt);
        renderGame(window, game, resources);
    }

    return 0;
}
