#pragma once
#include <vector>
#include <map>
#include "Config.hpp"
#include "Entities.hpp"
#include "EventRecorder.hpp"

// A 2D spatial grid over the 105m x 68m pitch (e.g. 21 x 14 cells = 5m x ~4.85m bins)
constexpr int HEATMAP_COLS = 21;
constexpr int HEATMAP_ROWS = 14;

struct PlayerHeatmap {
    int counts[HEATMAP_COLS][HEATMAP_ROWS]{{0}};
    int maxCount{1};
};

struct PassConnection {
    int fromPlayerId{0};
    int toPlayerId{0};
    TeamSide team{TeamSide::Home};
    int passCount{0};
};

struct TeamSpatialShape {
    Vec2 centroid{52.5f, 34.0f};
    float lengthM{0.0f}; // Longitudinal depth
    float widthM{0.0f};  // Lateral spread
    float areaM2{0.0f};  // Convex hull area in square meters
    std::vector<Vec2> hullVertices; // 2D polygon vertices in counter-clockwise order
};

class SpatialAnalytics {
public:
    SpatialAnalytics();

    void reset();
    void recordPositions(const std::vector<Player>& players);
    void recordPass(int fromId, int toId, TeamSide team);

    const PlayerHeatmap* getHeatmap(int playerId) const;
    const std::map<int, Vec2>& getAveragePositions() const { return m_averagePositions; }
    const std::vector<PassConnection>& getPassConnections() const { return m_passConnections; }

    static TeamSpatialShape calculateTeamShape(const std::vector<Player>& players, TeamSide team, bool excludeGK = true);
    static std::vector<Vec2> computeConvexHull(std::vector<Vec2> points);

private:
    std::map<int, PlayerHeatmap> m_heatmaps;
    std::map<int, Vec2> m_posSum;
    std::map<int, int> m_sampleCounts;
    std::map<int, Vec2> m_averagePositions;

    std::vector<PassConnection> m_passConnections;
};
