#include "renderer.hpp"
#include "game_logic.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace {

std::string formatDelayTime(float totalSeconds) {
    const int secs = std::max(0, static_cast<int>(totalSeconds));
    const int mins = secs / 60;
    const int remSecs = secs % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << mins << " min "
        << std::setfill('0') << std::setw(2) << remSecs << " s";
    return oss.str();
}

std::string formatDistance(float km) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << km << " / " << std::setprecision(1) << TARGET_DISTANCE_KM << " km";
    return oss.str();
}

sf::Color getDelayColor(float delaySeconds) {
    const float mins = delaySeconds / 60.0f;
    if (mins < 5.0f) {
        return sf::Color(88, 214, 141);
    }
    if (mins < 7.5f) {
        return sf::Color(244, 208, 63);
    }
    if (mins < 9.0f) {
        return sf::Color(235, 152, 78);
    }
    return sf::Color(231, 76, 60);
}

void drawButton(sf::RenderWindow& window,
                const sf::Font& font,
                const ButtonRect& button,
                sf::Vector2f mousePos,
                sf::Color baseColor) {
    const bool hovered = button.bounds.contains(mousePos);

    sf::RectangleShape rect(button.bounds.size);
    rect.setPosition(button.bounds.position);
    rect.setFillColor(hovered
        ? sf::Color(
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(baseColor.r) + 28)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(baseColor.g) + 28)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(baseColor.b) + 28))
        )
        : baseColor);
    rect.setOutlineThickness(hovered ? 3.0f : 2.0f);
    rect.setOutlineColor(hovered ? sf::Color(255, 255, 255) : sf::Color(210, 220, 235, 180));
    window.draw(rect);

    const sf::Vector2f center = {
        button.bounds.position.x + button.bounds.size.x * 0.5f,
        button.bounds.position.y + button.bounds.size.y * 0.5f
    };
    drawCenteredText(window, font, button.label, 20, sf::Color::White, center);
}

void drawCar(sf::RenderWindow& window,
             sf::Vector2f pos,
             sf::Vector2f size,
             sf::Color bodyColor,
             bool isPlayer,
             bool isChaoticDriver,
             bool blinkLeft,
             bool blinkRight,
             std::uint8_t alpha) {
    auto withAlpha = [alpha](sf::Color c) {
        c.a = alpha;
        return c;
    };

    // 4 ratai
    const sf::Color wheelColor = withAlpha(sf::Color(25, 28, 34));
    const float wheelW = 8.0f;
    const float wheelH = 22.0f;
    const sf::Vector2f wheelOffsets[4] = {
        {-4.0f, 14.0f},
        {size.x - wheelW + 4.0f, 14.0f},
        {-4.0f, size.y - wheelH - 14.0f},
        {size.x - wheelW + 4.0f, size.y - wheelH - 14.0f}
    };
    for (const auto& offset : wheelOffsets) {
        sf::RectangleShape wheel({wheelW, wheelH});
        wheel.setPosition({pos.x + offset.x, pos.y + offset.y});
        wheel.setFillColor(wheelColor);
        window.draw(wheel);
    }

    // Kėbulas
    sf::RectangleShape body(size);
    body.setPosition(pos);
    body.setFillColor(withAlpha(bodyColor));
    body.setOutlineThickness(2.0f);
    body.setOutlineColor(withAlpha(sf::Color(20, 24, 30)));
    window.draw(body);

    // Priekinis ir galinis stiklai
    sf::RectangleShape frontGlass({size.x - 14.0f, 20.0f});
    frontGlass.setPosition({pos.x + 7.0f, pos.y + 18.0f});
    frontGlass.setFillColor(withAlpha(sf::Color(40, 62, 82)));
    window.draw(frontGlass);

    sf::RectangleShape rearGlass({size.x - 16.0f, 14.0f});
    rearGlass.setPosition({pos.x + 8.0f, pos.y + size.y - 28.0f});
    rearGlass.setFillColor(withAlpha(sf::Color(35, 52, 70)));
    window.draw(rearGlass);

    // Stogas
    sf::RectangleShape roof({size.x - 18.0f, size.y * 0.34f});
    roof.setPosition({pos.x + 9.0f, pos.y + 40.0f});
    roof.setFillColor(withAlpha(sf::Color(
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(bodyColor.r) + 18)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(bodyColor.g) + 18)),
        static_cast<std::uint8_t>(std::min(255, static_cast<int>(bodyColor.b) + 18))
    )));
    window.draw(roof);

    // Žaidėjo (pavežėjo) baltas ženkliukas ant stogo
    if (isPlayer) {
        sf::RectangleShape badge({26.0f, 11.0f});
        badge.setPosition({pos.x + (size.x - 26.0f) * 0.5f, pos.y + 50.0f});
        badge.setFillColor(withAlpha(sf::Color(245, 255, 245)));
        badge.setOutlineThickness(1.5f);
        badge.setOutlineColor(withAlpha(sf::Color(20, 110, 55)));
        window.draw(badge);
    }

    // Skubančio chaotiško vairuotojo sportinės juostos
    if (isChaoticDriver) {
        sf::RectangleShape stripe({8.0f, size.y - 12.0f});
        stripe.setPosition({pos.x + (size.x - 8.0f) * 0.5f, pos.y + 6.0f});
        stripe.setFillColor(withAlpha(sf::Color(231, 76, 60, 170)));
        window.draw(stripe);
    }

    // Priekiniai žibintai (geltoni/balti)
    sf::RectangleShape leftHeadlight({12.0f, 6.0f});
    leftHeadlight.setPosition({pos.x + 6.0f, pos.y + 2.0f});
    leftHeadlight.setFillColor(withAlpha(sf::Color(255, 248, 196)));
    window.draw(leftHeadlight);

    sf::RectangleShape rightHeadlight({12.0f, 6.0f});
    rightHeadlight.setPosition({pos.x + size.x - 18.0f, pos.y + 2.0f});
    rightHeadlight.setFillColor(withAlpha(sf::Color(255, 248, 196)));
    window.draw(rightHeadlight);

    // Galiniai žibintai (raudoni)
    sf::RectangleShape leftTail({12.0f, 6.0f});
    leftTail.setPosition({pos.x + 6.0f, pos.y + size.y - 8.0f});
    leftTail.setFillColor(withAlpha(sf::Color(231, 76, 60)));
    window.draw(leftTail);

    sf::RectangleShape rightTail({12.0f, 6.0f});
    rightTail.setPosition({pos.x + size.x - 18.0f, pos.y + size.y - 8.0f});
    rightTail.setFillColor(withAlpha(sf::Color(231, 76, 60)));
    window.draw(rightTail);

    // Mirksintys oranžiniai posūkio signalai (įspėjimas prieš persirikiavimą!)
    if (blinkLeft) {
        sf::CircleShape blinker(7.0f);
        blinker.setPosition({pos.x - 4.0f, pos.y + 6.0f});
        blinker.setFillColor(sf::Color(255, 165, 0));
        window.draw(blinker);

        sf::CircleShape blinkerRear(7.0f);
        blinkerRear.setPosition({pos.x - 4.0f, pos.y + size.y - 18.0f});
        blinkerRear.setFillColor(sf::Color(255, 165, 0));
        window.draw(blinkerRear);
    }
    if (blinkRight) {
        sf::CircleShape blinker(7.0f);
        blinker.setPosition({pos.x + size.x - 10.0f, pos.y + 6.0f});
        blinker.setFillColor(sf::Color(255, 165, 0));
        window.draw(blinker);

        sf::CircleShape blinkerRear(7.0f);
        blinkerRear.setPosition({pos.x + size.x - 10.0f, pos.y + size.y - 18.0f});
        blinkerRear.setFillColor(sf::Color(255, 165, 0));
        window.draw(blinkerRear);
    }
}

void drawRoadworks(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size) {
    // Užtvaros pagrindas
    sf::RectangleShape barrier({size.x, 22.0f});
    barrier.setPosition({pos.x, pos.y + 12.0f});
    barrier.setFillColor(sf::Color(235, 110, 20));
    barrier.setOutlineThickness(2.0f);
    barrier.setOutlineColor(sf::Color(40, 25, 15));
    window.draw(barrier);

    // Baltos įspėjamosios juostos
    for (int i = 0; i < 4; ++i) {
        sf::RectangleShape stripe({10.0f, 20.0f});
        stripe.setPosition({pos.x + 8.0f + static_cast<float>(i) * 21.0f, pos.y + 13.0f});
        stripe.setFillColor(sf::Color(245, 245, 245));
        window.draw(stripe);
    }

    // Du kelio kūgiai šonuose
    for (int i = 0; i < 2; ++i) {
        sf::CircleShape cone(9.0f);
        const float cx = (i == 0) ? (pos.x + 6.0f) : (pos.x + size.x - 24.0f);
        cone.setPosition({cx, pos.y + size.y - 16.0f});
        cone.setFillColor(sf::Color(255, 120, 20));
        cone.setOutlineThickness(2.0f);
        cone.setOutlineColor(sf::Color::White);
        window.draw(cone);
    }
}

void drawCoffeeBonus(sf::RenderWindow& window, const CoffeeBonus& bonus) {
    // Auksinis švytėjimas aplink kavos puodelį
    sf::CircleShape glow(bonus.radius + 5.0f);
    glow.setOrigin({bonus.radius + 5.0f, bonus.radius + 5.0f});
    glow.setPosition(bonus.position);
    glow.setFillColor(sf::Color(241, 196, 15, 75));
    window.draw(glow);

    sf::CircleShape badge(bonus.radius);
    badge.setOrigin({bonus.radius, bonus.radius});
    badge.setPosition(bonus.position);
    badge.setFillColor(sf::Color(110, 62, 32));
    badge.setOutlineThickness(2.5f);
    badge.setOutlineColor(sf::Color(248, 214, 96));
    window.draw(badge);

    // Puodelis viduje
    sf::RectangleShape cup({18.0f, 16.0f});
    cup.setOrigin({9.0f, 8.0f});
    cup.setPosition({bonus.position.x, bonus.position.y + 2.0f});
    cup.setFillColor(sf::Color(248, 244, 236));
    window.draw(cup);

    sf::RectangleShape lid({22.0f, 5.0f});
    lid.setOrigin({11.0f, 2.5f});
    lid.setPosition({bonus.position.x, bonus.position.y - 7.0f});
    lid.setFillColor(sf::Color(65, 40, 24));
    window.draw(lid);
}

void drawPanelBox(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size) {
    sf::RectangleShape panel(size);
    panel.setPosition(pos);
    panel.setFillColor(sf::Color(18, 24, 34, 225));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(62, 78, 102));
    window.draw(panel);
}

std::string wrapTextToWidth(const sf::Font& font, const std::string& text, unsigned int fontSize, float maxWidth) {
    std::istringstream words(text);
    std::string word;
    std::string result;
    std::string currentLine;

    sf::Text measure(font, sf::String(), fontSize);
    while (words >> word) {
        const std::string candidate = currentLine.empty() ? word : (currentLine + " " + word);
        measure.setString(toSfString(candidate));
        if (!currentLine.empty() && measure.getLocalBounds().size.x > maxWidth) {
            if (!result.empty()) {
                result += "\n";
            }
            result += currentLine;
            currentLine = word;
        } else {
            currentLine = candidate;
        }
    }
    if (!currentLine.empty()) {
        if (!result.empty()) {
            result += "\n";
        }
        result += currentLine;
    }
    return result;
}

void drawCarSpeechBubble(sf::RenderWindow& window, const GameData& game, const sf::Font& font) {
    if (game.statusMessageTimer <= 0.0f || game.statusMessage.empty()) {
        return;
    }

    const float fadeRatio = std::clamp(game.statusMessageTimer / 0.35f, 0.0f, 1.0f);
    const std::uint8_t bgAlpha = static_cast<std::uint8_t>(240.0f * fadeRatio);
    const std::uint8_t fgAlpha = static_cast<std::uint8_t>(255.0f * fadeRatio);

    const std::string wrapped = wrapTextToWidth(font, game.statusMessage, 14, 225.0f);

    sf::Text speakerText(font, toSfString("Artas:"), 14);
    speakerText.setStyle(sf::Text::Bold);
    speakerText.setFillColor(sf::Color(22, 118, 66, fgAlpha));

    sf::Text bodyText(font, toSfString(wrapped), 14);
    bodyText.setFillColor(sf::Color(22, 28, 38, fgAlpha));

    const sf::FloatRect speakerBounds = speakerText.getLocalBounds();
    const sf::FloatRect bodyBounds = bodyText.getLocalBounds();

    const float padX = 14.0f;
    const float padY = 10.0f;
    const float gapY = 5.0f;
    const float contentW = std::max(speakerBounds.size.x, bodyBounds.size.x);
    const float bubbleW = std::max(145.0f, contentW + padX * 2.0f);
    const float bubbleH = speakerBounds.size.y + gapY + bodyBounds.size.y + padY * 2.0f + 6.0f;

    // Parenkame pusę šalia automobilio (išorinėje kelio pusėje, kad neužstotų eismo)
    const float carCenterX = game.player.position.x + game.player.size.x * 0.5f;
    const float carBackSeatY = game.player.position.y + game.player.size.y * 0.62f;

    bool placeLeft = (carCenterX < static_cast<float>(WINDOW_WIDTH) * 0.5f);
    const float tailGap = 16.0f;

    float bubbleX = placeLeft
        ? (game.player.position.x - tailGap - bubbleW)
        : (game.player.position.x + game.player.size.x + tailGap);
    float bubbleY = carBackSeatY - bubbleH * 0.55f;

    // Jei automobilis aukštai ir burbulas liestų šoninę HUD kortelę – perkeliame į kitą pusę
    if (placeLeft && bubbleX < 330.0f && bubbleY < 285.0f) {
        placeLeft = false;
        bubbleX = game.player.position.x + game.player.size.x + tailGap;
    } else if (!placeLeft && (bubbleX + bubbleW) > 950.0f && bubbleY < 318.0f) {
        placeLeft = true;
        bubbleX = game.player.position.x - tailGap - bubbleW;
    }

    bubbleX = std::clamp(bubbleX, 16.0f, static_cast<float>(WINDOW_WIDTH) - bubbleW - 16.0f);
    bubbleY = std::clamp(bubbleY, 16.0f, static_cast<float>(WINDOW_HEIGHT) - bubbleH - 16.0f);

    // Kalbos burbulo uodegėlė, nukreipta į galinę automobilio sėdynę
    sf::ConvexShape tail;
    tail.setPointCount(3);
    const float tailMidY = std::clamp(carBackSeatY, bubbleY + 14.0f, bubbleY + bubbleH - 14.0f);
    if (placeLeft) {
        tail.setPoint(0, {bubbleX + bubbleW - 1.0f, tailMidY - 8.0f});
        tail.setPoint(1, {bubbleX + bubbleW - 1.0f, tailMidY + 8.0f});
        tail.setPoint(2, {game.player.position.x - 2.0f, carBackSeatY});
    } else {
        tail.setPoint(0, {bubbleX + 1.0f, tailMidY - 8.0f});
        tail.setPoint(1, {bubbleX + 1.0f, tailMidY + 8.0f});
        tail.setPoint(2, {game.player.position.x + game.player.size.x + 2.0f, carBackSeatY});
    }
    tail.setFillColor(sf::Color(250, 252, 255, bgAlpha));
    tail.setOutlineThickness(2.0f);
    tail.setOutlineColor(sf::Color(28, 38, 52, bgAlpha));

    // Pagrindinis burbulo stačiakampis
    sf::RectangleShape bubble({bubbleW, bubbleH});
    bubble.setPosition({bubbleX, bubbleY});
    bubble.setFillColor(sf::Color(250, 252, 255, bgAlpha));
    bubble.setOutlineThickness(2.0f);
    bubble.setOutlineColor(sf::Color(28, 38, 52, bgAlpha));

    window.draw(tail);
    window.draw(bubble);

    // Užmaskuojame vidinę uodegėlės kraštinę, kad atrodytų vientisai
    sf::RectangleShape seamCover({4.0f, 14.0f});
    seamCover.setPosition({placeLeft ? (bubbleX + bubbleW - 3.0f) : (bubbleX - 1.0f), tailMidY - 7.0f});
    seamCover.setFillColor(sf::Color(250, 252, 255, bgAlpha));
    window.draw(seamCover);

    speakerText.setPosition({bubbleX + padX - speakerBounds.position.x,
                             bubbleY + padY - speakerBounds.position.y});
    bodyText.setPosition({bubbleX + padX - bodyBounds.position.x,
                          bubbleY + padY + speakerBounds.size.y + gapY - bodyBounds.position.y});

    window.draw(speakerText);
    window.draw(bodyText);
}

} // namespace

sf::String toSfString(const std::string& utf8Text) {
    return sf::String::fromUtf8(utf8Text.begin(), utf8Text.end());
}

void drawCenteredText(sf::RenderWindow& window,
                      const sf::Font& font,
                      const std::string& text,
                      unsigned int fontSize,
                      sf::Color color,
                      sf::Vector2f centerPos) {
    sf::Text sfText(font, toSfString(text), fontSize);
    sfText.setFillColor(color);
    const sf::FloatRect bounds = sfText.getLocalBounds();
    sfText.setOrigin({bounds.position.x + bounds.size.x * 0.5f,
                      bounds.position.y + bounds.size.y * 0.5f});
    sfText.setPosition(centerPos);
    window.draw(sfText);
}

void renderRoad(sf::RenderWindow& window, const GameData& game) {
    // 1. Žalia pakelė (fonas)
    sf::RectangleShape grass({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    grass.setFillColor(sf::Color(38, 76, 48));
    window.draw(grass);

    // 2. Judantys pastatai ir medžiai šonuose
    for (const SceneryProp& prop : game.scenery) {
        if (prop.isBuilding) {
            sf::RectangleShape building(prop.rectSize);
            building.setPosition(prop.position);
            building.setFillColor(prop.color);
            building.setOutlineThickness(2.0f);
            building.setOutlineColor(sf::Color(30, 36, 48));
            window.draw(building);
        } else {
            sf::CircleShape tree(prop.radius);
            tree.setOrigin({prop.radius, prop.radius});
            tree.setPosition(prop.position);
            tree.setFillColor(prop.color);
            tree.setOutlineThickness(2.0f);
            tree.setOutlineColor(sf::Color(24, 65, 32));
            window.draw(tree);
        }
    }

    // 3. Šaligatviai
    const float sidewalkW = 24.0f;
    sf::RectangleShape leftSidewalk({sidewalkW, static_cast<float>(WINDOW_HEIGHT)});
    leftSidewalk.setPosition({ROAD_LEFT - sidewalkW, 0.0f});
    leftSidewalk.setFillColor(sf::Color(115, 122, 130));
    window.draw(leftSidewalk);

    sf::RectangleShape rightSidewalk({sidewalkW, static_cast<float>(WINDOW_HEIGHT)});
    rightSidewalk.setPosition({ROAD_RIGHT, 0.0f});
    rightSidewalk.setFillColor(sf::Color(115, 122, 130));
    window.draw(rightSidewalk);

    // 4. Asfaltas
    sf::RectangleShape asphalt({ROAD_WIDTH, static_cast<float>(WINDOW_HEIGHT)});
    asphalt.setPosition({ROAD_LEFT, 0.0f});
    asphalt.setFillColor(sf::Color(52, 56, 64));
    window.draw(asphalt);

    // 5. Kraštinės geltonos linijos
    sf::RectangleShape leftLine({5.0f, static_cast<float>(WINDOW_HEIGHT)});
    leftLine.setPosition({ROAD_LEFT + 6.0f, 0.0f});
    leftLine.setFillColor(sf::Color(236, 188, 52));
    window.draw(leftLine);

    sf::RectangleShape rightLine({5.0f, static_cast<float>(WINDOW_HEIGHT)});
    rightLine.setPosition({ROAD_RIGHT - 11.0f, 0.0f});
    rightLine.setFillColor(sf::Color(236, 188, 52));
    window.draw(rightLine);

    // 6. Punktyrinės juostų linijos (juda žemyn)
    for (int lane = 1; lane < LANE_COUNT; ++lane) {
        const float x = ROAD_LEFT + static_cast<float>(lane) * LANE_WIDTH - 3.0f;
        for (float y = -90.0f + game.roadOffset; y < static_cast<float>(WINDOW_HEIGHT) + 90.0f; y += 90.0f) {
            sf::RectangleShape dash({6.0f, 44.0f});
            dash.setPosition({x, y});
            dash.setFillColor(sf::Color(232, 236, 240, 210));
            window.draw(dash);
        }
    }
}

void renderEntities(sf::RenderWindow& window, const GameData& game) {
    // 1. Kavos bonusai
    for (const CoffeeBonus& bonus : game.bonuses) {
        if (bonus.active) {
            drawCoffeeBonus(window, bonus);
        }
    }

    // 2. Kliūtys
    for (const Obstacle& obs : game.obstacles) {
        if (obs.type == ObstacleType::Roadworks) {
            drawRoadworks(window, obs.position, obs.size);
        } else {
            const bool isChaotic = (obs.type == ObstacleType::ChaoticDriver);
            const bool blinkPhase = (static_cast<int>(obs.signalTimer * 12.0f) % 2 == 0);
            const bool signalingActive = obs.isSignaling && blinkPhase;
            const bool blinkLeft = signalingActive && (obs.targetLane < obs.currentLane);
            const bool blinkRight = signalingActive && (obs.targetLane > obs.currentLane);

            drawCar(window, obs.position, obs.size, obs.color,
                    false, isChaotic, blinkLeft, blinkRight, 255);
        }
    }

    // 3. Žaidėjo automobilis – žalias dėstytojo pavežėjas (mirksi imuniteto metu po smūgio)
    std::uint8_t playerAlpha = 255;
    if (game.player.invincibilityTimer > 0.0f) {
        const bool dimFrame = (static_cast<int>(game.player.invincibilityTimer * 14.0f) % 2 == 0);
        playerAlpha = dimFrame ? 115 : 235;
    }

    drawCar(window, game.player.position, game.player.size,
            sf::Color(46, 204, 113), true, false, false, false, playerAlpha);
}

void renderHUD(sf::RenderWindow& window, const GameData& game, const sf::Font& font) {
    // Raudonas blyksnis po susidūrimo
    if (game.screenFlashTimer > 0.0f) {
        const std::uint8_t alpha = static_cast<std::uint8_t>(std::min(130.0f, game.screenFlashTimer * 360.0f));
        sf::RectangleShape flash({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        flash.setFillColor(sf::Color(220, 40, 40, alpha));
        window.draw(flash);
    }

    // --- KAIRĖS PUSĖS KORTELĖ: Maršrutas ir Vėlavimas ---
    drawPanelBox(window, {18.0f, 18.0f}, {305.0f, 255.0f});

    sf::Text headerLeft(font, toSfString("MARŠRUTAS Į PASKAITĄ"), 18);
    headerLeft.setFillColor(sf::Color(150, 195, 255));
    headerLeft.setPosition({34.0f, 32.0f});
    window.draw(headerLeft);

    sf::Text distLabel(font, toSfString("Atstumas: " + formatDistance(game.distanceKm)), 17);
    distLabel.setFillColor(sf::Color::White);
    distLabel.setPosition({34.0f, 68.0f});
    window.draw(distLabel);

    // Progreso juosta
    const float barW = 270.0f;
    sf::RectangleShape barBg({barW, 18.0f});
    barBg.setPosition({34.0f, 100.0f});
    barBg.setFillColor(sf::Color(42, 50, 64));
    barBg.setOutlineThickness(1.0f);
    barBg.setOutlineColor(sf::Color(95, 112, 138));
    window.draw(barBg);

    const float progress = std::clamp(game.distanceKm / TARGET_DISTANCE_KM, 0.0f, 1.0f);
    sf::RectangleShape barFill({barW * progress, 18.0f});
    barFill.setPosition({34.0f, 100.0f});
    barFill.setFillColor(sf::Color(46, 204, 113));
    window.draw(barFill);

    sf::Text delayTitle(font, toSfString("Vėlavimas į paskaitą:"), 16);
    delayTitle.setFillColor(sf::Color(190, 200, 215));
    delayTitle.setPosition({34.0f, 136.0f});
    window.draw(delayTitle);

    sf::Text delayValue(font, toSfString(formatDelayTime(game.delaySeconds)), 25);
    delayValue.setFillColor(getDelayColor(game.delaySeconds));
    delayValue.setPosition({34.0f, 160.0f});
    window.draw(delayValue);

    const int speedKmh = static_cast<int>(game.currentRoadSpeed * game.speedMultiplier * 0.16f);
    sf::Text speedText(font, toSfString("Greitis: " + std::to_string(speedKmh) + " km/h"), 17);
    speedText.setFillColor(sf::Color(215, 225, 240));
    speedText.setPosition({34.0f, 204.0f});
    window.draw(speedText);

    sf::Text limitText(font, toSfString("Riba: 10 min (tada studentai išeina)"), 13);
    limitText.setFillColor(sf::Color(150, 162, 180));
    limitText.setPosition({34.0f, 236.0f});
    window.draw(limitText);

    // --- DEŠINĖS PUSĖS KORTELĖ: Gyvybės, Taškai, Valdymas ---
    drawPanelBox(window, {956.0f, 18.0f}, {305.0f, 290.0f});

    sf::Text headerRight(font, toSfString("PAVEŽĖJO BŪSENA"), 18);
    headerRight.setFillColor(sf::Color(150, 195, 255));
    headerRight.setPosition({972.0f, 32.0f});
    window.draw(headerRight);

    sf::Text livesLabel(font, toSfString("Arto kantrybė:"), 16);
    livesLabel.setFillColor(sf::Color::White);
    livesLabel.setPosition({972.0f, 68.0f});
    window.draw(livesLabel);

    for (int i = 0; i < PLAYER_MAX_LIVES; ++i) {
        sf::RectangleShape lifeBar({76.0f, 18.0f});
        lifeBar.setPosition({972.0f + static_cast<float>(i) * 86.0f, 98.0f});
        lifeBar.setFillColor(i < game.player.lives ? sf::Color(231, 76, 60) : sf::Color(55, 60, 70));
        lifeBar.setOutlineThickness(1.5f);
        lifeBar.setOutlineColor(sf::Color(230, 235, 245, 160));
        window.draw(lifeBar);
    }

    sf::Text scoreText(font, toSfString("Taškai: " + std::to_string(game.score)), 22);
    scoreText.setFillColor(sf::Color(248, 214, 96));
    scoreText.setPosition({972.0f, 134.0f});
    window.draw(scoreText);

    sf::Text statsText(font,
        toSfString("Aplenktos kliūtys: " + std::to_string(game.dodgedCount) +
                   "\nIšgerta kavų: " + std::to_string(game.coffeesCollected)),
        15);
    statsText.setFillColor(sf::Color(200, 210, 224));
    statsText.setPosition({972.0f, 172.0f});
    window.draw(statsText);

    sf::Text controlsText(font,
        toSfString("Valdymas:\nA / D arba <- / ->  juostos\nW / S arba ^ / v    greitis\nP / Esc             pauzė"),
        14);
    controlsText.setFillColor(sf::Color(155, 170, 190));
    controlsText.setPosition({972.0f, 224.0f});
    window.draw(controlsText);

    // --- ARTO KALBOS BURBULAS ŠALIA AUTOMOBILIO ---
    drawCarSpeechBubble(window, game, font);
}

void renderMenu(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(10, 14, 22, 180));
    window.draw(dim);

    drawPanelBox(window, {290.0f, 90.0f}, {700.0f, 540.0f});

    drawCenteredText(window, font, "20 MINUČIŲ IKI PASKAITOS", 36, sf::Color(255, 214, 84), {640.0f, 150.0f});
    drawCenteredText(window, font, "Esi dėstytojo Arto pavežėjas – nuvežk jį į paskaitą laiku!", 19, sf::Color(165, 205, 255), {640.0f, 198.0f});

    const std::string rules =
        "Tikslas: nuvežk Artą 5.0 km iki universiteto neišsekinus jo\n"
        "kantrybės ir neviršijus 10 min. vėlavimo ribos!\n\n"
        "- Tavo automobilis – žalias pavežėjas; kiti vairuotojai tau trukdo.\n"
        "- Tamsūs sportiniai automobiliai mirksi posūkiu ir keičia juostas.\n"
        "- Kavos puodeliai nudžiugina Artą (+1 kantrybės, -0.5 min)!\n\n"
        "Valdymas: WASD / Rodyklės   |   Pauzė: P arba Esc";

    sf::Text rulesText(font, toSfString(rules), 16);
    rulesText.setFillColor(sf::Color(225, 232, 242));
    rulesText.setPosition({340.0f, 242.0f});
    window.draw(rulesText);

    drawButton(window, font, getPrimaryButton(GameState::Menu), mousePos, sf::Color(39, 155, 92));
    drawButton(window, font, getSecondaryButton(GameState::Menu), mousePos, sf::Color(158, 56, 56));
}

void renderPauseOverlay(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 185));
    window.draw(dim);

    drawPanelBox(window, {420.0f, 205.0f}, {440.0f, 355.0f});
    drawCenteredText(window, font, "PAUZĖ", 36, sf::Color(255, 214, 84), {640.0f, 260.0f});

    drawButton(window, font, getPrimaryButton(GameState::Paused), mousePos, sf::Color(39, 155, 92));
    drawButton(window, font, getSecondaryButton(GameState::Paused), mousePos, sf::Color(52, 118, 188));
    drawButton(window, font, getTertiaryButton(GameState::Paused), mousePos, sf::Color(145, 65, 65));
}

void renderEndOverlay(sf::RenderWindow& window, const GameData& game, const sf::Font& font, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 195));
    window.draw(dim);

    drawPanelBox(window, {300.0f, 115.0f}, {680.0f, 505.0f});

    if (game.state == GameState::Victory) {
        drawCenteredText(window, font, "ARTAS PRISTATYTAS!", 36, sf::Color(88, 224, 145), {640.0f, 170.0f});

        const float delayMins = game.delaySeconds / 60.0f;
        std::string rank;
        if (delayMins < 6.0f) {
            rank = "Arto įvertinimas: 5/5 žvaigždutės! Rekordiškai greita kelionė!";
        } else if (delayMins <= 8.5f) {
            rank = "Arto įvertinimas: 4/5 žvaigždutės! Spėjome beveik laiku!";
        } else {
            rank = "Arto įvertinimas: 3/5 žvaigždutės! Studentai jau stovėjo prie durų!";
        }
        drawCenteredText(window, font, rank, 18, sf::Color(255, 225, 120), {640.0f, 224.0f});
    } else {
        drawCenteredText(window, font, "KELIONĖ ATŠAUKTA...", 36, sf::Color(236, 92, 82), {640.0f, 170.0f});

        const std::string reason = (game.gameOverReason == GameOverReason::TooLate)
            ? "Praėjo 10 min. – Artas: „Studentai jau išėjo namo!“"
            : "Artas neišlaikė: „Sustok, geriau eisiu pėsčiomis!“";
        drawCenteredText(window, font, reason, 19, sf::Color(255, 196, 140), {640.0f, 224.0f});
    }

    const std::string summary =
        "Nuvažiuotas atstumas: " + formatDistance(game.distanceKm) + "\n"
        "Vėlavimo laikas:      " + formatDelayTime(game.delaySeconds) + "\n"
        "Surinkti taškai:      " + std::to_string(game.score) + "\n"
        "Aplenktos kliūtys:    " + std::to_string(game.dodgedCount) + "\n"
        "Išgerta kavų:         " + std::to_string(game.coffeesCollected);

    sf::Text summaryText(font, toSfString(summary), 19);
    summaryText.setFillColor(sf::Color(232, 238, 246));
    summaryText.setPosition({390.0f, 278.0f});
    window.draw(summaryText);

    drawButton(window, font, getPrimaryButton(game.state), mousePos, sf::Color(39, 155, 92));
    drawButton(window, font, getSecondaryButton(game.state), mousePos, sf::Color(70, 95, 130));
}

void renderGame(sf::RenderWindow& window, const GameData& game, const sf::Font& font) {
    window.clear(sf::Color(24, 30, 38));

    renderRoad(window, game);
    if (game.state != GameState::Menu) {
        renderEntities(window, game);
        renderHUD(window, game, font);
    }

    const sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    if (game.state == GameState::Menu) {
        renderMenu(window, font, mousePos);
    } else if (game.state == GameState::Paused) {
        renderPauseOverlay(window, font, mousePos);
    } else if (game.state == GameState::Victory || game.state == GameState::GameOver) {
        renderEndOverlay(window, game, font, mousePos);
    }

    window.display();
}
