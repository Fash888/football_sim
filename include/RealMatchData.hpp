#pragma once
#include <string>
#include <vector>
#include "Config.hpp"
#include "Vec2.hpp"
#include "Entities.hpp"

struct RealMatchPlayerTrack {
    int shirtNumber{0};
    TeamSide team{TeamSide::Home};
    bool isActive{false};
    Vec2 pitchPos{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    float speedMps{0.0f};
    float distanceCoveredM{0.0f};
    bool isGoalkeeper{false};
};

struct RealMatchEvent {
    float timestampS{0.0f};
    int startFrame{0};
    int endFrame{0};
    int period{1};
    TeamSide team{TeamSide::Home};
    std::string type;
    std::string subType;
    std::string fromPlayer;
    std::string toPlayer;
    Vec2 startPos{0.0f, 0.0f};
    Vec2 endPos{0.0f, 0.0f};
    bool isGoal{false};
    bool isShot{false};
};

struct RealMatchFrame {
    int frameNumber{0};
    float timestampS{0.0f};
    int period{1};
    std::vector<RealMatchPlayerTrack> players; // Active players on pitch (typically 11 Home + 11 Away)
    Vec2 ballPos{52.5f, 34.0f};
    float ballHeight{0.0f};
};

class RealMatchDataset {
public:
    RealMatchDataset();

    bool loadMetricaSampleGame(
        const std::string& homeTrackingCsv,
        const std::string& awayTrackingCsv,
        const std::string& eventsCsv,
        int frameStride = 2 // Downsample from 25Hz to 12.5Hz for smooth 60fps playback and lean RAM
    );

    bool isLoaded() const { return m_loaded; }
    size_t getFrameCount() const { return m_frames.size(); }
    float getDurationSeconds() const { return m_totalDurationS; }

    const RealMatchFrame* getFrameAtTime(float timeS) const;
    const RealMatchFrame* getFrameByIndex(size_t index) const;
    size_t getIndexForTime(float timeS) const;

    const std::vector<RealMatchEvent>& getEvents() const { return m_events; }
    std::string getActiveEventText(float timeS, float windowSeconds = 3.5f) const;

    int getHomeScore(float timeS) const;
    int getAwayScore(float timeS) const;

    int getFinalHomeScore() const { return m_finalHomeScore; }
    int getFinalAwayScore() const { return m_finalAwayScore; }
    int getTotalHomePasses() const { return m_totalHomePasses; }
    int getTotalAwayPasses() const { return m_totalAwayPasses; }
    int getTotalHomeShots() const { return m_totalHomeShots; }
    int getTotalAwayShots() const { return m_totalAwayShots; }

private:
    bool m_loaded{false};
    float m_totalDurationS{0.0f};
    std::vector<RealMatchFrame> m_frames;
    std::vector<RealMatchEvent> m_events;

    int m_finalHomeScore{0};
    int m_finalAwayScore{0};
    int m_totalHomePasses{0};
    int m_totalAwayPasses{0};
    int m_totalHomeShots{0};
    int m_totalAwayShots{0};
};
