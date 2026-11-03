#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Config.hpp"
#include "Entities.hpp"

// Single spatial tracking sample for a player or ball
struct TrackingSample {
    float timestamp{0.0f};
    int entityId{0};          // 0 for ball, 1-22 for players
    int teamId{-1};           // -1 for ball, 0 for Home, 1 for Away
    int shirtNumber{-1};
    std::string role;
    float x{0.0f};            // Metres
    float y{0.0f};            // Metres
    float z{0.0f};            // Metres (height)
    float speedMps{0.0f};     // Metres per second
    float distanceTotalM{0.0f};
    std::string state;
    bool hasPossession{false};
};

class TrackingSystem {
public:
    TrackingSystem(float sampleFrequencyHz = 10.0f);
    ~TrackingSystem();

    void reset();

    // Sample step called from simulation loop (samples at exact 10 Hz intervals)
    void update(float simTime, const std::vector<Player>& players, const Ball& ball);

    // Export captured data to CSV files
    bool flushToCSV(const std::string& trackingPath = "data/tracking.csv");
    bool exportPlayersCSV(const std::vector<Player>& players, const std::string& playersPath = "data/players.csv");
    bool exportMatchCSV(int homeScore, int awayScore, float matchDurationSeconds, const std::string& matchPath = "data/matches.csv");

    size_t getSampleCount() const { return m_sampleCount; }
    float getSampleRateHz() const { return m_sampleRateHz; }

private:
    float m_sampleRateHz{10.0f};
    float m_sampleInterval{0.1f};
    float m_timeSinceLastSample{0.0f};
    size_t m_sampleCount{0};

    std::ofstream m_trackingFile;
    bool m_fileInitialized{false};

    std::string roleToString(TacticalRole role) const;
    std::string stateToString(PlayerState state) const;
};
