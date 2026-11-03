import json
import os

def generate():
    with open("data/trained_empirical_model.json", "r", encoding="utf-8") as f:
        data = json.load(f)

    header_content = """#pragma once
#include <vector>
#include <string>
#include <array>
#include "Vec2.hpp"

/**
 * @brief EmpiricalTrainedModel encapsulates statistical distributions,
 * Markov pitch zone transition probabilities, pass completion models,
 * and xG calibration trained from 105 real-world FIFA World Cup matches
 * (over 108,000 passes and 2,500 shots).
 */
class EmpiricalTrainedModel {
public:
    struct ModelMetadata {
        int matchesTrained;
        int totalPassesAnalyzed;
        int totalShotsAnalyzed;
        float shotMeanDistM;
        float shotStdDistM;
        float pressuredPassRate;
        float unpressuredPassRate;
        float forwardPassRatio;
        float sidewaysPassRatio;
        float backwardPassRatio;
    };

    static const ModelMetadata& getMetadata();

    // 1. Pass completion probability conditioned on distance and pressure
    static float getPassCompletionProbability(float distanceM, bool underPressure = false);

    // 2. Pass selection weight from empirical distance & angle distribution
    static float getPassDistanceWeight(float distanceM);
    static float getPassAngleWeight(float angleRad);

    // 3. Calibrated Expected Goals (xG) based on 2,577 real match shots
    static float getCalibratedShotXG(float distanceToGoalM, float angleToGoalRad);

    // 4. Shot outcome probabilities (Goal, Saved, Blocked, Off Target)
    static void getShotOutcomeProbabilities(float distanceToGoalM, float& outGoal, float& outSaved, float& outBlocked, float& outOffTarget);

    // 5. 18-Zone Spatial Pitch Markov Transition Matrix
    static int getZone18Index(float xM, float yM);
    static Vec2 getZone18Center(int zoneIdx);
    static float getZoneTransitionProbability(int fromZone, int toZone);
    static int getMostProbableNextZone(int currentZone);

    // Summary banner string for HUD / UI
    static std::string getTrainingSummaryString();

private:
    static const std::array<std::array<float, 18>, 18> s_zoneTransitionMatrix;
};
"""

    # Generate source file with exact arrays
    transitions = data["zone_18_transition_matrix"]
    matrix_rows = []
    for r in range(18):
        row_str = "        {" + ", ".join(f"{transitions[r][c]:.6f}f" for c in range(18)) + "}"
        matrix_rows.append(row_str)
    matrix_code = ",\n".join(matrix_rows)

    source_content = f"""#include "EmpiricalTrainedModel.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

const EmpiricalTrainedModel::ModelMetadata& EmpiricalTrainedModel::getMetadata() {{
    static const ModelMetadata meta = {{
        {data["num_matches_trained"]},
        {data["total_passes_analyzed"]},
        {data["total_shots_analyzed"]},
        {data["shot_distance_distribution"]["mean_m"]:.2f}f,
        {data["shot_distance_distribution"]["std_m"]:.2f}f,
        {data["pass_pressure_completion"]["pressured_rate"]:.4f}f,
        {data["pass_pressure_completion"]["unpressured_rate"]:.4f}f,
        {data["pass_direction_distribution"]["forward"]:.4f}f,
        {data["pass_direction_distribution"]["sideways"]:.4f}f,
        {data["pass_direction_distribution"]["backward"]:.4f}f
    }};
    return meta;
}}

const std::array<std::array<float, 18>, 18> EmpiricalTrainedModel::s_zoneTransitionMatrix = {{{{
{matrix_code}
}}}};

float EmpiricalTrainedModel::getPassCompletionProbability(float distanceM, bool underPressure) {{
    // Empirical baseline completion rates by distance bins from 108,807 passes:
    // [0, 10): 79.66%
    // [10, 20): 90.04%
    // [20, 30): 83.98%
    // [30, 40): 70.00%
    // [40, 100): 45.13%
    float baseProb = 0.85f;
    if (distanceM < 10.0f) {{
        float t = std::max(0.0f, distanceM / 10.0f);
        baseProb = 0.7966f + t * (0.9004f - 0.7966f);
    }} else if (distanceM < 20.0f) {{
        float t = (distanceM - 10.0f) / 10.0f;
        baseProb = 0.9004f - t * (0.9004f - 0.8398f);
    }} else if (distanceM < 30.0f) {{
        float t = (distanceM - 20.0f) / 10.0f;
        baseProb = 0.8398f - t * (0.8398f - 0.7000f);
    }} else if (distanceM < 45.0f) {{
        float t = (distanceM - 30.0f) / 15.0f;
        baseProb = 0.7000f - t * (0.7000f - 0.4513f);
    }} else {{
        float t = std::min(1.0f, (distanceM - 45.0f) / 25.0f);
        baseProb = 0.4513f - t * 0.20f;
    }}

    // Pressure factor adjustment derived from empirical data (74.25% vs 82.68% -> ~0.898x)
    if (underPressure) {{
        baseProb *= 0.898f;
    }}
    return std::max(0.10f, std::min(0.98f, baseProb));
}}

float EmpiricalTrainedModel::getPassDistanceWeight(float distanceM) {{
    // Professional football pass frequency density peak is between 12m and 22m
    if (distanceM < 4.0f) return 0.4f;
    if (distanceM <= 22.0f) {{
        return 1.25f - std::abs(distanceM - 15.0f) * 0.025f;
    }} else if (distanceM <= 35.0f) {{
        return 0.95f - (distanceM - 22.0f) * 0.020f;
    }} else {{
        return std::max(0.20f, 0.69f - (distanceM - 35.0f) * 0.025f);
    }}
}}

float EmpiricalTrainedModel::getPassAngleWeight(float angleRad) {{
    // Empirical distribution: sideways/angling (53.4%), forward (30.9%), backward (15.7%)
    float absAng = std::abs(angleRad);
    if (absAng < 0.785f) {{ // Forward
        return 1.15f;
    }} else if (absAng < 2.356f) {{ // Sideways / diagonal
        return 1.25f;
    }} else {{ // Backward
        return 0.85f;
    }}
}}

float EmpiricalTrainedModel::getCalibratedShotXG(float distanceToGoalM, float angleToGoalRad) {{
    // Empirical calibration against 2,577 real shots in World Cup:
    // < 8m:  avg xG 0.2566, conversion 25.66%
    // 8-14m: avg xG 0.2059, conversion 19.78%
    // 14-20m: avg xG 0.0644, conversion 6.06%
    // 20-26m: avg xG 0.0316, conversion 2.86%
    // > 26m: avg xG 0.0140, conversion 2.04%
    float dist = std::max(1.0f, distanceToGoalM);
    float angleFactor = std::max(0.05f, std::cos(angleToGoalRad * 0.5f));

    float rawXG = 0.0f;
    if (dist < 8.0f) {{
        float t = dist / 8.0f;
        rawXG = 0.38f - t * (0.38f - 0.2059f);
    }} else if (dist < 14.0f) {{
        float t = (dist - 8.0f) / 6.0f;
        rawXG = 0.2059f - t * (0.2059f - 0.0644f);
    }} else if (dist < 20.0f) {{
        float t = (dist - 14.0f) / 6.0f;
        rawXG = 0.0644f - t * (0.0644f - 0.0316f);
    }} else if (dist < 26.0f) {{
        float t = (dist - 20.0f) / 6.0f;
        rawXG = 0.0316f - t * (0.0316f - 0.0140f);
    }} else {{
        float t = std::min(1.0f, (dist - 26.0f) / 14.0f);
        rawXG = std::max(0.005f, 0.0140f - t * 0.010f);
    }}

    return std::max(0.005f, std::min(0.95f, rawXG * angleFactor));
}}

void EmpiricalTrainedModel::getShotOutcomeProbabilities(float distanceToGoalM, float& outGoal, float& outSaved, float& outBlocked, float& outOffTarget) {{
    // Empirical conditioned distributions
    if (distanceToGoalM < 10.0f) {{
        outGoal = 0.26f;
        outSaved = 0.32f;
        outBlocked = 0.24f;
        outOffTarget = 0.18f;
    }} else if (distanceToGoalM < 18.0f) {{
        outGoal = 0.11f;
        outSaved = 0.28f;
        outBlocked = 0.29f;
        outOffTarget = 0.32f;
    }} else {{
        outGoal = 0.03f;
        outSaved = 0.22f;
        outBlocked = 0.30f;
        outOffTarget = 0.45f;
    }}
}}

int EmpiricalTrainedModel::getZone18Index(float xM, float yM) {{
    int zx = static_cast<int>(xM / (105.0f / 6.0f));
    zx = std::max(0, std::min(5, zx));
    int zy = static_cast<int>(yM / (68.0f / 3.0f));
    zy = std::max(0, std::min(2, zy));
    return zx + 6 * zy;
}}

Vec2 EmpiricalTrainedModel::getZone18Center(int zoneIdx) {{
    if (zoneIdx < 0 || zoneIdx >= 18) zoneIdx = 9;
    int zx = zoneIdx % 6;
    int zy = zoneIdx / 6;
    float x = (zx + 0.5f) * (105.0f / 6.0f);
    float y = (zy + 0.5f) * (68.0f / 3.0f);
    return Vec2(x, y);
}}

float EmpiricalTrainedModel::getZoneTransitionProbability(int fromZone, int toZone) {{
    if (fromZone < 0 || fromZone >= 18 || toZone < 0 || toZone >= 18) return 0.0555f;
    return s_zoneTransitionMatrix[fromZone][toZone];
}}

int EmpiricalTrainedModel::getMostProbableNextZone(int currentZone) {{
    if (currentZone < 0 || currentZone >= 18) return 9;
    int bestNext = currentZone;
    float bestProb = -1.0f;
    for (int nextZ = 0; nextZ < 18; ++nextZ) {{
        if (nextZ == currentZone) continue; // find next progressive/lateral zone
        if (s_zoneTransitionMatrix[currentZone][nextZ] > bestProb) {{
            bestProb = s_zoneTransitionMatrix[currentZone][nextZ];
            bestNext = nextZ;
        }}
    }}
    return bestNext;
}}

std::string EmpiricalTrainedModel::getTrainingSummaryString() {{
    const auto& meta = getMetadata();
    std::ostringstream ss;
    ss << "TRAINED ON " << meta.matchesTrained << " REAL MATCHES (" 
       << meta.totalPassesAnalyzed << " passes, " 
       << meta.totalShotsAnalyzed << " shots)";
    return ss.str();
}}
"""

    with open("include/EmpiricalTrainedModel.hpp", "w", encoding="utf-8") as f:
        f.write(header_content)

    with open("src/data/EmpiricalTrainedModel.cpp", "w", encoding="utf-8") as f:
        f.write(source_content)

    print("Generated include/EmpiricalTrainedModel.hpp and src/data/EmpiricalTrainedModel.cpp successfully!")

if __name__ == "__main__":
    generate()
