#pragma once

#include "types.hpp"
#include <string>
#include <vector>

// Rekordų skaitymas ir rašymas į tekstinį failą (saugus prieš sugadintus duomenis)
std::vector<HighScoreEntry> loadHighScores(const std::string& filename);
bool saveHighScores(const std::string& filename, const std::vector<HighScoreEntry>& scores);

// Prideda naują įrašą, surūšiuoja ir palieka TOP 5 kiekvienam maršrutui
void addHighScore(std::vector<HighScoreEntry>& scores, const HighScoreEntry& entry);

// Grąžina konkretaus maršruto rekordus
std::vector<HighScoreEntry> getRouteHighScores(const std::vector<HighScoreEntry>& scores, int routeIndex);

// Tikrina, ar įvestas simbolis leistinas žaidėjo vardui
bool isAllowedNameChar(char32_t unicode);
std::string trimWhitespace(const std::string& text);
