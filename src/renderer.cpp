#include "renderer.hpp"
#include "effects.hpp"
#include "game_logic.hpp"
#include "storage.hpp"

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

std::string formatDistance(float km, float targetKm) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << km << " / " << std::setprecision(1) << targetKm << " km";
    return oss.str();
}

sf::Color getDelayColor(float delaySeconds, float maxDelayMinutes) {
    const float ratio = (delaySeconds / 60.0f) / std::max(1.0f, maxDelayMinutes);
    if (ratio < 0.50f) return sf::Color(88, 214, 141);
    if (ratio < 0.75f) return sf::Color(244, 208, 63);
    if (ratio < 0.90f) return sf::Color(235, 152, 78);
    return sf::Color(231, 76, 60);
}

void drawPanelBox(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size, sf::Color outline = sf::Color(68, 86, 114)) {
    sf::RectangleShape shadow(size);
    shadow.setPosition({pos.x + 5.0f, pos.y + 6.0f});
    shadow.setFillColor(sf::Color(0, 0, 0, 90));
    window.draw(shadow);

    sf::RectangleShape panel(size);
    panel.setPosition(pos);
    panel.setFillColor(sf::Color(18, 24, 34, 232));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(outline);
    window.draw(panel);
}

void drawButton(sf::RenderWindow& window,
                const sf::Font& font,
                const ButtonRect& button,
                sf::Vector2f mousePos) {
    const bool hovered = button.bounds.contains(mousePos);

    sf::RectangleShape rect(button.bounds.size);
    rect.setPosition(button.bounds.position);
    rect.setFillColor(hovered
        ? sf::Color(
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(button.baseColor.r) + 28)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(button.baseColor.g) + 28)),
            static_cast<std::uint8_t>(std::min(255, static_cast<int>(button.baseColor.b) + 28))
        )
        : button.baseColor);
    rect.setOutlineThickness(hovered ? 3.0f : 2.0f);
    rect.setOutlineColor(hovered ? sf::Color(255, 255, 255) : sf::Color(210, 220, 235, 180));
    window.draw(rect);

    const sf::Vector2f center = {
        button.bounds.position.x + button.bounds.size.x * 0.5f,
        button.bounds.position.y + button.bounds.size.y * 0.5f
    };
    drawCenteredText(window, font, button.label, 19, sf::Color::White, center);
}

void drawSpriteFit(sf::RenderWindow& window,
                   const sf::Texture& texture,
                   sf::Vector2f pos,
                   sf::Vector2f targetSize,
                   std::uint8_t alpha = 255) {
    sf::Sprite sprite(texture);
    const sf::Vector2u texSize = texture.getSize();
    if (texSize.x > 0 && texSize.y > 0) {
        sprite.setScale({
            targetSize.x / static_cast<float>(texSize.x),
            targetSize.y / static_cast<float>(texSize.y)
        });
    }
    sprite.setPosition(pos);
    sprite.setColor(sf::Color(255, 255, 255, alpha));
    window.draw(sprite);
}

void drawBlinkers(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size, bool blinkLeft, bool blinkRight) {
    if (blinkLeft) {
        sf::CircleShape b1(7.0f);
        b1.setPosition({pos.x - 5.0f, pos.y + 8.0f});
        b1.setFillColor(sf::Color(255, 175, 20));
        window.draw(b1);

        sf::CircleShape b2(7.0f);
        b2.setPosition({pos.x - 5.0f, pos.y + size.y - 20.0f});
        b2.setFillColor(sf::Color(255, 175, 20));
        window.draw(b2);
    }
    if (blinkRight) {
        sf::CircleShape b1(7.0f);
        b1.setPosition({pos.x + size.x - 9.0f, pos.y + 8.0f});
        b1.setFillColor(sf::Color(255, 175, 20));
        window.draw(b1);

        sf::CircleShape b2(7.0f);
        b2.setPosition({pos.x + size.x - 9.0f, pos.y + size.y - 20.0f});
        b2.setFillColor(sf::Color(255, 175, 20));
        window.draw(b2);
    }
}

void drawFallbackRect(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size, sf::Color color, std::uint8_t alpha = 255) {
    color.a = alpha;
    sf::RectangleShape rect(size);
    rect.setPosition(pos);
    rect.setFillColor(color);
    rect.setOutlineThickness(2.0f);
    rect.setOutlineColor(sf::Color(20, 24, 30, alpha));
    window.draw(rect);
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
            if (!result.empty()) result += "\n";
            result += currentLine;
            currentLine = word;
        } else {
            currentLine = candidate;
        }
    }
    if (!currentLine.empty()) {
        if (!result.empty()) result += "\n";
        result += currentLine;
    }
    return result;
}

void drawCarSpeechBubble(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    if (game.statusMessageTimer <= 0.0f || game.statusMessage.empty()) {
        return;
    }

    const float fadeRatio = std::clamp(game.statusMessageTimer / 0.35f, 0.0f, 1.0f);
    const std::uint8_t bgAlpha = static_cast<std::uint8_t>(242.0f * fadeRatio);
    const std::uint8_t fgAlpha = static_cast<std::uint8_t>(255.0f * fadeRatio);

    const std::string wrapped = wrapTextToWidth(res.font, game.statusMessage, 14, 215.0f);

    sf::Text speakerText(res.font, toSfString("Artas:"), 14);
    speakerText.setStyle(sf::Text::Bold);
    speakerText.setFillColor(sf::Color(22, 118, 66, fgAlpha));

    sf::Text bodyText(res.font, toSfString(wrapped), 14);
    bodyText.setFillColor(sf::Color(22, 28, 38, fgAlpha));

    const sf::FloatRect speakerBounds = speakerText.getLocalBounds();
    const sf::FloatRect bodyBounds = bodyText.getLocalBounds();

    const float avatarSize = res.hasArtasFaces ? 44.0f : 0.0f;
    const float avatarGap = res.hasArtasFaces ? 10.0f : 0.0f;
    const float padX = 12.0f;
    const float padY = 10.0f;
    const float gapY = 5.0f;

    const float textW = std::max(speakerBounds.size.x, bodyBounds.size.x);
    const float textH = speakerBounds.size.y + gapY + bodyBounds.size.y + 6.0f;
    const float bubbleW = std::max(165.0f, avatarSize + avatarGap + textW + padX * 2.0f);
    const float bubbleH = std::max(avatarSize, textH) + padY * 2.0f;

    const float carCenterX = game.player.position.x + game.player.size.x * 0.5f;
    const float carBackSeatY = game.player.position.y + game.player.size.y * 0.62f;

    bool placeLeft = (carCenterX < static_cast<float>(WINDOW_WIDTH) * 0.5f);
    const float tailGap = 16.0f;

    float bubbleX = placeLeft
        ? (game.player.position.x - tailGap - bubbleW)
        : (game.player.position.x + game.player.size.x + tailGap);
    float bubbleY = carBackSeatY - bubbleH * 0.55f;

    if (placeLeft && bubbleX < 332.0f && bubbleY < 305.0f) {
        placeLeft = false;
        bubbleX = game.player.position.x + game.player.size.x + tailGap;
    } else if (!placeLeft && (bubbleX + bubbleW) > 948.0f && bubbleY < 335.0f) {
        placeLeft = true;
        bubbleX = game.player.position.x - tailGap - bubbleW;
    }

    bubbleX = std::clamp(bubbleX, 16.0f, static_cast<float>(WINDOW_WIDTH) - bubbleW - 16.0f);
    bubbleY = std::clamp(bubbleY, 16.0f, static_cast<float>(WINDOW_HEIGHT) - bubbleH - 16.0f);

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

    sf::RectangleShape bubble({bubbleW, bubbleH});
    bubble.setPosition({bubbleX, bubbleY});
    bubble.setFillColor(sf::Color(250, 252, 255, bgAlpha));
    bubble.setOutlineThickness(2.0f);
    bubble.setOutlineColor(sf::Color(28, 38, 52, bgAlpha));

    window.draw(tail);
    window.draw(bubble);

    sf::RectangleShape seamCover({4.0f, 14.0f});
    seamCover.setPosition({placeLeft ? (bubbleX + bubbleW - 3.0f) : (bubbleX - 1.0f), tailMidY - 7.0f});
    seamCover.setFillColor(sf::Color(250, 252, 255, bgAlpha));
    window.draw(seamCover);

    // Arto portretas pagal likusią kantrybę
    if (res.hasArtasFaces) {
        const sf::Texture& face = (game.player.lives >= 3)
            ? res.artasHappy
            : (game.player.lives == 2 ? res.artasNeutral : res.artasAngry);
        drawSpriteFit(window, face, {bubbleX + padX, bubbleY + (bubbleH - avatarSize) * 0.5f},
                      {avatarSize, avatarSize}, fgAlpha);
    }

    const float textStartX = bubbleX + padX + avatarSize + avatarGap;
    speakerText.setPosition({textStartX - speakerBounds.position.x,
                             bubbleY + padY - speakerBounds.position.y});
    bodyText.setPosition({textStartX - bodyBounds.position.x,
                          bubbleY + padY + speakerBounds.size.y + gapY - bodyBounds.position.y});

    window.draw(speakerText);
    window.draw(bodyText);
}

void drawMuteButton(sf::RenderWindow& window, const GameData& game, const sf::Font& font, sf::Vector2f mousePos) {
    const sf::FloatRect bounds = getMuteButtonBounds();
    const bool hovered = bounds.contains(mousePos);

    sf::RectangleShape box(bounds.size);
    box.setPosition(bounds.position);
    box.setFillColor(game.soundMuted ? sf::Color(115, 42, 46, 225) : sf::Color(26, 40, 58, 225));
    box.setOutlineThickness(hovered ? 2.5f : 1.5f);
    box.setOutlineColor(hovered ? sf::Color::White : sf::Color(110, 130, 160));
    window.draw(box);

    const std::string label = game.soundMuted ? "Garsas: OFF (M)" : "Garsas: ON (M)";
    drawCenteredText(window, font, label, 13, sf::Color(230, 238, 248),
                     {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
}

void renderMenuOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 175));
    window.draw(dim);

    drawPanelBox(window, {310.0f, 80.0f}, {660.0f, 545.0f}, sf::Color(88, 135, 195));

    if (res.hasArtasFaces) {
        drawSpriteFit(window, res.artasHappy, {598.0f, 98.0f}, {84.0f, 84.0f});
    }

    drawCenteredText(window, res.font, "20 MINUČIŲ IKI PASKAITOS", 34, sf::Color(255, 214, 84), {640.0f, 216.0f});
    drawCenteredText(window, res.font, "Esi dėstytojo Arto pavežėjas – nuvežk jį į Saulėtekį laiku!", 17, sf::Color(175, 212, 255), {640.0f, 260.0f});
    drawCenteredText(window, res.font, "Valdymas: WASD / Rodyklės   |   Pauzė: P / Esc   |   Garsas: M", 14, sf::Color(160, 176, 200), {640.0f, 300.0f});

    for (const ButtonRect& btn : getScreenButtons(GameState::Menu)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderRouteSelectOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 185));
    window.draw(dim);

    drawPanelBox(window, {295.0f, 85.0f}, {690.0f, 535.0f});
    drawCenteredText(window, res.font, "PASIRINK MARŠRUTĄ", 32, sf::Color(255, 214, 84), {640.0f, 135.0f});
    drawCenteredText(window, res.font, "Spausk ant maršruto arba 1 / 2 / 3 klaviatūroje", 15, sf::Color(175, 192, 215), {640.0f, 172.0f});

    const std::vector<ButtonRect> buttons = getScreenButtons(GameState::RouteSelect);
    for (const ButtonRect& btn : buttons) {
        if (btn.actionId >= 10 && btn.actionId <= 12) {
            const RouteConfig route = getRouteConfig(btn.actionId - 10);
            const bool hovered = btn.bounds.contains(mousePos);

            sf::RectangleShape card(btn.bounds.size);
            card.setPosition(btn.bounds.position);
            card.setFillColor(hovered
                ? sf::Color(
                    static_cast<std::uint8_t>(std::min(255, static_cast<int>(btn.baseColor.r) + 25)),
                    static_cast<std::uint8_t>(std::min(255, static_cast<int>(btn.baseColor.g) + 25)),
                    static_cast<std::uint8_t>(std::min(255, static_cast<int>(btn.baseColor.b) + 25))
                )
                : btn.baseColor);
            card.setOutlineThickness(hovered ? 3.0f : 2.0f);
            card.setOutlineColor(hovered ? sf::Color::White : sf::Color(220, 230, 245, 180));
            window.draw(card);

            sf::Text title(res.font, toSfString(btn.label + "  [" + route.difficulty + "]"), 20);
            title.setStyle(sf::Text::Bold);
            title.setFillColor(sf::Color::White);
            title.setPosition({btn.bounds.position.x + 22.0f, btn.bounds.position.y + 14.0f});
            window.draw(title);

            sf::Text desc(res.font, toSfString(route.description), 15);
            desc.setFillColor(sf::Color(235, 242, 250));
            desc.setPosition({btn.bounds.position.x + 22.0f, btn.bounds.position.y + 48.0f});
            window.draw(desc);
        } else {
            drawButton(window, res.font, btn, mousePos);
        }
    }
}

void renderHowToPlayOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 190));
    window.draw(dim);

    drawPanelBox(window, {195.0f, 65.0f}, {890.0f, 595.0f});
    drawCenteredText(window, res.font, "KAIP ŽAISTI", 32, sf::Color(255, 214, 84), {640.0f, 110.0f});

    // Kairė kolona: Kliūtys
    sf::Text leftHeader(res.font, toSfString("KLIŪTYS KELYJE:"), 19);
    leftHeader.setStyle(sf::Text::Bold);
    leftHeader.setFillColor(sf::Color(255, 165, 145));
    leftHeader.setPosition({235.0f, 152.0f});
    window.draw(leftHeader);

    const std::string obstaclesInfo =
        "- Sportiniai automobiliai: mirksi posūkiu ir staigiai\n"
        "  persirikiuoja į gretimą juostą!\n\n"
        "- Raudoni troleibusai: ilgi ir lėti – apvažiuok iš anksto.\n\n"
        "- Paspirtukininkai: siauri, greiti ir vingiuoja juostoje.\n\n"
        "- Kelio darbai: stacionarios oranžinės užtvaros.\n\n"
        "Kiekviena avarija atima -1 Arto kantrybės ir prideda +1 min!";

    sf::Text obsText(res.font, toSfString(obstaclesInfo), 14);
    obsText.setFillColor(sf::Color(225, 232, 242));
    obsText.setPosition({235.0f, 190.0f});
    window.draw(obsText);

    // Dešinė kolona: Bonusai ir valdymas
    sf::Text rightHeader(res.font, toSfString("BONUSAI IR VALDYMAS:"), 19);
    rightHeader.setStyle(sf::Text::Bold);
    rightHeader.setFillColor(sf::Color(130, 235, 165));
    rightHeader.setPosition({680.0f, 152.0f});
    window.draw(rightHeader);

    // Bonusų ikonėlės + aprašymai
    if (res.hasCoffee) drawSpriteFit(window, res.coffee, {680.0f, 192.0f}, {44.0f, 44.0f});
    sf::Text tCoffee(res.font, toSfString("Kava: +1 Arto kantrybės,\n+150 tšk. ir -0.5 min vėlavimo."), 14);
    tCoffee.setFillColor(sf::Color(235, 240, 248));
    tCoffee.setPosition({736.0f, 194.0f});
    window.draw(tCoffee);

    if (res.hasGreenWave) drawSpriteFit(window, res.greenWave, {680.0f, 256.0f}, {44.0f, 44.0f});
    sf::Text tWave(res.font, toSfString("Žalia banga: 3.5 s turbo greitis\nir laikinas imunitetas avarijoms!"), 14);
    tWave.setFillColor(sf::Color(235, 240, 248));
    tWave.setPosition({736.0f, 258.0f});
    window.draw(tWave);

    if (res.hasUsb) drawSpriteFit(window, res.usb, {680.0f, 320.0f}, {44.0f, 44.0f});
    sf::Text tUsb(res.font, toSfString("USB su skaidrėmis: +300 taškų!\nArtas labai apsidžiaugia."), 14);
    tUsb.setFillColor(sf::Color(235, 240, 248));
    tUsb.setPosition({736.0f, 322.0f});
    window.draw(tUsb);

    sf::Text ctrl(res.font,
        toSfString("Valdymas:\n"
                   "  A / D arba <- / -> : keisti juostas\n"
                   "  W / S arba ^ / v   : greitinti / stabdyti\n"
                   "  P arba Esc         : pauzė\n"
                   "  M                  : išjungti / įjungti garsą"),
        14);
    ctrl.setFillColor(sf::Color(185, 205, 230));
    ctrl.setPosition({680.0f, 396.0f});
    window.draw(ctrl);

    for (const ButtonRect& btn : getScreenButtons(GameState::HowToPlay)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderHighScoresOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 190));
    window.draw(dim);

    drawPanelBox(window, {225.0f, 75.0f}, {830.0f, 580.0f});
    drawCenteredText(window, res.font, "GERIAUSI PAVEŽĖJAI (TOP 5)", 30, sf::Color(255, 214, 84), {640.0f, 122.0f});

    // 3 maršrutų skirtukai
    static const std::string tabNames[ROUTE_COUNT] = {"1. Antakalnis", "2. Centras", "3. Pašilaičiai"};
    for (int r = 0; r < ROUTE_COUNT; ++r) {
        const sf::FloatRect tab = getHighScoreTabBounds(r);
        const bool active = (game.highScoreTab == r);
        const bool hovered = tab.contains(mousePos);

        sf::RectangleShape box(tab.size);
        box.setPosition(tab.position);
        box.setFillColor(active ? sf::Color(46, 145, 92) : (hovered ? sf::Color(58, 74, 98) : sf::Color(34, 44, 60)));
        box.setOutlineThickness(active ? 2.5f : 1.5f);
        box.setOutlineColor(active ? sf::Color::White : sf::Color(95, 115, 145));
        window.draw(box);

        drawCenteredText(window, res.font, tabNames[r], 16, sf::Color::White,
                         {tab.position.x + tab.size.x * 0.5f, tab.position.y + tab.size.y * 0.5f});
    }

    const RouteConfig route = getRouteConfig(game.highScoreTab);
    drawCenteredText(window, res.font, route.name + " (" + route.difficulty + ")", 16, sf::Color(165, 205, 255), {640.0f, 236.0f});

    // Lentelės antraštė
    sf::Text header(res.font, toSfString("Vieta   Vairuotojas        Taškai     Vėlavimas      Įvertinimas"), 16);
    header.setStyle(sf::Text::Bold);
    header.setFillColor(sf::Color(255, 224, 120));
    header.setPosition({270.0f, 268.0f});
    window.draw(header);

    const std::vector<HighScoreEntry> list = getRouteHighScores(game.highScores, game.highScoreTab);
    if (list.empty()) {
        drawCenteredText(window, res.font, "Šiame maršrute dar nėra išsaugotų rekordų.\nBūk pirmasis!", 19, sf::Color(180, 195, 215), {640.0f, 400.0f});
    } else {
        for (std::size_t i = 0; i < list.size(); ++i) {
            const HighScoreEntry& e = list[i];
            const float rowY = 312.0f + static_cast<float>(i) * 48.0f;

            sf::RectangleShape rowBg({740.0f, 40.0f});
            rowBg.setPosition({270.0f, rowY - 6.0f});
            rowBg.setFillColor(i == 0 ? sf::Color(48, 66, 48, 190) : sf::Color(30, 38, 52, 180));
            window.draw(rowBg);

            std::ostringstream oss;
            oss << " #" << (i + 1) << "     "
                << std::left << std::setw(15) << e.playerName
                << std::right << std::setw(7) << e.score << "      "
                << formatDelayTime(e.delaySeconds) << "     "
                << e.stars << "/5 zv.";

            sf::Text rowText(res.font, toSfString(oss.str()), 16);
            rowText.setFillColor(i == 0 ? sf::Color(255, 232, 130) : sf::Color(232, 238, 248));
            rowText.setPosition({280.0f, rowY + 2.0f});
            window.draw(rowText);
        }
    }

    for (const ButtonRect& btn : getScreenButtons(GameState::HighScores)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderCountdownOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 18, 110));
    window.draw(dim);

    const int sec = static_cast<int>(std::ceil(game.countdownTimer));
    const std::string label = (sec >= 1) ? std::to_string(sec) : "VAŽIUOJAM!";
    const sf::Color col = (sec >= 1) ? sf::Color(255, 224, 90) : sf::Color(88, 235, 145);

    drawCenteredText(window, res.font, "PASIRUOŠK!", 28, sf::Color::White, {640.0f, 250.0f});
    drawCenteredText(window, res.font, label, 84, col, {640.0f, 345.0f});
}

void renderPauseOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 185));
    window.draw(dim);

    drawPanelBox(window, {415.0f, 205.0f}, {450.0f, 330.0f});
    drawCenteredText(window, res.font, "PAUZĖ", 36, sf::Color(255, 214, 84), {640.0f, 260.0f});

    for (const ButtonRect& btn : getScreenButtons(GameState::Paused)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderNameInputOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 195));
    window.draw(dim);

    drawPanelBox(window, {310.0f, 130.0f}, {660.0f, 470.0f}, sf::Color(88, 214, 141));
    drawCenteredText(window, res.font, "ARTAS PRISTATYTAS!", 34, sf::Color(88, 224, 145), {640.0f, 185.0f});

    const RouteConfig route = getRouteConfig(game.selectedRoute);
    const int stars = getStarsForRun(game.delaySeconds, route);
    drawCenteredText(window, res.font,
        "Taškai: " + std::to_string(game.score) + "  |  Vėlavimas: " + formatDelayTime(game.delaySeconds) +
        "  |  " + std::to_string(stars) + "/5 žvaigždutės",
        17, sf::Color(255, 228, 130), {640.0f, 236.0f});

    drawCenteredText(window, res.font, "Įvesk savo vairuotojo vardą rekordų lentelei:", 17, sf::Color(225, 235, 248), {640.0f, 288.0f});

    // Teksto įvedimo laukelis su mirksinčiu žymekliu
    sf::RectangleShape inputBox({380.0f, 52.0f});
    inputBox.setPosition({450.0f, 322.0f});
    inputBox.setFillColor(sf::Color(28, 36, 50));
    inputBox.setOutlineThickness(2.5f);
    inputBox.setOutlineColor(game.nameValidationError.empty() ? sf::Color(96, 178, 255) : sf::Color(235, 86, 80));
    window.draw(inputBox);

    const bool showCursor = (static_cast<int>(game.totalElapsed * 2.2f) % 2 == 0);
    const std::string displayInput = game.playerNameInput + (showCursor ? "_" : "");
    drawCenteredText(window, res.font, displayInput, 24, sf::Color::White, {640.0f, 348.0f});

    if (!game.nameValidationError.empty()) {
        drawCenteredText(window, res.font, game.nameValidationError, 14, sf::Color(255, 118, 112), {640.0f, 404.0f});
    } else {
        drawCenteredText(window, res.font, "Tik raidės (A-Z), skaičiai, tarpas, _ arba - (iki 12 simb.)", 13, sf::Color(150, 168, 192), {640.0f, 404.0f});
    }

    for (const ButtonRect& btn : getScreenButtons(GameState::NameInput)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderEndOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(8, 12, 20, 195));
    window.draw(dim);

    const RouteConfig route = getRouteConfig(game.selectedRoute);
    drawPanelBox(window, {295.0f, 82.0f}, {690.0f, 575.0f});

    if (game.state == GameState::Victory) {
        drawCenteredText(window, res.font, "ARTAS PRISTATYTAS!", 34, sf::Color(88, 224, 145), {640.0f, 134.0f});

        const int stars = getStarsForRun(game.delaySeconds, route);
        std::string rank;
        if (stars == 5) rank = "Arto įvertinimas: 5/5 žvaigždutės! Rekordiškai greita kelionė!";
        else if (stars == 4) rank = "Arto įvertinimas: 4/5 žvaigždutės! Spėjome beveik laiku!";
        else rank = "Arto įvertinimas: 3/5 žvaigždutės! Studentai jau stovėjo prie durų!";

        drawCenteredText(window, res.font, rank, 17, sf::Color(255, 225, 120), {640.0f, 182.0f});
        if (game.scoreSavedThisRun) {
            drawCenteredText(window, res.font, "Rezultatas išsaugotas rekordų lentelėje!", 14, sf::Color(130, 230, 165), {640.0f, 214.0f});
        }
    } else {
        drawCenteredText(window, res.font, "KELIONĖ ATŠAUKTA...", 34, sf::Color(236, 92, 82), {640.0f, 134.0f});

        const std::string reason = (game.gameOverReason == GameOverReason::TooLate)
            ? ("Praėjo " + std::to_string(static_cast<int>(route.maxDelayMinutes)) + " min. – Artas: „Studentai jau išėjo namo!“")
            : "Artas neišlaikė: „Sustok, geriau eisiu pėsčiomis!“";
        drawCenteredText(window, res.font, reason, 18, sf::Color(255, 196, 140), {640.0f, 186.0f});
    }

    const std::string summary =
        "Maršrutas:            " + route.name + "\n"
        "Nuvažiuotas atstumas: " + formatDistance(game.distanceKm, route.targetDistanceKm) + "\n"
        "Vėlavimo laikas:      " + formatDelayTime(game.delaySeconds) + "\n"
        "Surinkti taškai:      " + std::to_string(game.score) + "\n"
        "Aplenktos kliūtys:    " + std::to_string(game.dodgedCount) + "\n"
        "Kavos / USB skaidrės: " + std::to_string(game.coffeesCollected) + " / " + std::to_string(game.usbCollected);

    sf::Text summaryText(res.font, toSfString(summary), 17);
    summaryText.setFillColor(sf::Color(232, 238, 246));
    summaryText.setPosition({365.0f, 246.0f});
    window.draw(summaryText);

    for (const ButtonRect& btn : getScreenButtons(game.state)) {
        drawButton(window, res.font, btn, mousePos);
    }
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
    const RouteConfig route = getRouteConfig(game.selectedRoute);
    const float progress = std::clamp(game.distanceKm / route.targetDistanceKm, 0.0f, 1.0f);

    // Subtilus rytinės -> dienos šviesos perėjimas pakelėje
    const sf::Color grassColor(
        static_cast<std::uint8_t>(30.0f + 16.0f * progress),
        static_cast<std::uint8_t>(66.0f + 26.0f * progress),
        static_cast<std::uint8_t>(42.0f + 12.0f * progress)
    );
    sf::RectangleShape grass({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    grass.setFillColor(grassColor);
    window.draw(grass);

    for (const SceneryProp& prop : game.scenery) {
        if (prop.isBuilding) {
            sf::RectangleShape shadow(prop.rectSize);
            shadow.setPosition({prop.position.x + 8.0f, prop.position.y + 8.0f});
            shadow.setFillColor(sf::Color(0, 0, 0, 55));
            window.draw(shadow);

            sf::RectangleShape building(prop.rectSize);
            building.setPosition(prop.position);
            building.setFillColor(prop.color);
            building.setOutlineThickness(2.0f);
            building.setOutlineColor(sf::Color(28, 34, 46));
            window.draw(building);
        } else {
            sf::CircleShape shadow(prop.radius);
            shadow.setOrigin({prop.radius, prop.radius});
            shadow.setPosition({prop.position.x + 6.0f, prop.position.y + 6.0f});
            shadow.setFillColor(sf::Color(0, 0, 0, 50));
            window.draw(shadow);

            sf::CircleShape tree(prop.radius);
            tree.setOrigin({prop.radius, prop.radius});
            tree.setPosition(prop.position);
            tree.setFillColor(prop.color);
            tree.setOutlineThickness(2.0f);
            tree.setOutlineColor(sf::Color(24, 65, 32));
            window.draw(tree);
        }
    }

    // Šaligatviai
    const float sidewalkW = 24.0f;
    sf::RectangleShape leftSidewalk({sidewalkW, static_cast<float>(WINDOW_HEIGHT)});
    leftSidewalk.setPosition({ROAD_LEFT - sidewalkW, 0.0f});
    leftSidewalk.setFillColor(sf::Color(118, 125, 134));
    window.draw(leftSidewalk);

    sf::RectangleShape rightSidewalk({sidewalkW, static_cast<float>(WINDOW_HEIGHT)});
    rightSidewalk.setPosition({ROAD_RIGHT, 0.0f});
    rightSidewalk.setFillColor(sf::Color(118, 125, 134));
    window.draw(rightSidewalk);

    // Asfaltas
    sf::RectangleShape asphalt({ROAD_WIDTH, static_cast<float>(WINDOW_HEIGHT)});
    asphalt.setPosition({ROAD_LEFT, 0.0f});
    asphalt.setFillColor(sf::Color(50, 54, 62));
    window.draw(asphalt);

    // Kraštinės geltonos linijos
    sf::RectangleShape leftLine({5.0f, static_cast<float>(WINDOW_HEIGHT)});
    leftLine.setPosition({ROAD_LEFT + 6.0f, 0.0f});
    leftLine.setFillColor(sf::Color(236, 188, 52));
    window.draw(leftLine);

    sf::RectangleShape rightLine({5.0f, static_cast<float>(WINDOW_HEIGHT)});
    rightLine.setPosition({ROAD_RIGHT - 11.0f, 0.0f});
    rightLine.setFillColor(sf::Color(236, 188, 52));
    window.draw(rightLine);

    // Punktyrinės juostų linijos
    for (int lane = 1; lane < LANE_COUNT; ++lane) {
        const float x = ROAD_LEFT + static_cast<float>(lane) * LANE_WIDTH - 3.0f;
        for (float y = -90.0f + game.roadOffset; y < static_cast<float>(WINDOW_HEIGHT) + 90.0f; y += 90.0f) {
            sf::RectangleShape dash({6.0f, 44.0f});
            dash.setPosition({x, y});
            dash.setFillColor(sf::Color(232, 236, 240, 210));
            window.draw(dash);
        }
    }

    // Greičio linijos šonuose, kai laikomas W arba veikia Žalia banga
    if (game.state == GameState::Playing && game.speedMultiplier > 1.2f) {
        const std::uint8_t lineAlpha = (game.player.greenWaveTimer > 0.0f) ? 140 : 85;
        const sf::Color lineColor = (game.player.greenWaveTimer > 0.0f)
            ? sf::Color(120, 255, 175, lineAlpha)
            : sf::Color(235, 245, 255, lineAlpha);

        for (int i = 0; i < 6; ++i) {
            const float y = std::fmod(game.roadOffset * 2.4f + static_cast<float>(i) * 130.0f, static_cast<float>(WINDOW_HEIGHT) + 80.0f) - 40.0f;
            sf::RectangleShape leftStreak({3.0f, 52.0f});
            leftStreak.setPosition({ROAD_LEFT + 18.0f, y});
            leftStreak.setFillColor(lineColor);
            window.draw(leftStreak);

            sf::RectangleShape rightStreak({3.0f, 52.0f});
            rightStreak.setPosition({ROAD_RIGHT - 21.0f, y + 40.0f});
            rightStreak.setFillColor(lineColor);
            window.draw(rightStreak);
        }
    }
}

void renderEntities(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    // 1. Bonusai su švelniu pulsavimu
    for (const BonusItem& bonus : game.bonuses) {
        if (!bonus.active) continue;

        const float pulse = 1.0f + 0.08f * std::sin(bonus.pulsePhase);
        const float drawSize = bonus.radius * 2.25f * pulse;
        const sf::Vector2f topLeft = {bonus.position.x - drawSize * 0.5f, bonus.position.y - drawSize * 0.5f};

        if (bonus.type == BonusType::Coffee && res.hasCoffee) {
            drawSpriteFit(window, res.coffee, topLeft, {drawSize, drawSize});
        } else if (bonus.type == BonusType::GreenWave && res.hasGreenWave) {
            drawSpriteFit(window, res.greenWave, topLeft, {drawSize, drawSize});
        } else if (bonus.type == BonusType::UsbDrive && res.hasUsb) {
            drawSpriteFit(window, res.usb, topLeft, {drawSize, drawSize});
        } else {
            sf::CircleShape fallback(bonus.radius * pulse);
            fallback.setOrigin({bonus.radius * pulse, bonus.radius * pulse});
            fallback.setPosition(bonus.position);
            fallback.setFillColor(bonus.type == BonusType::Coffee ? sf::Color(180, 115, 50)
                : (bonus.type == BonusType::GreenWave ? sf::Color(46, 204, 113) : sf::Color(52, 152, 219)));
            window.draw(fallback);
        }
    }

    // 2. Kliūtys (su paveikslėliais ir atsarginiu piešimu)
    for (const Obstacle& obs : game.obstacles) {
        if (obs.type == ObstacleType::Roadworks) {
            if (res.hasRoadworks) drawSpriteFit(window, res.roadworks, obs.position, obs.size);
            else drawFallbackRect(window, obs.position, obs.size, obs.color);
        } else if (obs.type == ObstacleType::Trolleybus) {
            if (res.hasTrolleybus) drawSpriteFit(window, res.trolleybus, obs.position, obs.size);
            else drawFallbackRect(window, obs.position, obs.size, obs.color);
        } else if (obs.type == ObstacleType::Scooter) {
            if (res.hasScooter) drawSpriteFit(window, res.scooter, obs.position, obs.size);
            else drawFallbackRect(window, obs.position, obs.size, obs.color);
        } else if (obs.type == ObstacleType::ChaoticDriver) {
            if (res.hasSportCar) drawSpriteFit(window, res.sportCar, obs.position, obs.size);
            else drawFallbackRect(window, obs.position, obs.size, obs.color);

            const bool blinkPhase = (static_cast<int>(obs.signalTimer * 12.0f) % 2 == 0);
            const bool signalingActive = obs.isSignaling && blinkPhase;
            drawBlinkers(window, obs.position, obs.size,
                         signalingActive && (obs.targetLane < obs.currentLane),
                         signalingActive && (obs.targetLane > obs.currentLane));
        } else {
            if (!res.trafficCars.empty()) {
                const std::size_t idx = static_cast<std::size_t>(obs.textureIndex) % res.trafficCars.size();
                drawSpriteFit(window, res.trafficCars[idx], obs.position, obs.size);
            } else {
                drawFallbackRect(window, obs.position, obs.size, obs.color);
            }
        }
    }

    // 3. Žaidėjo automobilis – žalias Arto pavežėjas
    if (game.player.greenWaveTimer > 0.0f) {
        // Žalios bangos apsauginė aura
        const float auraPulse = 8.0f + 4.0f * std::sin(game.totalElapsed * 12.0f);
        sf::RectangleShape aura({game.player.size.x + auraPulse * 2.0f, game.player.size.y + auraPulse * 2.0f});
        aura.setPosition({game.player.position.x - auraPulse, game.player.position.y - auraPulse});
        aura.setFillColor(sf::Color(46, 240, 128, 70));
        aura.setOutlineThickness(3.0f);
        aura.setOutlineColor(sf::Color(145, 255, 185, 200));
        window.draw(aura);
    }

    std::uint8_t playerAlpha = 255;
    if (game.player.invincibilityTimer > 0.0f && game.player.greenWaveTimer <= 0.0f) {
        const bool dimFrame = (static_cast<int>(game.player.invincibilityTimer * 14.0f) % 2 == 0);
        playerAlpha = dimFrame ? 115 : 235;
    }

    if (res.hasPlayerCar) {
        drawSpriteFit(window, res.playerCar, game.player.position, game.player.size, playerAlpha);
    } else {
        drawFallbackRect(window, game.player.position, game.player.size, sf::Color(46, 204, 113), playerAlpha);
    }
}

void renderParticles(sf::RenderWindow& window, const GameData& game) {
    for (const Particle& p : game.particles) {
        const float ratio = std::clamp(p.lifetime / std::max(0.01f, p.maxLifetime), 0.0f, 1.0f);
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(255.0f * ratio);

        sf::CircleShape dot(p.size * ratio);
        dot.setOrigin({p.size * ratio, p.size * ratio});
        dot.setPosition(p.position);
        dot.setFillColor(c);
        window.draw(dot);
    }
}

void renderHUD(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    const RouteConfig route = getRouteConfig(game.selectedRoute);

    if (game.screenFlashTimer > 0.0f) {
        const std::uint8_t alpha = static_cast<std::uint8_t>(std::min(130.0f, game.screenFlashTimer * 360.0f));
        sf::RectangleShape flash({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        flash.setFillColor(sf::Color(220, 40, 40, alpha));
        window.draw(flash);
    }

    // --- KAIRĖS PUSĖS KORTELĖ: Maršrutas ir Vėlavimas ---
    drawPanelBox(window, {18.0f, 18.0f}, {306.0f, 278.0f});

    sf::Text headerLeft(res.font, toSfString("MARŠRUTAS Į PASKAITĄ"), 17);
    headerLeft.setStyle(sf::Text::Bold);
    headerLeft.setFillColor(sf::Color(150, 195, 255));
    headerLeft.setPosition({34.0f, 30.0f});
    window.draw(headerLeft);

    sf::Text routeNameText(res.font, toSfString(route.name), 13);
    routeNameText.setFillColor(sf::Color(195, 210, 230));
    routeNameText.setPosition({34.0f, 54.0f});
    window.draw(routeNameText);

    sf::Text distLabel(res.font, toSfString("Atstumas: " + formatDistance(game.distanceKm, route.targetDistanceKm)), 16);
    distLabel.setFillColor(sf::Color::White);
    distLabel.setPosition({34.0f, 80.0f});
    window.draw(distLabel);

    // Progreso juosta su mini automobiliuko žymekliu
    const float barW = 270.0f;
    sf::RectangleShape barBg({barW, 18.0f});
    barBg.setPosition({34.0f, 110.0f});
    barBg.setFillColor(sf::Color(42, 50, 64));
    barBg.setOutlineThickness(1.0f);
    barBg.setOutlineColor(sf::Color(95, 112, 138));
    window.draw(barBg);

    const float progress = std::clamp(game.distanceKm / route.targetDistanceKm, 0.0f, 1.0f);
    sf::RectangleShape barFill({barW * progress, 18.0f});
    barFill.setPosition({34.0f, 110.0f});
    barFill.setFillColor(sf::Color(46, 204, 113));
    window.draw(barFill);

    // Mažas baltas automobiliuko žymeklis ant juostos
    sf::CircleShape marker(6.0f);
    marker.setOrigin({6.0f, 6.0f});
    marker.setPosition({34.0f + barW * progress, 119.0f});
    marker.setFillColor(sf::Color(255, 240, 130));
    marker.setOutlineThickness(1.5f);
    marker.setOutlineColor(sf::Color(20, 28, 38));
    window.draw(marker);

    sf::Text delayTitle(res.font, toSfString("Vėlavimas į paskaitą:"), 15);
    delayTitle.setFillColor(sf::Color(190, 200, 215));
    delayTitle.setPosition({34.0f, 144.0f});
    window.draw(delayTitle);

    sf::Text delayValue(res.font, toSfString(formatDelayTime(game.delaySeconds)), 25);
    delayValue.setStyle(sf::Text::Bold);
    delayValue.setFillColor(getDelayColor(game.delaySeconds, route.maxDelayMinutes));
    delayValue.setPosition({34.0f, 168.0f});
    window.draw(delayValue);

    const int speedKmh = static_cast<int>(game.currentRoadSpeed * game.speedMultiplier * 0.16f);
    sf::Text speedText(res.font, toSfString("Greitis: " + std::to_string(speedKmh) + " km/h"), 16);
    speedText.setFillColor(game.player.greenWaveTimer > 0.0f ? sf::Color(115, 255, 175) : sf::Color(215, 225, 240));
    speedText.setPosition({34.0f, 212.0f});
    window.draw(speedText);

    sf::Text limitText(res.font, toSfString("Riba: " + std::to_string(static_cast<int>(route.maxDelayMinutes)) + " min (tada studentai išeina)"), 12);
    limitText.setFillColor(sf::Color(150, 162, 180));
    limitText.setPosition({34.0f, 244.0f});
    window.draw(limitText);

    // Žalios bangos aktyvaus laikmačio juostelė
    if (game.player.greenWaveTimer > 0.0f) {
        drawPanelBox(window, {18.0f, 308.0f}, {306.0f, 54.0f}, sf::Color(88, 240, 145));
        sf::Text gwLabel(res.font, toSfString("ŽALIA BANGA AKTYVI!"), 14);
        gwLabel.setStyle(sf::Text::Bold);
        gwLabel.setFillColor(sf::Color(115, 255, 175));
        gwLabel.setPosition({34.0f, 316.0f});
        window.draw(gwLabel);

        const float gwRatio = std::clamp(game.player.greenWaveTimer / GREEN_WAVE_DURATION, 0.0f, 1.0f);
        sf::RectangleShape gwBar({270.0f * gwRatio, 10.0f});
        gwBar.setPosition({34.0f, 342.0f});
        gwBar.setFillColor(sf::Color(88, 240, 145));
        window.draw(gwBar);
    }

    // --- DEŠINĖS PUSĖS KORTELĖ: Arto kantrybė, Taškai, Valdymas ---
    drawPanelBox(window, {956.0f, 18.0f}, {306.0f, 305.0f});

    sf::Text headerRight(res.font, toSfString("PAVEŽĖJO BŪSENA"), 17);
    headerRight.setStyle(sf::Text::Bold);
    headerRight.setFillColor(sf::Color(150, 195, 255));
    headerRight.setPosition({972.0f, 30.0f});
    window.draw(headerRight);

    sf::Text livesLabel(res.font, toSfString("Arto kantrybė:"), 15);
    livesLabel.setFillColor(sf::Color::White);
    livesLabel.setPosition({972.0f, 64.0f});
    window.draw(livesLabel);

    for (int i = 0; i < PLAYER_MAX_LIVES; ++i) {
        if (res.hasArtasFaces) {
            const sf::Vector2f iconPos = {974.0f + static_cast<float>(i) * 52.0f, 88.0f};
            const std::uint8_t alpha = (i < game.player.lives) ? 255 : 48;
            drawSpriteFit(window, res.artasHappy, iconPos, {40.0f, 40.0f}, alpha);
        } else {
            sf::RectangleShape lifeBar({76.0f, 18.0f});
            lifeBar.setPosition({972.0f + static_cast<float>(i) * 86.0f, 96.0f});
            lifeBar.setFillColor(i < game.player.lives ? sf::Color(231, 76, 60) : sf::Color(55, 60, 70));
            window.draw(lifeBar);
        }
    }

    sf::Text scoreText(res.font, toSfString("Taškai: " + std::to_string(game.score)), 22);
    scoreText.setStyle(sf::Text::Bold);
    scoreText.setFillColor(sf::Color(248, 214, 96));
    scoreText.setPosition({972.0f, 142.0f});
    window.draw(scoreText);

    sf::Text statsText(res.font,
        toSfString("Aplenktos kliūtys: " + std::to_string(game.dodgedCount) +
                   "\nKavos: " + std::to_string(game.coffeesCollected) +
                   "  |  USB: " + std::to_string(game.usbCollected)),
        14);
    statsText.setFillColor(sf::Color(200, 210, 224));
    statsText.setPosition({972.0f, 180.0f});
    window.draw(statsText);

    sf::Text controlsText(res.font,
        toSfString("Valdymas:\nA / D arba <- / ->  juostos\nW / S arba ^ / v    greitis\nP / Esc             pauzė\nM                   garsas"),
        13);
    controlsText.setFillColor(sf::Color(155, 170, 190));
    controlsText.setPosition({972.0f, 226.0f});
    window.draw(controlsText);

    // --- ARTO KALBOS BURBULAS ŠALIA AUTOMOBILIO ---
    drawCarSpeechBubble(window, game, res);
}

void renderGame(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    window.clear(sf::Color(20, 26, 34));

    // Pasaulio vaizdas su galimu ekrano drebėjimu po avarijos
    const sf::Vector2f baseCenter = {static_cast<float>(WINDOW_WIDTH) * 0.5f, static_cast<float>(WINDOW_HEIGHT) * 0.5f};
    const sf::Vector2f viewSize = {static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)};

    sf::View worldView(baseCenter + getScreenShakeOffset(game), viewSize);
    window.setView(worldView);

    renderRoad(window, game);
    if (game.state == GameState::Countdown ||
        game.state == GameState::Playing ||
        game.state == GameState::Paused ||
        game.state == GameState::NameInput ||
        game.state == GameState::Victory ||
        game.state == GameState::GameOver) {
        renderEntities(window, game, res);
        renderParticles(window, game);
    }

    // Sąsajos (HUD ir meniu) vaizdas visada stabilus (be drebėjimo)
    const sf::View uiView(baseCenter, viewSize);
    window.setView(uiView);
    const sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window), uiView);

    if (game.state == GameState::Countdown ||
        game.state == GameState::Playing ||
        game.state == GameState::Paused) {
        renderHUD(window, game, res);
    }

    if (game.state == GameState::Menu) {
        renderMenuOverlay(window, res, mousePos);
    } else if (game.state == GameState::RouteSelect) {
        renderRouteSelectOverlay(window, res, mousePos);
    } else if (game.state == GameState::HowToPlay) {
        renderHowToPlayOverlay(window, res, mousePos);
    } else if (game.state == GameState::HighScores) {
        renderHighScoresOverlay(window, game, res, mousePos);
    } else if (game.state == GameState::Countdown) {
        renderCountdownOverlay(window, game, res);
    } else if (game.state == GameState::Paused) {
        renderPauseOverlay(window, res, mousePos);
    } else if (game.state == GameState::NameInput) {
        renderNameInputOverlay(window, game, res, mousePos);
    } else if (game.state == GameState::Victory || game.state == GameState::GameOver) {
        renderEndOverlay(window, game, res, mousePos);
    }

    drawMuteButton(window, game, res.font, mousePos);
    window.display();
}
