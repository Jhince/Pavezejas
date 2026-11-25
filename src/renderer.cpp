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

// ============================================================================
// Moderni, sodri spalvų paletė (be išblukusių pilkų tonų)
// ============================================================================
const sf::Color COL_PANEL_BG(11, 17, 32, 238);
const sf::Color COL_PANEL_INNER(20, 30, 52, 220);
const sf::Color COL_BORDER_SUBTLE(72, 102, 148, 175);
const sf::Color COL_EMERALD(0, 214, 125);
const sf::Color COL_GOLD(255, 196, 36);
const sf::Color COL_CYAN(56, 189, 248);
const sf::Color COL_CORAL(244, 63, 94);
const sf::Color COL_TEXT_MAIN(244, 248, 255);
const sf::Color COL_TEXT_MUTED(158, 180, 214);

std::string formatDelayTime(float totalSeconds) {
    const int secs = std::max(0, static_cast<int>(totalSeconds));
    const int mins = secs / 60;
    const int remSecs = secs % 60;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << mins << ":"
        << std::setfill('0') << std::setw(2) << remSecs;
    return oss.str();
}

std::string formatDelayVerbose(float totalSeconds) {
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
    if (ratio < 0.50f) return COL_EMERALD;
    if (ratio < 0.75f) return COL_GOLD;
    if (ratio < 0.90f) return sf::Color(251, 146, 60);
    return COL_CORAL;
}

sf::Color brightenColor(sf::Color c, int delta) {
    return sf::Color(
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(c.r) + delta, 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(c.g) + delta, 0, 255)),
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(c.b) + delta, 0, 255)),
        c.a
    );
}

// Sukuria suapvalinto stačiakampio figūrą (moderniai UI išvaizdai)
sf::ConvexShape makeRoundedRect(sf::Vector2f pos, sf::Vector2f size, float radius, int cornerSegments = 6) {
    const float r = std::min(radius, std::min(size.x, size.y) * 0.5f);
    sf::ConvexShape shape;
    const std::size_t totalPoints = static_cast<std::size_t>(cornerSegments * 4);
    shape.setPointCount(totalPoints);

    constexpr float PI = 3.14159265f;
    const sf::Vector2f centers[4] = {
        {pos.x + size.x - r, pos.y + r},          // Viršus-dešinė
        {pos.x + size.x - r, pos.y + size.y - r}, // Apačia-dešinė
        {pos.x + r,          pos.y + size.y - r}, // Apačia-kairė
        {pos.x + r,          pos.y + r}           // Viršus-kairė
    };
    const float startAngles[4] = {-PI * 0.5f, 0.0f, PI * 0.5f, PI};

    std::size_t idx = 0;
    for (int corner = 0; corner < 4; ++corner) {
        for (int s = 0; s < cornerSegments; ++s) {
            const float frac = static_cast<float>(s) / static_cast<float>(cornerSegments - 1);
            const float angle = startAngles[corner] + frac * (PI * 0.5f);
            shape.setPoint(idx++, {
                centers[corner].x + std::cos(angle) * r,
                centers[corner].y + std::sin(angle) * r
            });
        }
    }
    return shape;
}

void drawRoundedRect(sf::RenderWindow& window,
                     sf::Vector2f pos,
                     sf::Vector2f size,
                     float radius,
                     sf::Color fill,
                     float outlineThickness = 0.0f,
                     sf::Color outlineColor = sf::Color::Transparent) {
    sf::ConvexShape shape = makeRoundedRect(pos, size, radius);
    shape.setFillColor(fill);
    if (outlineThickness > 0.0f) {
        shape.setOutlineThickness(outlineThickness);
        shape.setOutlineColor(outlineColor);
    }
    window.draw(shape);
}

void drawShadowedText(sf::RenderWindow& window,
                      const sf::Font& font,
                      const std::string& text,
                      unsigned int fontSize,
                      sf::Color color,
                      sf::Vector2f pos,
                      bool bold = false) {
    sf::Text shadow(font, toSfString(text), fontSize);
    if (bold) shadow.setStyle(sf::Text::Bold);
    shadow.setFillColor(sf::Color(0, 0, 0, 155));
    shadow.setPosition({pos.x + 2.0f, pos.y + 2.0f});
    window.draw(shadow);

    sf::Text mainText(font, toSfString(text), fontSize);
    if (bold) mainText.setStyle(sf::Text::Bold);
    mainText.setFillColor(color);
    mainText.setPosition(pos);
    window.draw(mainText);
}

void drawShadowedCenteredText(sf::RenderWindow& window,
                              const sf::Font& font,
                              const std::string& text,
                              unsigned int fontSize,
                              sf::Color color,
                              sf::Vector2f centerPos,
                              bool bold = false) {
    sf::Text shadow(font, toSfString(text), fontSize);
    if (bold) shadow.setStyle(sf::Text::Bold);
    shadow.setFillColor(sf::Color(0, 0, 0, 175));
    const sf::FloatRect b = shadow.getLocalBounds();
    shadow.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    shadow.setPosition({centerPos.x + 2.0f, centerPos.y + 2.5f});
    window.draw(shadow);

    sf::Text mainText(font, toSfString(text), fontSize);
    if (bold) mainText.setStyle(sf::Text::Bold);
    mainText.setFillColor(color);
    mainText.setOrigin({b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f});
    mainText.setPosition(centerPos);
    window.draw(mainText);
}

// Moderni suapvalinta stiklinė kortelė su šešėliu ir spalvota viršutine akcento juosta
void drawGlassPanel(sf::RenderWindow& window,
                    sf::Vector2f pos,
                    sf::Vector2f size,
                    sf::Color accentColor = COL_CYAN) {
    // Minkštas šešėlis
    drawRoundedRect(window, {pos.x + 6.0f, pos.y + 8.0f}, size, 16.0f, sf::Color(0, 0, 0, 125));

    // Pagrindinis tamsus stiklinis fonas
    drawRoundedRect(window, pos, size, 16.0f, COL_PANEL_BG, 1.8f, COL_BORDER_SUBTLE);

    // Subtilus viršutinis vidinis švytėjimas
    drawRoundedRect(window, {pos.x + 12.0f, pos.y + 2.0f}, {size.x - 24.0f, 4.0f}, 2.0f, accentColor);
}

// Modernus 3D efekto suapvalintas mygtukas su hover paryškinimu
void drawButton(sf::RenderWindow& window,
                const sf::Font& font,
                const ButtonRect& button,
                sf::Vector2f mousePos) {
    const bool hovered = button.bounds.contains(mousePos);
    const float liftY = hovered ? -2.0f : 0.0f;
    const sf::Vector2f pos = {button.bounds.position.x, button.bounds.position.y + liftY};
    const sf::Vector2f size = button.bounds.size;

    // Apatinis 3D gylio šešėlis
    drawRoundedRect(window, {pos.x, pos.y + 5.0f}, size, 12.0f,
                    brightenColor(button.baseColor, -55));

    // Pagrindinis mygtuko kūnas
    const sf::Color fill = hovered ? brightenColor(button.baseColor, 26) : button.baseColor;
    const sf::Color border = hovered ? sf::Color(255, 255, 255, 235) : brightenColor(button.baseColor, 48);
    drawRoundedRect(window, pos, size, 12.0f, fill, hovered ? 2.5f : 1.5f, border);

    // Viršutinė blizgesio juostelė
    drawRoundedRect(window, {pos.x + 8.0f, pos.y + 3.0f}, {size.x - 16.0f, size.y * 0.36f}, 8.0f,
                    sf::Color(255, 255, 255, hovered ? 42 : 26));

    const sf::Vector2f center = {pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
    drawShadowedCenteredText(window, font, button.label, 18, sf::Color::White, center, true);
}

void drawKeyCap(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f pos, float width, const std::string& keyLabel, const std::string& desc) {
    drawRoundedRect(window, pos, {width, 22.0f}, 6.0f, sf::Color(30, 44, 72), 1.2f, sf::Color(102, 136, 186));
    drawCenteredText(window, font, keyLabel, 11, COL_GOLD, {pos.x + width * 0.5f, pos.y + 11.0f});

    sf::Text d(font, toSfString(desc), 12);
    d.setFillColor(COL_TEXT_MUTED);
    d.setPosition({pos.x + width + 8.0f, pos.y + 3.0f});
    window.draw(d);
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
    auto drawGlowBlinker = [&](sf::Vector2f p) {
        sf::CircleShape glow(12.0f);
        glow.setOrigin({12.0f, 12.0f});
        glow.setPosition(p);
        glow.setFillColor(sf::Color(255, 165, 0, 105));
        window.draw(glow);

        sf::CircleShape core(6.0f);
        core.setOrigin({6.0f, 6.0f});
        core.setPosition(p);
        core.setFillColor(sf::Color(255, 200, 40));
        window.draw(core);
    };

    if (blinkLeft) {
        drawGlowBlinker({pos.x + 1.0f, pos.y + 14.0f});
        drawGlowBlinker({pos.x + 1.0f, pos.y + size.y - 14.0f});
    }
    if (blinkRight) {
        drawGlowBlinker({pos.x + size.x - 1.0f, pos.y + 14.0f});
        drawGlowBlinker({pos.x + size.x - 1.0f, pos.y + size.y - 14.0f});
    }
}

void drawFallbackRect(sf::RenderWindow& window, sf::Vector2f pos, sf::Vector2f size, sf::Color color, std::uint8_t alpha = 255) {
    color.a = alpha;
    drawRoundedRect(window, pos, size, 10.0f, color, 2.0f, sf::Color(15, 20, 30, alpha));
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
    const std::uint8_t bgAlpha = static_cast<std::uint8_t>(248.0f * fadeRatio);
    const std::uint8_t fgAlpha = static_cast<std::uint8_t>(255.0f * fadeRatio);

    const std::string wrapped = wrapTextToWidth(res.font, game.statusMessage, 14, 215.0f);

    sf::Text speakerText(res.font, toSfString("DĖSTYTOJAS"), 11);
    speakerText.setStyle(sf::Text::Bold);
    speakerText.setFillColor(sf::Color(255, 255, 255, fgAlpha));

    sf::Text bodyText(res.font, toSfString(wrapped), 14);
    bodyText.setFillColor(sf::Color(15, 23, 42, fgAlpha));

    const sf::FloatRect speakerBounds = speakerText.getLocalBounds();
    const sf::FloatRect bodyBounds = bodyText.getLocalBounds();

    const float avatarSize = res.hasLecturerFaces ? 46.0f : 0.0f;
    const float avatarGap = res.hasLecturerFaces ? 10.0f : 0.0f;
    const float padX = 14.0f;
    const float padY = 12.0f;
    const float pillH = 20.0f;
    const float gapY = 7.0f;

    const float pillW = speakerBounds.size.x + 16.0f;
    const float textW = std::max(pillW, bodyBounds.size.x);
    const float textH = pillH + gapY + bodyBounds.size.y + 4.0f;
    const float bubbleW = std::max(185.0f, avatarSize + avatarGap + textW + padX * 2.0f);
    const float bubbleH = std::max(avatarSize, textH) + padY * 2.0f;

    const float carCenterX = game.player.position.x + game.player.size.x * 0.5f;
    const float carBackSeatY = game.player.position.y + game.player.size.y * 0.62f;

    bool placeLeft = (carCenterX < static_cast<float>(WINDOW_WIDTH) * 0.5f);
    const float tailGap = 16.0f;

    float bubbleX = placeLeft
        ? (game.player.position.x - tailGap - bubbleW)
        : (game.player.position.x + game.player.size.x + tailGap);
    float bubbleY = carBackSeatY - bubbleH * 0.55f;

    if (placeLeft && bubbleX < 332.0f && bubbleY < 312.0f) {
        placeLeft = false;
        bubbleX = game.player.position.x + game.player.size.x + tailGap;
    } else if (!placeLeft && (bubbleX + bubbleW) > 948.0f && bubbleY < 342.0f) {
        placeLeft = true;
        bubbleX = game.player.position.x - tailGap - bubbleW;
    }

    bubbleX = std::clamp(bubbleX, 16.0f, static_cast<float>(WINDOW_WIDTH) - bubbleW - 16.0f);
    bubbleY = std::clamp(bubbleY, 16.0f, static_cast<float>(WINDOW_HEIGHT) - bubbleH - 16.0f);

    // Šešėlis po burbulu
    drawRoundedRect(window, {bubbleX + 4.0f, bubbleY + 5.0f}, {bubbleW, bubbleH}, 14.0f,
                    sf::Color(0, 0, 0, static_cast<std::uint8_t>(95.0f * fadeRatio)));

    // Kalbos burbulo uodegėlė
    sf::ConvexShape tail;
    tail.setPointCount(3);
    const float tailMidY = std::clamp(carBackSeatY, bubbleY + 18.0f, bubbleY + bubbleH - 18.0f);
    if (placeLeft) {
        tail.setPoint(0, {bubbleX + bubbleW - 2.0f, tailMidY - 8.0f});
        tail.setPoint(1, {bubbleX + bubbleW - 2.0f, tailMidY + 8.0f});
        tail.setPoint(2, {game.player.position.x - 2.0f, carBackSeatY});
    } else {
        tail.setPoint(0, {bubbleX + 2.0f, tailMidY - 8.0f});
        tail.setPoint(1, {bubbleX + 2.0f, tailMidY + 8.0f});
        tail.setPoint(2, {game.player.position.x + game.player.size.x + 2.0f, carBackSeatY});
    }
    tail.setFillColor(sf::Color(248, 250, 255, bgAlpha));
    tail.setOutlineThickness(2.0f);
    tail.setOutlineColor(sf::Color(15, 23, 42, bgAlpha));
    window.draw(tail);

    // Suapvalintas burbulo kūnas
    drawRoundedRect(window, {bubbleX, bubbleY}, {bubbleW, bubbleH}, 14.0f,
                    sf::Color(248, 250, 255, bgAlpha), 2.0f, sf::Color(15, 23, 42, bgAlpha));

    // Vientisas uodegėlės sujungimas
    sf::RectangleShape seamCover({5.0f, 14.0f});
    seamCover.setPosition({placeLeft ? (bubbleX + bubbleW - 4.0f) : (bubbleX - 1.0f), tailMidY - 7.0f});
    seamCover.setFillColor(sf::Color(248, 250, 255, bgAlpha));
    window.draw(seamCover);

    // Dėstytojo portretas pagal nuotaiką
    if (res.hasLecturerFaces) {
        const sf::Texture& face = (game.player.lives >= 3)
            ? res.lecturerHappy
            : (game.player.lives == 2 ? res.lecturerNeutral : res.lecturerAngry);
        drawSpriteFit(window, face, {bubbleX + padX, bubbleY + (bubbleH - avatarSize) * 0.5f},
                      {avatarSize, avatarSize}, fgAlpha);
    }

    // Spalvota „DĖSTYTOJAS“ žymeklio kapsulė
    const float textStartX = bubbleX + padX + avatarSize + avatarGap;
    const sf::Color badgeCol = (game.player.lives >= 3)
        ? sf::Color(0, 178, 102, fgAlpha)
        : (game.player.lives == 2 ? sf::Color(217, 119, 6, fgAlpha) : sf::Color(225, 29, 72, fgAlpha));
    drawRoundedRect(window, {textStartX, bubbleY + padY}, {pillW, pillH}, 7.0f, badgeCol);

    speakerText.setPosition({textStartX + 8.0f - speakerBounds.position.x,
                             bubbleY + padY + (pillH - speakerBounds.size.y) * 0.5f - speakerBounds.position.y});
    bodyText.setPosition({textStartX - bodyBounds.position.x,
                          bubbleY + padY + pillH + gapY - bodyBounds.position.y});

    window.draw(speakerText);
    window.draw(bodyText);
}

void drawMuteButton(sf::RenderWindow& window, const GameData& game, const sf::Font& font, sf::Vector2f mousePos) {
    const sf::FloatRect bounds = getMuteButtonBounds();
    const bool hovered = bounds.contains(mousePos);

    const sf::Color bg = game.soundMuted ? sf::Color(159, 18, 57, 235) : sf::Color(15, 23, 42, 235);
    const sf::Color border = hovered ? sf::Color::White : (game.soundMuted ? COL_CORAL : COL_EMERALD);
    drawRoundedRect(window, bounds.position, bounds.size, 10.0f, bg, hovered ? 2.2f : 1.5f, border);

    const std::string label = game.soundMuted ? "Garsas: OFF (M)" : "Garsas: ON (M)";
    drawCenteredText(window, font, label, 13, COL_TEXT_MAIN,
                     {bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f});
}

void renderMenuOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 175));
    window.draw(dim);

    drawGlassPanel(window, {300.0f, 68.0f}, {680.0f, 558.0f}, COL_EMERALD);

    // Žėrintis apskritimas aplink dėstytojo portretą
    if (res.hasLecturerFaces) {
        sf::CircleShape ring(48.0f);
        ring.setOrigin({48.0f, 48.0f});
        ring.setPosition({640.0f, 134.0f});
        ring.setFillColor(sf::Color(0, 214, 125, 45));
        ring.setOutlineThickness(2.5f);
        ring.setOutlineColor(COL_EMERALD);
        window.draw(ring);

        drawSpriteFit(window, res.lecturerHappy, {598.0f, 92.0f}, {84.0f, 84.0f});
    }

    drawShadowedCenteredText(window, res.font, "20 MINUČIŲ IKI PASKAITOS", 34, COL_GOLD, {640.0f, 212.0f}, true);

    // Spalvota paantraštės kapsulė
    drawRoundedRect(window, {375.0f, 240.0f}, {530.0f, 34.0f}, 17.0f, sf::Color(0, 214, 125, 34), 1.5f, sf::Color(0, 214, 125, 160));
    drawCenteredText(window, res.font, "Esi dėstytojo pavežėjas – nuvežk jį į Saulėtekį laiku!", 15, sf::Color(167, 243, 208), {640.0f, 257.0f});

    drawCenteredText(window, res.font, "WASD / Rodyklės – vairuoti   |   P / Esc – pauzė   |   M – garsas", 13, COL_TEXT_MUTED, {640.0f, 304.0f});

    for (const ButtonRect& btn : getScreenButtons(GameState::Menu)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderRouteSelectOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 185));
    window.draw(dim);

    drawGlassPanel(window, {285.0f, 80.0f}, {710.0f, 545.0f}, COL_CYAN);
    drawShadowedCenteredText(window, res.font, "PASIRINK MARŠRUTĄ", 32, COL_GOLD, {640.0f, 130.0f}, true);
    drawCenteredText(window, res.font, "Spausk ant kortelės arba 1 / 2 / 3 klaviatūroje", 14, COL_TEXT_MUTED, {640.0f, 168.0f});

    const std::vector<ButtonRect> buttons = getScreenButtons(GameState::RouteSelect);
    for (const ButtonRect& btn : buttons) {
        if (btn.actionId >= 10 && btn.actionId <= 12) {
            const RouteConfig route = getRouteConfig(btn.actionId - 10);
            const bool hovered = btn.bounds.contains(mousePos);
            const float liftY = hovered ? -3.0f : 0.0f;
            const sf::Vector2f pos = {btn.bounds.position.x, btn.bounds.position.y + liftY};

            // Kortelės šešėlis
            drawRoundedRect(window, {pos.x, pos.y + 5.0f}, btn.bounds.size, 14.0f, brightenColor(btn.baseColor, -65));

            // Pagrindinė kortelė
            const sf::Color fill = hovered ? brightenColor(btn.baseColor, 22) : btn.baseColor;
            drawRoundedRect(window, pos, btn.bounds.size, 14.0f, fill, hovered ? 2.5f : 1.5f,
                            hovered ? sf::Color::White : brightenColor(btn.baseColor, 55));

            // Viršutinis blizgesys
            drawRoundedRect(window, {pos.x + 10.0f, pos.y + 3.0f}, {btn.bounds.size.x - 20.0f, 26.0f}, 8.0f,
                            sf::Color(255, 255, 255, hovered ? 36 : 22));

            drawShadowedText(window, res.font, btn.label, 20, sf::Color::White, {pos.x + 22.0f, pos.y + 14.0f}, true);

            // Sunkumo ženkliukas dešinėje
            drawRoundedRect(window, {pos.x + btn.bounds.size.x - 165.0f, pos.y + 14.0f}, {145.0f, 26.0f}, 13.0f,
                            sf::Color(10, 16, 28, 155));
            drawCenteredText(window, res.font, route.difficulty, 12, COL_GOLD,
                             {pos.x + btn.bounds.size.x - 92.5f, pos.y + 27.0f});

            sf::Text desc(res.font, toSfString(route.description), 14);
            desc.setFillColor(sf::Color(238, 245, 255));
            desc.setPosition({pos.x + 22.0f, pos.y + 50.0f});
            window.draw(desc);
        } else {
            drawButton(window, res.font, btn, mousePos);
        }
    }
}

void renderHowToPlayOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 190));
    window.draw(dim);

    drawGlassPanel(window, {185.0f, 58.0f}, {910.0f, 608.0f}, sf::Color(168, 85, 247));
    drawShadowedCenteredText(window, res.font, "KAIP ŽAISTI", 32, COL_GOLD, {640.0f, 104.0f}, true);

    // Kairė vidinė kortelė: Kliūtys
    drawRoundedRect(window, {218.0f, 142.0f}, {410.0f, 412.0f}, 14.0f, COL_PANEL_INNER, 1.2f, sf::Color(244, 63, 94, 120));
    drawShadowedText(window, res.font, "KLIŪTYS KELYJE", 18, sf::Color(253, 164, 175), {240.0f, 158.0f}, true);

    const std::string obstaclesInfo =
        "- Sportiniai automobiliai:\n  pypia, mirksi posūkiu ir staigiai\n  persirikiuoja į gretimą juostą!\n\n"
        "- Raudoni troleibusai:\n  ilgi ir lėti – apvažiuok iš anksto.\n\n"
        "- Paspirtukininkai:\n  siauri, greiti ir vingiuoja juostoje.\n\n"
        "- Kelio darbai:\n  stacionarios oranžinės užtvaros.\n\n"
        "Avarija: -1 dėstytojo kantrybės\nir +1 min vėlavimo baudos!";

    sf::Text obsText(res.font, toSfString(obstaclesInfo), 14);
    obsText.setFillColor(COL_TEXT_MAIN);
    obsText.setPosition({240.0f, 196.0f});
    window.draw(obsText);

    // Dešinė vidinė kortelė: Bonusai ir valdymas
    drawRoundedRect(window, {652.0f, 142.0f}, {410.0f, 412.0f}, 14.0f, COL_PANEL_INNER, 1.2f, sf::Color(0, 214, 125, 120));
    drawShadowedText(window, res.font, "BONUSAI IR VALDYMAS", 18, sf::Color(110, 231, 183), {674.0f, 158.0f}, true);

    if (res.hasCoffee) drawSpriteFit(window, res.coffee, {674.0f, 198.0f}, {44.0f, 44.0f});
    sf::Text tCoffee(res.font, toSfString("Kava: +1 dėstytojo kantrybės,\n+150 tšk. ir -0.5 min vėlavimo."), 13);
    tCoffee.setFillColor(COL_TEXT_MAIN);
    tCoffee.setPosition({730.0f, 200.0f});
    window.draw(tCoffee);

    if (res.hasGreenWave) drawSpriteFit(window, res.greenWave, {674.0f, 260.0f}, {44.0f, 44.0f});
    sf::Text tWave(res.font, toSfString("Žalia banga: 3.5 s turbo greitis\nir laikinas imunitetas avarijoms!"), 13);
    tWave.setFillColor(COL_TEXT_MAIN);
    tWave.setPosition({730.0f, 262.0f});
    window.draw(tWave);

    if (res.hasUsb) drawSpriteFit(window, res.usb, {674.0f, 322.0f}, {44.0f, 44.0f});
    sf::Text tUsb(res.font, toSfString("USB su skaidrėmis: +300 taškų!\nDėstytojas labai apsidžiaugia."), 13);
    tUsb.setFillColor(COL_TEXT_MAIN);
    tUsb.setPosition({730.0f, 324.0f});
    window.draw(tUsb);

    drawKeyCap(window, res.font, {674.0f, 392.0f}, 86.0f, "A/D / <- ->", "keisti juostas");
    drawKeyCap(window, res.font, {674.0f, 424.0f}, 86.0f, "W/S / ^ v", "greitinti / stabdyti");
    drawKeyCap(window, res.font, {674.0f, 456.0f}, 86.0f, "P / ESC", "pauzė");
    drawKeyCap(window, res.font, {674.0f, 488.0f}, 86.0f, "M", "įjungti / išjungti garsą");

    for (const ButtonRect& btn : getScreenButtons(GameState::HowToPlay)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderHighScoresOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 190));
    window.draw(dim);

    drawGlassPanel(window, {215.0f, 68.0f}, {850.0f, 590.0f}, COL_GOLD);
    drawShadowedCenteredText(window, res.font, "GERIAUSI PAVEŽĖJAI (TOP 5)", 30, COL_GOLD, {640.0f, 118.0f}, true);

    static const std::string tabNames[ROUTE_COUNT] = {"1. Antakalnis", "2. Centras", "3. Pašilaičiai"};
    for (int r = 0; r < ROUTE_COUNT; ++r) {
        const sf::FloatRect tab = getHighScoreTabBounds(r);
        const bool active = (game.highScoreTab == r);
        const bool hovered = tab.contains(mousePos);

        const sf::Color bg = active ? sf::Color(0, 178, 102) : (hovered ? sf::Color(37, 99, 235, 190) : sf::Color(22, 33, 56));
        drawRoundedRect(window, tab.position, tab.size, 11.0f, bg, active ? 2.2f : 1.4f,
                        active ? sf::Color::White : sf::Color(86, 115, 160));
        drawShadowedCenteredText(window, res.font, tabNames[r], 15, sf::Color::White,
                                 {tab.position.x + tab.size.x * 0.5f, tab.position.y + tab.size.y * 0.5f}, active);
    }

    const RouteConfig route = getRouteConfig(game.highScoreTab);
    drawCenteredText(window, res.font, route.name + "  [" + route.difficulty + "]", 15, COL_CYAN, {640.0f, 236.0f});

    drawRoundedRect(window, {255.0f, 260.0f}, {770.0f, 34.0f}, 8.0f, sf::Color(25, 38, 66));
    sf::Text header(res.font, toSfString("Vieta   Vairuotojas        Taškai     Vėlavimas      Įvertinimas"), 15);
    header.setStyle(sf::Text::Bold);
    header.setFillColor(COL_GOLD);
    header.setPosition({272.0f, 268.0f});
    window.draw(header);

    const std::vector<HighScoreEntry> list = getRouteHighScores(game.highScores, game.highScoreTab);
    if (list.empty()) {
        drawCenteredText(window, res.font, "Šiame maršrute dar nėra išsaugotų rekordų.\nBūk pirmasis!", 18, COL_TEXT_MUTED, {640.0f, 415.0f});
    } else {
        for (std::size_t i = 0; i < list.size(); ++i) {
            const HighScoreEntry& e = list[i];
            const float rowY = 308.0f + static_cast<float>(i) * 48.0f;

            const sf::Color rowBg = (i == 0)
                ? sf::Color(0, 178, 102, 72)
                : ((i % 2 == 0) ? sf::Color(22, 33, 56, 200) : sf::Color(16, 24, 44, 200));
            drawRoundedRect(window, {255.0f, rowY}, {770.0f, 40.0f}, 9.0f, rowBg,
                            i == 0 ? 1.5f : 0.0f, sf::Color(0, 214, 125, 170));

            std::ostringstream oss;
            oss << " #" << (i + 1) << "     "
                << std::left << std::setw(15) << e.playerName
                << std::right << std::setw(7) << e.score << "      "
                << formatDelayVerbose(e.delaySeconds) << "     "
                << e.stars << "/5 zv.";

            sf::Text rowText(res.font, toSfString(oss.str()), 15);
            rowText.setFillColor(i == 0 ? COL_GOLD : COL_TEXT_MAIN);
            rowText.setPosition({272.0f, rowY + 10.0f});
            window.draw(rowText);
        }
    }

    for (const ButtonRect& btn : getScreenButtons(GameState::HighScores)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderCountdownOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 125));
    window.draw(dim);

    const int sec = static_cast<int>(std::ceil(game.countdownTimer));
    const std::string label = (sec >= 1) ? std::to_string(sec) : "VAŽIUOJAM!";
    const sf::Color col = (sec >= 1) ? COL_GOLD : COL_EMERALD;

    drawGlassPanel(window, {450.0f, 205.0f}, {380.0f, 220.0f}, col);
    drawShadowedCenteredText(window, res.font, "PASIRUOŠK!", 26, COL_TEXT_MAIN, {640.0f, 255.0f}, true);
    drawShadowedCenteredText(window, res.font, label, 82, col, {640.0f, 342.0f}, true);
}

void renderPauseOverlay(sf::RenderWindow& window, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 185));
    window.draw(dim);

    drawGlassPanel(window, {415.0f, 200.0f}, {450.0f, 338.0f}, COL_GOLD);
    drawShadowedCenteredText(window, res.font, "PAUZĖ", 36, COL_GOLD, {640.0f, 256.0f}, true);

    for (const ButtonRect& btn : getScreenButtons(GameState::Paused)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderNameInputOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 195));
    window.draw(dim);

    drawGlassPanel(window, {300.0f, 122.0f}, {680.0f, 482.0f}, COL_EMERALD);
    drawShadowedCenteredText(window, res.font, "DĖSTYTOJAS PRISTATYTAS!", 32, COL_EMERALD, {640.0f, 176.0f}, true);

    const RouteConfig route = getRouteConfig(game.selectedRoute);
    const int stars = getStarsForRun(game.delaySeconds, route);
    drawRoundedRect(window, {355.0f, 212.0f}, {570.0f, 36.0f}, 18.0f, sf::Color(255, 196, 36, 32), 1.2f, sf::Color(255, 196, 36, 150));
    drawCenteredText(window, res.font,
        "Taškai: " + std::to_string(game.score) + "  |  Vėlavimas: " + formatDelayVerbose(game.delaySeconds) +
        "  |  " + std::to_string(stars) + "/5 zv.",
        15, COL_GOLD, {640.0f, 230.0f});

    drawCenteredText(window, res.font, "Įvesk savo vairuotojo vardą rekordų lentelei:", 16, COL_TEXT_MAIN, {640.0f, 286.0f});

    const sf::Color boxBorder = game.nameValidationError.empty() ? COL_CYAN : COL_CORAL;
    drawRoundedRect(window, {445.0f, 318.0f}, {390.0f, 54.0f}, 12.0f, sf::Color(20, 30, 52), 2.5f, boxBorder);

    const bool showCursor = (static_cast<int>(game.totalElapsed * 2.2f) % 2 == 0);
    const std::string displayInput = game.playerNameInput + (showCursor ? "_" : "");
    drawCenteredText(window, res.font, displayInput, 24, sf::Color::White, {640.0f, 345.0f});

    if (!game.nameValidationError.empty()) {
        drawCenteredText(window, res.font, game.nameValidationError, 14, sf::Color(253, 164, 175), {640.0f, 404.0f});
    } else {
        drawCenteredText(window, res.font, "Raidės (A-Z), skaičiai, tarpas, _ arba - (iki 12 simb.)", 13, COL_TEXT_MUTED, {640.0f, 404.0f});
    }

    for (const ButtonRect& btn : getScreenButtons(GameState::NameInput)) {
        drawButton(window, res.font, btn, mousePos);
    }
}

void renderEndOverlay(sf::RenderWindow& window, const GameData& game, const GameResources& res, sf::Vector2f mousePos) {
    sf::RectangleShape dim({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    dim.setFillColor(sf::Color(6, 10, 22, 195));
    window.draw(dim);

    const RouteConfig route = getRouteConfig(game.selectedRoute);
    const bool isWin = (game.state == GameState::Victory);
    drawGlassPanel(window, {285.0f, 76.0f}, {710.0f, 584.0f}, isWin ? COL_EMERALD : COL_CORAL);

    if (isWin) {
        drawShadowedCenteredText(window, res.font, "DĖSTYTOJAS PRISTATYTAS!", 33, COL_EMERALD, {640.0f, 128.0f}, true);

        const int stars = getStarsForRun(game.delaySeconds, route);
        std::string rank;
        if (stars == 5) rank = "Įvertinimas: 5/5 žvaigždutės! Rekordiškai greita kelionė!";
        else if (stars == 4) rank = "Įvertinimas: 4/5 žvaigždutės! Spėjome beveik laiku!";
        else rank = "Įvertinimas: 3/5 žvaigždutės! Studentai jau stovėjo prie durų!";

        drawCenteredText(window, res.font, rank, 16, COL_GOLD, {640.0f, 174.0f});
        if (game.scoreSavedThisRun) {
            drawCenteredText(window, res.font, "Rezultatas išsaugotas rekordų lentelėje!", 13, sf::Color(110, 231, 183), {640.0f, 204.0f});
        }
    } else {
        drawShadowedCenteredText(window, res.font, "KELIONĖ ATŠAUKTA...", 33, COL_CORAL, {640.0f, 128.0f}, true);

        const std::string reason = (game.gameOverReason == GameOverReason::TooLate)
            ? ("Praėjo " + std::to_string(static_cast<int>(route.maxDelayMinutes)) + " min. – Dėstytojas: „Studentai jau išėjo namo!“")
            : "Dėstytojas neišlaikė: „Sustok, geriau eisiu pėsčiomis!“";
        drawCenteredText(window, res.font, reason, 17, sf::Color(253, 186, 116), {640.0f, 180.0f});
    }

    drawRoundedRect(window, {345.0f, 230.0f}, {590.0f, 198.0f}, 14.0f, COL_PANEL_INNER, 1.2f, COL_BORDER_SUBTLE);

    const std::string summary =
        "Maršrutas:            " + route.name + "\n"
        "Nuvažiuotas atstumas: " + formatDistance(game.distanceKm, route.targetDistanceKm) + "\n"
        "Vėlavimo laikas:      " + formatDelayVerbose(game.delaySeconds) + "\n"
        "Surinkti taškai:      " + std::to_string(game.score) + "\n"
        "Aplenktos kliūtys:    " + std::to_string(game.dodgedCount) + "\n"
        "Kavos / USB skaidrės: " + std::to_string(game.coffeesCollected) + " / " + std::to_string(game.usbCollected);

    sf::Text summaryText(res.font, toSfString(summary), 16);
    summaryText.setFillColor(COL_TEXT_MAIN);
    summaryText.setPosition({372.0f, 250.0f});
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

    // Sodri žalia miesto parko pakelė (švelniai šviesėja artėjant prie tikslo)
    const sf::Color grassColor(
        static_cast<std::uint8_t>(18.0f + 12.0f * progress),
        static_cast<std::uint8_t>(78.0f + 28.0f * progress),
        static_cast<std::uint8_t>(48.0f + 16.0f * progress)
    );
    sf::RectangleShape grass({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    grass.setFillColor(grassColor);
    window.draw(grass);

    // Pastatai su šiltai šviečiančiais langais ir sluoksniuoti medžiai
    for (const SceneryProp& prop : game.scenery) {
        if (prop.isBuilding) {
            drawRoundedRect(window, {prop.position.x + 8.0f, prop.position.y + 9.0f}, prop.rectSize, 8.0f, sf::Color(0, 0, 0, 75));
            drawRoundedRect(window, prop.position, prop.rectSize, 8.0f, sf::Color(30, 41, 59), 2.0f, sf::Color(51, 65, 85));
            // Stogo akcentas
            drawRoundedRect(window, {prop.position.x + 10.0f, prop.position.y + 10.0f},
                            {prop.rectSize.x - 20.0f, prop.rectSize.y - 20.0f}, 5.0f, sf::Color(42, 56, 80));
            // Šilti šviečiantys langai
            for (float wy = prop.position.y + 18.0f; wy < prop.position.y + prop.rectSize.y - 22.0f; wy += 24.0f) {
                for (float wx = prop.position.x + 18.0f; wx < prop.position.x + prop.rectSize.x - 22.0f; wx += 24.0f) {
                    sf::RectangleShape win({10.0f, 10.0f});
                    win.setPosition({wx, wy});
                    win.setFillColor(sf::Color(253, 224, 71, 185));
                    window.draw(win);
                }
            }
        } else {
            sf::CircleShape shadow(prop.radius);
            shadow.setOrigin({prop.radius, prop.radius});
            shadow.setPosition({prop.position.x + 7.0f, prop.position.y + 7.0f});
            shadow.setFillColor(sf::Color(0, 0, 0, 65));
            window.draw(shadow);

            sf::CircleShape treeOuter(prop.radius);
            treeOuter.setOrigin({prop.radius, prop.radius});
            treeOuter.setPosition(prop.position);
            treeOuter.setFillColor(sf::Color(21, 128, 61));
            treeOuter.setOutlineThickness(2.0f);
            treeOuter.setOutlineColor(sf::Color(12, 74, 38));
            window.draw(treeOuter);

            sf::CircleShape treeInner(prop.radius * 0.62f);
            treeInner.setOrigin({prop.radius * 0.62f, prop.radius * 0.62f});
            treeInner.setPosition({prop.position.x - 3.0f, prop.position.y - 3.0f});
            treeInner.setFillColor(sf::Color(34, 197, 94, 180));
            window.draw(treeInner);
        }
    }

    // Šaligatviai
    const float sidewalkW = 24.0f;
    sf::RectangleShape leftSidewalk({sidewalkW, static_cast<float>(WINDOW_HEIGHT)});
    leftSidewalk.setPosition({ROAD_LEFT - sidewalkW, 0.0f});
    leftSidewalk.setFillColor(sf::Color(71, 85, 105));
    window.draw(leftSidewalk);

    sf::RectangleShape rightSidewalk({sidewalkW, static_cast<float>(WINDOW_HEIGHT)});
    rightSidewalk.setPosition({ROAD_RIGHT, 0.0f});
    rightSidewalk.setFillColor(sf::Color(71, 85, 105));
    window.draw(rightSidewalk);

    // Raudonai balti kelio borteliai (gyvas arkadinis akcentas)
    int curbIdx = 0;
    for (float y = -90.0f + game.roadOffset; y < static_cast<float>(WINDOW_HEIGHT) + 90.0f; y += 45.0f) {
        const sf::Color curbCol = (curbIdx++ % 2 == 0) ? sf::Color(225, 29, 72) : sf::Color(241, 245, 249);
        sf::RectangleShape lc({6.0f, 45.0f});
        lc.setPosition({ROAD_LEFT - 6.0f, y});
        lc.setFillColor(curbCol);
        window.draw(lc);

        sf::RectangleShape rc({6.0f, 45.0f});
        rc.setPosition({ROAD_RIGHT, y});
        rc.setFillColor(curbCol);
        window.draw(rc);
    }

    // Tamsus kontrastingas asfaltas su subtiliai skirtingais juostų atspalviais
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        sf::RectangleShape laneRect({LANE_WIDTH, static_cast<float>(WINDOW_HEIGHT)});
        laneRect.setPosition({ROAD_LEFT + static_cast<float>(lane) * LANE_WIDTH, 0.0f});
        laneRect.setFillColor((lane % 2 == 0) ? sf::Color(30, 35, 48) : sf::Color(36, 42, 56));
        window.draw(laneRect);
    }

    // Ryškios gintarinės kraštinės linijos
    sf::RectangleShape leftLine({5.0f, static_cast<float>(WINDOW_HEIGHT)});
    leftLine.setPosition({ROAD_LEFT + 7.0f, 0.0f});
    leftLine.setFillColor(COL_GOLD);
    window.draw(leftLine);

    sf::RectangleShape rightLine({5.0f, static_cast<float>(WINDOW_HEIGHT)});
    rightLine.setPosition({ROAD_RIGHT - 12.0f, 0.0f});
    rightLine.setFillColor(COL_GOLD);
    window.draw(rightLine);

    // Ryškios baltos punktyrinės juostų linijos
    for (int lane = 1; lane < LANE_COUNT; ++lane) {
        const float x = ROAD_LEFT + static_cast<float>(lane) * LANE_WIDTH - 3.0f;
        for (float y = -90.0f + game.roadOffset; y < static_cast<float>(WINDOW_HEIGHT) + 90.0f; y += 90.0f) {
            drawRoundedRect(window, {x, y}, {6.0f, 46.0f}, 3.0f, sf::Color(241, 245, 249, 225));
        }
    }

    // Greičio linijos laikant W arba veikiant Žaliai bangai
    if (game.state == GameState::Playing && game.speedMultiplier > 1.2f) {
        const std::uint8_t lineAlpha = (game.player.greenWaveTimer > 0.0f) ? 165 : 95;
        const sf::Color lineColor = (game.player.greenWaveTimer > 0.0f)
            ? sf::Color(0, 255, 150, lineAlpha)
            : sf::Color(186, 230, 253, lineAlpha);

        for (int i = 0; i < 6; ++i) {
            const float y = std::fmod(game.roadOffset * 2.4f + static_cast<float>(i) * 130.0f, static_cast<float>(WINDOW_HEIGHT) + 80.0f) - 40.0f;
            sf::RectangleShape leftStreak({3.5f, 54.0f});
            leftStreak.setPosition({ROAD_LEFT + 18.0f, y});
            leftStreak.setFillColor(lineColor);
            window.draw(leftStreak);

            sf::RectangleShape rightStreak({3.5f, 54.0f});
            rightStreak.setPosition({ROAD_RIGHT - 21.5f, y + 40.0f});
            rightStreak.setFillColor(lineColor);
            window.draw(rightStreak);
        }
    }
}

void renderEntities(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    // 1. Bonusai su švelniu pulsavimu
    for (const BonusItem& bonus : game.bonuses) {
        if (!bonus.active) continue;

        const float pulse = 1.0f + 0.09f * std::sin(bonus.pulsePhase);
        const float drawSize = bonus.radius * 2.3f * pulse;
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
            fallback.setFillColor(bonus.type == BonusType::Coffee ? COL_GOLD
                : (bonus.type == BonusType::GreenWave ? COL_EMERALD : COL_CYAN));
            window.draw(fallback);
        }
    }

    // 2. Kliūtys
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

    // 3. Žaidėjo automobilis – žalias pavežėjas
    if (game.player.greenWaveTimer > 0.0f) {
        const float auraPulse = 8.0f + 4.0f * std::sin(game.totalElapsed * 12.0f);
        drawRoundedRect(window,
                        {game.player.position.x - auraPulse, game.player.position.y - auraPulse},
                        {game.player.size.x + auraPulse * 2.0f, game.player.size.y + auraPulse * 2.0f},
                        16.0f, sf::Color(0, 235, 135, 65), 2.8f, sf::Color(110, 255, 185, 220));
    }

    std::uint8_t playerAlpha = 255;
    if (game.player.invincibilityTimer > 0.0f && game.player.greenWaveTimer <= 0.0f) {
        const bool dimFrame = (static_cast<int>(game.player.invincibilityTimer * 14.0f) % 2 == 0);
        playerAlpha = dimFrame ? 115 : 235;
    }

    if (res.hasPlayerCar) {
        drawSpriteFit(window, res.playerCar, game.player.position, game.player.size, playerAlpha);
    } else {
        drawFallbackRect(window, game.player.position, game.player.size, COL_EMERALD, playerAlpha);
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
        const std::uint8_t alpha = static_cast<std::uint8_t>(std::min(125.0f, game.screenFlashTimer * 350.0f));
        sf::RectangleShape flash({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        flash.setFillColor(sf::Color(244, 63, 94, alpha));
        window.draw(flash);
    }

    // --- KAIRĖS PUSĖS KORTELĖ: Maršrutas, Progresas ir Vėlavimas ---
    drawGlassPanel(window, {18.0f, 18.0f}, {308.0f, 286.0f}, COL_EMERALD);

    drawShadowedText(window, res.font, "MARŠRUTAS", 15, COL_CYAN, {36.0f, 30.0f}, true);

    drawRoundedRect(window, {36.0f, 54.0f}, {272.0f, 26.0f}, 8.0f, COL_PANEL_INNER);
    drawCenteredText(window, res.font, route.name, 13, COL_TEXT_MAIN, {172.0f, 67.0f});

    // Atstumas ir suapvalinta progreso juosta
    sf::Text distLabel(res.font, toSfString("Atstumas: " + formatDistance(game.distanceKm, route.targetDistanceKm)), 14);
    distLabel.setFillColor(COL_TEXT_MAIN);
    distLabel.setPosition({36.0f, 90.0f});
    window.draw(distLabel);

    const float barW = 272.0f;
    drawRoundedRect(window, {36.0f, 114.0f}, {barW, 18.0f}, 9.0f, sf::Color(24, 34, 56), 1.2f, sf::Color(72, 102, 148));

    const float progress = std::clamp(game.distanceKm / route.targetDistanceKm, 0.0f, 1.0f);
    if (progress > 0.02f) {
        drawRoundedRect(window, {36.0f, 114.0f}, {std::max(18.0f, barW * progress), 18.0f}, 9.0f, COL_EMERALD);
    }
    sf::CircleShape marker(7.0f);
    marker.setOrigin({7.0f, 7.0f});
    marker.setPosition({36.0f + barW * progress, 123.0f});
    marker.setFillColor(COL_GOLD);
    marker.setOutlineThickness(2.0f);
    marker.setOutlineColor(sf::Color(15, 23, 42));
    window.draw(marker);

    // Vėlavimo laikmačio kapsulė
    const sf::Color delayCol = getDelayColor(game.delaySeconds, route.maxDelayMinutes);
    drawRoundedRect(window, {36.0f, 146.0f}, {272.0f, 76.0f}, 12.0f, COL_PANEL_INNER, 1.5f,
                    sf::Color(delayCol.r, delayCol.g, delayCol.b, 160));

    sf::Text delayTitle(res.font, toSfString("Vėlavimas į paskaitą (riba " + std::to_string(static_cast<int>(route.maxDelayMinutes)) + ":00)"), 12);
    delayTitle.setFillColor(COL_TEXT_MUTED);
    delayTitle.setPosition({48.0f, 154.0f});
    window.draw(delayTitle);

    drawShadowedText(window, res.font, formatDelayTime(game.delaySeconds) + " min", 28, delayCol, {48.0f, 174.0f}, true);

    // Greičio rodiklis
    const int speedKmh = static_cast<int>(game.currentRoadSpeed * game.speedMultiplier * 0.16f);
    drawRoundedRect(window, {36.0f, 236.0f}, {272.0f, 50.0f}, 12.0f, COL_PANEL_INNER);
    sf::Text spdLbl(res.font, toSfString("Greitis:"), 14);
    spdLbl.setFillColor(COL_TEXT_MUTED);
    spdLbl.setPosition({50.0f, 252.0f});
    window.draw(spdLbl);

    drawShadowedText(window, res.font, std::to_string(speedKmh) + " km/h", 22,
                     game.player.greenWaveTimer > 0.0f ? COL_EMERALD : COL_TEXT_MAIN, {146.0f, 247.0f}, true);

    // Žalios bangos aktyvaus laikmačio kortelė
    if (game.player.greenWaveTimer > 0.0f) {
        drawGlassPanel(window, {18.0f, 316.0f}, {308.0f, 58.0f}, COL_EMERALD);
        drawShadowedText(window, res.font, "ŽALIA BANGA AKTYVI!", 14, COL_EMERALD, {36.0f, 326.0f}, true);
        const float gwRatio = std::clamp(game.player.greenWaveTimer / GREEN_WAVE_DURATION, 0.0f, 1.0f);
        drawRoundedRect(window, {36.0f, 352.0f}, {272.0f, 10.0f}, 5.0f, sf::Color(24, 34, 56));
        drawRoundedRect(window, {36.0f, 352.0f}, {std::max(10.0f, 272.0f * gwRatio), 10.0f}, 5.0f, COL_EMERALD);
    }

    // --- DEŠINĖS PUSĖS KORTELĖ: Dėstytojo kantrybė, Taškai, Valdymas ---
    drawGlassPanel(window, {954.0f, 18.0f}, {308.0f, 314.0f}, COL_GOLD);

    drawShadowedText(window, res.font, "PAVEŽĖJO BŪSENA", 15, COL_GOLD, {972.0f, 30.0f}, true);

    sf::Text livesLabel(res.font, toSfString("Dėstytojo kantrybė:"), 14);
    livesLabel.setFillColor(COL_TEXT_MUTED);
    livesLabel.setPosition({972.0f, 58.0f});
    window.draw(livesLabel);

    for (int i = 0; i < PLAYER_MAX_LIVES; ++i) {
        const sf::Vector2f slotPos = {972.0f + static_cast<float>(i) * 56.0f, 82.0f};
        const bool alive = (i < game.player.lives);
        if (res.hasLecturerFaces) {
            sf::CircleShape ring(22.0f);
            ring.setPosition({slotPos.x - 2.0f, slotPos.y - 2.0f});
            ring.setFillColor(alive ? sf::Color(0, 214, 125, 45) : sf::Color(30, 41, 59, 120));
            ring.setOutlineThickness(1.5f);
            ring.setOutlineColor(alive ? COL_EMERALD : sf::Color(51, 65, 85));
            window.draw(ring);
            drawSpriteFit(window, res.lecturerHappy, slotPos, {40.0f, 40.0f}, alive ? 255 : 40);
        } else {
            drawRoundedRect(window, slotPos, {44.0f, 22.0f}, 8.0f, alive ? COL_CORAL : sf::Color(35, 45, 65));
        }
    }

    // Taškų kapsulė
    drawRoundedRect(window, {972.0f, 136.0f}, {272.0f, 64.0f}, 12.0f, COL_PANEL_INNER, 1.2f, sf::Color(255, 196, 36, 120));
    sf::Text scoreLbl(res.font, toSfString("TAŠKAI"), 11);
    scoreLbl.setStyle(sf::Text::Bold);
    scoreLbl.setFillColor(COL_TEXT_MUTED);
    scoreLbl.setPosition({986.0f, 144.0f});
    window.draw(scoreLbl);

    drawShadowedText(window, res.font, std::to_string(game.score), 26, COL_GOLD, {986.0f, 160.0f}, true);

    sf::Text statsText(res.font,
        toSfString("Aplenkta: " + std::to_string(game.dodgedCount) +
                   "  |  Kavos: " + std::to_string(game.coffeesCollected) +
                   "  |  USB: " + std::to_string(game.usbCollected)),
        12);
    statsText.setFillColor(COL_TEXT_MUTED);
    statsText.setPosition({974.0f, 210.0f});
    window.draw(statsText);

    // Kompaktiški klavišų ženkliukai
    drawKeyCap(window, res.font, {972.0f, 238.0f}, 72.0f, "WASD", "vairuoti ir greitinti");
    drawKeyCap(window, res.font, {972.0f, 268.0f}, 72.0f, "P / ESC", "pauzė");
    drawKeyCap(window, res.font, {972.0f, 298.0f}, 72.0f, "M", "garsas");

    // --- DĖSTYTOJO KALBOS BURBULAS ŠALIA AUTOMOBILIO ---
    drawCarSpeechBubble(window, game, res);
}

void renderGame(sf::RenderWindow& window, const GameData& game, const GameResources& res) {
    window.clear(sf::Color(10, 16, 28));

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
