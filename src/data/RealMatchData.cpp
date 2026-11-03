#include "RealMatchData.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace {

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::stringstream ss(line);
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }
    // If the line ended with a comma, add an empty token
    if (!line.empty() && line.back() == ',') {
        tokens.push_back("");
    }
    return tokens;
}

bool parseFloatSafe(const std::string& s, float& outVal) {
    if (s.empty() || s == "NaN" || s == "nan" || s == "NAN") {
        return false;
    }
    try {
        outVal = std::stof(s);
        return !std::isnan(outVal);
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

RealMatchDataset::RealMatchDataset() = default;

bool RealMatchDataset::loadMetricaSampleGame(
    const std::string& homeTrackingCsv,
    const std::string& awayTrackingCsv,
    const std::string& eventsCsv,
    int frameStride
) {
    std::cout << "[RealMatch] Loading Metrica Sports real match tracking & events...\n";
    if (frameStride < 1) frameStride = 1;

    // 1. Load Events CSV
    m_events.clear();
    std::ifstream eFile(eventsCsv);
    if (eFile.is_open()) {
        std::string line;
        // Skip header
        std::getline(eFile, line);

        while (std::getline(eFile, line)) {
            if (line.empty()) continue;
            auto cols = splitCsvLine(line);
            if (cols.size() < 10) continue;

            RealMatchEvent ev;
            ev.team = (cols[0] == "Home") ? TeamSide::Home : TeamSide::Away;
            ev.type = cols[1];
            ev.subType = cols[2];

            try {
                ev.period = std::stoi(cols[3]);
                ev.startFrame = std::stoi(cols[4]);
                ev.timestampS = std::stof(cols[5]);
                if (cols.size() > 6 && !cols[6].empty()) ev.endFrame = std::stoi(cols[6]);
            } catch (...) {}

            ev.fromPlayer = cols[8];
            if (cols.size() > 9) ev.toPlayer = cols[9];

            float sx = 0.0f, sy = 0.0f, ex = 0.0f, ey = 0.0f;
            if (cols.size() > 10 && parseFloatSafe(cols[10], sx)) ev.startPos.x = std::clamp(sx * 105.0f, 0.0f, 105.0f);
            if (cols.size() > 11 && parseFloatSafe(cols[11], sy)) ev.startPos.y = std::clamp(sy * 68.0f, 0.0f, 68.0f);
            if (cols.size() > 12 && parseFloatSafe(cols[12], ex)) ev.endPos.x = std::clamp(ex * 105.0f, 0.0f, 105.0f);
            if (cols.size() > 13 && parseFloatSafe(cols[13], ey)) ev.endPos.y = std::clamp(ey * 68.0f, 0.0f, 68.0f);

            // Classify goals, shots, and passes
            if (ev.subType.find("GOAL") != std::string::npos && ev.subType.find("GOAL KICK") == std::string::npos) {
                ev.isGoal = true;
                if (ev.team == TeamSide::Home) m_finalHomeScore++;
                else m_finalAwayScore++;
            }

            if (ev.type == "SHOT" || ev.subType.find("SHOT") != std::string::npos) {
                ev.isShot = true;
                if (ev.team == TeamSide::Home) m_totalHomeShots++;
                else m_totalAwayShots++;
            } else if (ev.type == "PASS") {
                if (ev.team == TeamSide::Home) m_totalHomePasses++;
                else m_totalAwayPasses++;
            }

            m_events.push_back(ev);
        }
        std::cout << "[RealMatch] Loaded " << m_events.size() << " real match events. Final Score: Home "
                  << m_finalHomeScore << " - " << m_finalAwayScore << " Away\n";
    } else {
        std::cerr << "[RealMatch] Warning: Could not open events file: " << eventsCsv << "\n";
    }

    // 2. Load Home Tracking CSV
    std::ifstream hFile(homeTrackingCsv);
    std::ifstream aFile(awayTrackingCsv);
    if (!hFile.is_open() || !aFile.is_open()) {
        std::cerr << "[RealMatch] Error: Could not open tracking CSV files." << std::endl;
        return false;
    }

    // Read headers to determine player shirt numbers
    std::string hLine1, hLine2, hLine3;
    std::getline(hFile, hLine1);
    std::getline(hFile, hLine2);
    std::getline(hFile, hLine3);

    std::string aLine1, aLine2, aLine3;
    std::getline(aFile, aLine1);
    std::getline(aFile, aLine2);
    std::getline(aFile, aLine3);

    auto hCols2 = splitCsvLine(hLine2);
    std::vector<int> homeShirts;
    // Player columns start at index 3, 5, 7... until Ball (at index 31)
    for (size_t c = 3; c + 1 < hCols2.size() && c < 31; c += 2) {
        try {
            homeShirts.push_back(std::stoi(hCols2[c]));
        } catch (...) {
            homeShirts.push_back(static_cast<int>(homeShirts.size() + 1));
        }
    }

    auto aCols2 = splitCsvLine(aLine2);
    std::vector<int> awayShirts;
    for (size_t c = 3; c + 1 < aCols2.size() && c < 31; c += 2) {
        try {
            awayShirts.push_back(std::stoi(aCols2[c]));
        } catch (...) {
            awayShirts.push_back(static_cast<int>(15 + awayShirts.size()));
        }
    }

    m_frames.clear();
    m_frames.reserve(75000);

    std::string hLine, aLine;
    int lineCounter = 0;
    Vec2 prevBallPos(52.5f, 34.0f);

    struct PlayerHistory {
        Vec2 prevPos{0.0f, 0.0f};
        float distCoveredM{0.0f};
        bool hadPos{false};
    };
    std::vector<PlayerHistory> homeHist(homeShirts.size());
    std::vector<PlayerHistory> awayHist(awayShirts.size());

    while (std::getline(hFile, hLine) && std::getline(aFile, aLine)) {
        lineCounter++;
        if ((lineCounter % frameStride) != 0) {
            continue;
        }

        auto hTokens = splitCsvLine(hLine);
        auto aTokens = splitCsvLine(aLine);
        if (hTokens.size() < 33 || aTokens.size() < 33) continue;

        RealMatchFrame frame;
        try {
            frame.period = std::stoi(hTokens[0]);
            frame.frameNumber = std::stoi(hTokens[1]);
            frame.timestampS = std::stof(hTokens[2]);
        } catch (...) {
            continue;
        }

        float dt = (m_frames.empty()) ? 0.04f * frameStride : (frame.timestampS - m_frames.back().timestampS);
        if (dt <= 0.0001f) dt = 0.04f * frameStride;

        // Parse Home Players
        for (size_t i = 0; i < homeShirts.size(); ++i) {
            size_t colIdx = 3 + i * 2;
            if (colIdx + 1 >= hTokens.size()) break;

            float x = 0.0f, y = 0.0f;
            if (parseFloatSafe(hTokens[colIdx], x) && parseFloatSafe(hTokens[colIdx + 1], y)) {
                RealMatchPlayerTrack pt;
                pt.shirtNumber = homeShirts[i];
                pt.team = TeamSide::Home;
                pt.isActive = true;
                pt.isGoalkeeper = (homeShirts[i] == 11); // Metrica Home GK is #11
                pt.pitchPos = Vec2(std::clamp(x * 105.0f, 0.0f, 105.0f), std::clamp(y * 68.0f, 0.0f, 68.0f));

                if (homeHist[i].hadPos) {
                    Vec2 delta = pt.pitchPos - homeHist[i].prevPos;
                    float stepDist = delta.length();
                    if (stepDist < 15.0f) { // filter out teleporting jumps
                        homeHist[i].distCoveredM += stepDist;
                        pt.velocity = delta * (1.0f / dt);
                        pt.speedMps = pt.velocity.length();
                    }
                }
                homeHist[i].prevPos = pt.pitchPos;
                homeHist[i].hadPos = true;
                pt.distanceCoveredM = homeHist[i].distCoveredM;

                frame.players.push_back(pt);
            }
        }

        // Parse Away Players
        for (size_t i = 0; i < awayShirts.size(); ++i) {
            size_t colIdx = 3 + i * 2;
            if (colIdx + 1 >= aTokens.size()) break;

            float x = 0.0f, y = 0.0f;
            if (parseFloatSafe(aTokens[colIdx], x) && parseFloatSafe(aTokens[colIdx + 1], y)) {
                RealMatchPlayerTrack pt;
                pt.shirtNumber = awayShirts[i];
                pt.team = TeamSide::Away;
                pt.isActive = true;
                pt.isGoalkeeper = (awayShirts[i] == 25); // Metrica Away GK is #25
                pt.pitchPos = Vec2(std::clamp(x * 105.0f, 0.0f, 105.0f), std::clamp(y * 68.0f, 0.0f, 68.0f));

                if (awayHist[i].hadPos) {
                    Vec2 delta = pt.pitchPos - awayHist[i].prevPos;
                    float stepDist = delta.length();
                    if (stepDist < 15.0f) {
                        awayHist[i].distCoveredM += stepDist;
                        pt.velocity = delta * (1.0f / dt);
                        pt.speedMps = pt.velocity.length();
                    }
                }
                awayHist[i].prevPos = pt.pitchPos;
                awayHist[i].hadPos = true;
                pt.distanceCoveredM = awayHist[i].distCoveredM;

                frame.players.push_back(pt);
            }
        }

        // Parse Ball (columns 31 and 32 in home CSV)
        float bx = 0.0f, by = 0.0f;
        if (parseFloatSafe(hTokens[31], bx) && parseFloatSafe(hTokens[32], by)) {
            frame.ballPos = Vec2(std::clamp(bx * 105.0f, 0.0f, 105.0f), std::clamp(by * 68.0f, 0.0f, 68.0f));
            prevBallPos = frame.ballPos;
        } else if (parseFloatSafe(aTokens[31], bx) && parseFloatSafe(aTokens[32], by)) {
            frame.ballPos = Vec2(std::clamp(bx * 105.0f, 0.0f, 105.0f), std::clamp(by * 68.0f, 0.0f, 68.0f));
            prevBallPos = frame.ballPos;
        } else {
            frame.ballPos = prevBallPos; // retain last known ball spot
        }

        m_frames.push_back(frame);
    }

    if (!m_frames.empty()) {
        m_totalDurationS = m_frames.back().timestampS;
        m_loaded = true;
        std::cout << "[RealMatch] Successfully loaded " << m_frames.size()
                  << " frames (" << m_totalDurationS / 60.0f << " minutes of real tracking data).\n";
        return true;
    }

    return false;
}

const RealMatchFrame* RealMatchDataset::getFrameByIndex(size_t index) const {
    if (m_frames.empty()) return nullptr;
    if (index >= m_frames.size()) index = m_frames.size() - 1;
    return &m_frames[index];
}

size_t RealMatchDataset::getIndexForTime(float timeS) const {
    if (m_frames.empty()) return 0;
    if (timeS <= m_frames.front().timestampS) return 0;
    if (timeS >= m_frames.back().timestampS) return m_frames.size() - 1;

    // Binary search for closest timestamp
    size_t low = 0;
    size_t high = m_frames.size() - 1;
    while (low + 1 < high) {
        size_t mid = low + (high - low) / 2;
        if (m_frames[mid].timestampS <= timeS) {
            low = mid;
        } else {
            high = mid;
        }
    }
    return (std::abs(m_frames[low].timestampS - timeS) <= std::abs(m_frames[high].timestampS - timeS)) ? low : high;
}

const RealMatchFrame* RealMatchDataset::getFrameAtTime(float timeS) const {
    if (m_frames.empty()) return nullptr;
    return &m_frames[getIndexForTime(timeS)];
}

std::string RealMatchDataset::getActiveEventText(float timeS, float windowSeconds) const {
    for (const auto& ev : m_events) {
        if (timeS >= ev.timestampS && timeS <= ev.timestampS + windowSeconds) {
            std::ostringstream ss;
            int mins = static_cast<int>(ev.timestampS) / 60;
            int secs = static_cast<int>(ev.timestampS) % 60;

            if (ev.isGoal) {
                ss << "[GOAL " << std::setfill('0') << std::setw(2) << mins << ":"
                   << std::setfill('0') << std::setw(2) << secs << "!] "
                   << (ev.team == TeamSide::Home ? "HOME " : "AWAY ") << ev.fromPlayer
                   << " scores! (" << ev.subType << ")";
            } else if (ev.isShot) {
                ss << "[SHOT " << std::setfill('0') << std::setw(2) << mins << ":"
                   << std::setfill('0') << std::setw(2) << secs << "] "
                   << (ev.team == TeamSide::Home ? "HOME " : "AWAY ") << ev.fromPlayer
                   << " - " << ev.subType;
            } else {
                ss << "[EVENT " << std::setfill('0') << std::setw(2) << mins << ":"
                   << std::setfill('0') << std::setw(2) << secs << "] "
                   << (ev.team == TeamSide::Home ? "HOME " : "AWAY ") << ev.fromPlayer;
                if (!ev.toPlayer.empty()) ss << " -> " << ev.toPlayer;
                ss << " (" << ev.type;
                if (!ev.subType.empty()) ss << " - " << ev.subType;
                ss << ")";
            }
            return ss.str();
        }
    }
    return "";
}

int RealMatchDataset::getHomeScore(float timeS) const {
    int score = 0;
    for (const auto& ev : m_events) {
        if (ev.isGoal && ev.team == TeamSide::Home && ev.timestampS <= timeS) {
            score++;
        }
    }
    return score;
}

int RealMatchDataset::getAwayScore(float timeS) const {
    int score = 0;
    for (const auto& ev : m_events) {
        if (ev.isGoal && ev.team == TeamSide::Away && ev.timestampS <= timeS) {
            score++;
        }
    }
    return score;
}
