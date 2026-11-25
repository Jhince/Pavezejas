#include "storage.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

std::vector<HighScoreEntry> loadHighScores(const std::string& filename) {
    std::vector<HighScoreEntry> result;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return result; // Failo dar nėra – pradedame nuo tuščio sąrašo
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        // Formatas: routeIndex;score;delaySeconds;stars;playerName
        std::istringstream iss(line);
        std::string routeStr;
        std::string scoreStr;
        std::string delayStr;
        std::string starsStr;
        std::string name;

        if (!std::getline(iss, routeStr, ';') ||
            !std::getline(iss, scoreStr, ';') ||
            !std::getline(iss, delayStr, ';') ||
            !std::getline(iss, starsStr, ';') ||
            !std::getline(iss, name)) {
            continue; // Praleidžiame sugadintą eilutę
        }

        try {
            HighScoreEntry entry;
            entry.routeIndex = std::stoi(routeStr);
            entry.score = std::stoi(scoreStr);
            entry.delaySeconds = std::stof(delayStr);
            entry.stars = std::stoi(starsStr);
            entry.playerName = trimWhitespace(name);

            if (entry.routeIndex >= 0 && entry.routeIndex < ROUTE_COUNT &&
                entry.score >= 0 && !entry.playerName.empty()) {
                result.push_back(entry);
            }
        } catch (...) {
            continue; // Saugiai ignoruojame neteisingus skaičius faile
        }
    }

    return result;
}

bool saveHighScores(const std::string& filename, const std::vector<HighScoreEntry>& scores) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    for (const HighScoreEntry& entry : scores) {
        file << entry.routeIndex << ';'
             << entry.score << ';'
             << entry.delaySeconds << ';'
             << entry.stars << ';'
             << entry.playerName << '\n';
    }
    return file.good();
}

void addHighScore(std::vector<HighScoreEntry>& scores, const HighScoreEntry& entry) {
    scores.push_back(entry);

    // Surūšiuojame pagal maršrutą, tada pagal taškus (mažėjančiai), tada pagal mažesnį vėlavimą
    std::sort(scores.begin(), scores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        if (a.routeIndex != b.routeIndex) {
            return a.routeIndex < b.routeIndex;
        }
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.delaySeconds < b.delaySeconds;
    });

    // Paliekame tik TOP MAX_SCORES_PER_ROUTE kiekvienam maršrutui
    std::vector<HighScoreEntry> trimmed;
    int counts[ROUTE_COUNT] = {0, 0, 0};
    for (const HighScoreEntry& item : scores) {
        if (item.routeIndex >= 0 && item.routeIndex < ROUTE_COUNT &&
            counts[item.routeIndex] < MAX_SCORES_PER_ROUTE) {
            trimmed.push_back(item);
            counts[item.routeIndex] += 1;
        }
    }
    scores = std::move(trimmed);
}

std::vector<HighScoreEntry> getRouteHighScores(const std::vector<HighScoreEntry>& scores, int routeIndex) {
    std::vector<HighScoreEntry> filtered;
    for (const HighScoreEntry& entry : scores) {
        if (entry.routeIndex == routeIndex) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

bool isAllowedNameChar(char32_t unicode) {
    // Lotyniškos raidės, skaičiai, tarpas, brūkšnelis, pabraukimas
    if ((unicode >= U'A' && unicode <= U'Z') ||
        (unicode >= U'a' && unicode <= U'z') ||
        (unicode >= U'0' && unicode <= U'9') ||
        unicode == U' ' || unicode == U'_' || unicode == U'-') {
        return true;
    }
    return false;
}

std::string trimWhitespace(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}
