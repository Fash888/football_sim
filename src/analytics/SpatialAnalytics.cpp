#include "SpatialAnalytics.hpp"
#include <algorithm>

SpatialAnalytics::SpatialAnalytics() {
    reset();
}

void SpatialAnalytics::reset() {
    m_heatmaps.clear();
    m_posSum.clear();
    m_sampleCounts.clear();
    m_averagePositions.clear();
    m_passConnections.clear();
}

void SpatialAnalytics::recordPositions(const std::vector<Player>& players) {
    for (const auto& p : players) {
        // Discretize into 21 x 14 grid
        int col = std::clamp(static_cast<int>((p.position.x / SimConfig::PITCH_LENGTH_M) * HEATMAP_COLS), 0, HEATMAP_COLS - 1);
        int row = std::clamp(static_cast<int>((p.position.y / SimConfig::PITCH_WIDTH_M) * HEATMAP_ROWS), 0, HEATMAP_ROWS - 1);

        auto& hm = m_heatmaps[p.id];
        hm.counts[col][row]++;
        if (hm.counts[col][row] > hm.maxCount) {
            hm.maxCount = hm.counts[col][row];
        }

        // Accumulate for average position
        m_posSum[p.id] += p.position;
        m_sampleCounts[p.id]++;
        m_averagePositions[p.id] = m_posSum[p.id] / static_cast<float>(m_sampleCounts[p.id]);
    }
}

void SpatialAnalytics::recordPass(int fromId, int toId, TeamSide team) {
    if (fromId == toId || fromId <= 0 || toId <= 0) return;

    for (auto& conn : m_passConnections) {
        if (conn.fromPlayerId == fromId && conn.toPlayerId == toId) {
            conn.passCount++;
            return;
        }
    }

    m_passConnections.push_back({fromId, toId, team, 1});
}

const PlayerHeatmap* SpatialAnalytics::getHeatmap(int playerId) const {
    auto it = m_heatmaps.find(playerId);
    if (it != m_heatmaps.end()) {
        return &it->second;
    }
    return nullptr;
}

static float crossProduct2D(const Vec2& O, const Vec2& A, const Vec2& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

std::vector<Vec2> SpatialAnalytics::computeConvexHull(std::vector<Vec2> points) {
    size_t n = points.size();
    if (n <= 2) return points;

    std::sort(points.begin(), points.end(), [](const Vec2& a, const Vec2& b) {
        if (std::abs(a.x - b.x) > 0.001f) return a.x < b.x;
        return a.y < b.y;
    });

    std::vector<Vec2> hull;
    hull.reserve(2 * n);

    // Lower hull
    for (size_t i = 0; i < n; ++i) {
        while (hull.size() >= 2 && crossProduct2D(hull[hull.size() - 2], hull[hull.size() - 1], points[i]) <= 0.0001f) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }

    // Upper hull
    size_t lowerSize = hull.size();
    for (int i = static_cast<int>(n) - 2; i >= 0; --i) {
        while (hull.size() > lowerSize && crossProduct2D(hull[hull.size() - 2], hull[hull.size() - 1], points[i]) <= 0.0001f) {
            hull.pop_back();
        }
        hull.push_back(points[i]);
    }

    hull.pop_back();
    return hull;
}

TeamSpatialShape SpatialAnalytics::calculateTeamShape(const std::vector<Player>& players, TeamSide team, bool excludeGK) {
    TeamSpatialShape shape;
    std::vector<Vec2> outfieldPoints;
    outfieldPoints.reserve(11);

    Vec2 sumPos{0.0f, 0.0f};
    float minX = 1e9f, maxX = -1e9f;
    float minY = 1e9f, maxY = -1e9f;

    for (const auto& p : players) {
        if (p.team != team) continue;
        if (excludeGK && p.role == TacticalRole::GK) continue;

        outfieldPoints.push_back(p.position);
        sumPos += p.position;

        if (p.position.x < minX) minX = p.position.x;
        if (p.position.x > maxX) maxX = p.position.x;
        if (p.position.y < minY) minY = p.position.y;
        if (p.position.y > maxY) maxY = p.position.y;
    }

    if (outfieldPoints.empty()) return shape;

    shape.centroid = sumPos / static_cast<float>(outfieldPoints.size());
    shape.lengthM = std::max(0.0f, maxX - minX);
    shape.widthM = std::max(0.0f, maxY - minY);

    shape.hullVertices = computeConvexHull(outfieldPoints);

    // Shoelace formula for polygon area
    float areaSum = 0.0f;
    size_t hn = shape.hullVertices.size();
    if (hn >= 3) {
        for (size_t i = 0; i < hn; ++i) {
            size_t next = (i + 1) % hn;
            areaSum += (shape.hullVertices[i].x * shape.hullVertices[next].y) - (shape.hullVertices[next].x * shape.hullVertices[i].y);
        }
        shape.areaM2 = std::abs(areaSum) * 0.5f;
    }

    return shape;
}
