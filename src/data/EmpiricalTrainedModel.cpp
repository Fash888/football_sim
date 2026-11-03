#include "EmpiricalTrainedModel.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

const EmpiricalTrainedModel::ModelMetadata& EmpiricalTrainedModel::getMetadata() {
    static const ModelMetadata meta = {
        105,
        108807,
        2577,
        16.17f,
        7.36f,
        0.7425f,
        0.8268f,
        0.3092f,
        0.5340f,
        0.1568f
    };
    return meta;
}

const std::array<std::array<float, 18>, 18> EmpiricalTrainedModel::s_zoneTransitionMatrix = {{
        {0.636852f, 0.146889f, 0.041344f, 0.015106f, 0.003975f, 0.000199f, 0.095607f, 0.026834f, 0.007553f, 0.005963f, 0.001193f, 0.000398f, 0.005764f, 0.009342f, 0.001590f, 0.001391f, 0.000000f, 0.000000f},
        {0.030329f, 0.661284f, 0.128806f, 0.017436f, 0.007429f, 0.000614f, 0.032785f, 0.069376f, 0.025479f, 0.003500f, 0.002394f, 0.000123f, 0.001044f, 0.011419f, 0.005464f, 0.000798f, 0.001535f, 0.000184f},
        {0.001676f, 0.055337f, 0.679373f, 0.103301f, 0.015541f, 0.002932f, 0.005320f, 0.022244f, 0.076366f, 0.015960f, 0.003644f, 0.000754f, 0.000000f, 0.002681f, 0.008420f, 0.003058f, 0.002765f, 0.000628f},
        {0.000256f, 0.003663f, 0.069691f, 0.709010f, 0.104835f, 0.008435f, 0.000085f, 0.001406f, 0.025218f, 0.049159f, 0.011715f, 0.003323f, 0.000000f, 0.000213f, 0.001491f, 0.004814f, 0.004345f, 0.002343f},
        {0.000000f, 0.000163f, 0.004567f, 0.076066f, 0.757395f, 0.074761f, 0.000109f, 0.000217f, 0.001033f, 0.014735f, 0.036483f, 0.023760f, 0.000000f, 0.000000f, 0.000000f, 0.001196f, 0.003860f, 0.005655f},
        {0.000000f, 0.000435f, 0.000290f, 0.003478f, 0.106506f, 0.721924f, 0.000000f, 0.000000f, 0.000000f, 0.000145f, 0.013476f, 0.143313f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.001739f, 0.008694f},
        {0.047117f, 0.052067f, 0.013563f, 0.023205f, 0.005914f, 0.000386f, 0.587388f, 0.081057f, 0.007264f, 0.027512f, 0.008228f, 0.003471f, 0.050588f, 0.047953f, 0.014463f, 0.023205f, 0.006299f, 0.000321f},
        {0.003308f, 0.062633f, 0.044017f, 0.006224f, 0.004542f, 0.000449f, 0.035382f, 0.626949f, 0.092856f, 0.006448f, 0.002804f, 0.000224f, 0.003477f, 0.060054f, 0.040597f, 0.006617f, 0.002972f, 0.000449f},
        {0.000202f, 0.008187f, 0.070054f, 0.037911f, 0.008066f, 0.002460f, 0.004235f, 0.029482f, 0.647348f, 0.064005f, 0.004799f, 0.000605f, 0.000323f, 0.009639f, 0.067957f, 0.033999f, 0.008590f, 0.002138f},
        {0.000000f, 0.001070f, 0.011541f, 0.071388f, 0.037158f, 0.006024f, 0.000338f, 0.007150f, 0.058608f, 0.637991f, 0.049094f, 0.003209f, 0.000000f, 0.000957f, 0.010190f, 0.064745f, 0.034287f, 0.006249f},
        {0.000000f, 0.000109f, 0.000000f, 0.011109f, 0.060880f, 0.028316f, 0.000000f, 0.000218f, 0.001851f, 0.040405f, 0.710412f, 0.047593f, 0.000000f, 0.000000f, 0.000218f, 0.008931f, 0.062187f, 0.027772f},
        {0.000000f, 0.000000f, 0.000313f, 0.000625f, 0.006254f, 0.028455f, 0.000000f, 0.000000f, 0.000000f, 0.000625f, 0.034396f, 0.893371f, 0.000313f, 0.000000f, 0.000000f, 0.000313f, 0.005941f, 0.029393f},
        {0.004566f, 0.006088f, 0.000761f, 0.000951f, 0.000190f, 0.000000f, 0.094749f, 0.026636f, 0.009513f, 0.007801f, 0.002283f, 0.000000f, 0.650114f, 0.135464f, 0.039574f, 0.016172f, 0.004947f, 0.000190f},
        {0.000182f, 0.011537f, 0.004979f, 0.001275f, 0.001275f, 0.000000f, 0.035764f, 0.067946f, 0.027871f, 0.004250f, 0.002854f, 0.000061f, 0.029571f, 0.671686f, 0.114883f, 0.017305f, 0.007529f, 0.001032f},
        {0.000000f, 0.002544f, 0.009518f, 0.002544f, 0.003597f, 0.000746f, 0.005483f, 0.025002f, 0.068295f, 0.016844f, 0.003904f, 0.001184f, 0.002456f, 0.055882f, 0.680586f, 0.100930f, 0.016712f, 0.003772f},
        {0.000000f, 0.000226f, 0.001537f, 0.003482f, 0.005336f, 0.002713f, 0.000090f, 0.001764f, 0.024734f, 0.050328f, 0.013972f, 0.003889f, 0.000317f, 0.003346f, 0.066652f, 0.707710f, 0.104680f, 0.009225f},
        {0.000000f, 0.000056f, 0.000000f, 0.001002f, 0.003118f, 0.005122f, 0.000056f, 0.000111f, 0.000724f, 0.014808f, 0.035740f, 0.027056f, 0.000000f, 0.000000f, 0.003730f, 0.068084f, 0.761064f, 0.079330f},
        {0.000144f, 0.000144f, 0.000000f, 0.000000f, 0.002162f, 0.009079f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.012250f, 0.144401f, 0.000000f, 0.000144f, 0.000000f, 0.005332f, 0.099438f, 0.726906f}
}};

float EmpiricalTrainedModel::getPassCompletionProbability(float distanceM, bool underPressure) {
    // Empirical baseline completion rates by distance bins from 108,807 passes:
    // [0, 10): 79.66%
    // [10, 20): 90.04%
    // [20, 30): 83.98%
    // [30, 40): 70.00%
    // [40, 100): 45.13%
    float baseProb = 0.85f;
    if (distanceM < 10.0f) {
        float t = std::max(0.0f, distanceM / 10.0f);
        baseProb = 0.7966f + t * (0.9004f - 0.7966f);
    } else if (distanceM < 20.0f) {
        float t = (distanceM - 10.0f) / 10.0f;
        baseProb = 0.9004f - t * (0.9004f - 0.8398f);
    } else if (distanceM < 30.0f) {
        float t = (distanceM - 20.0f) / 10.0f;
        baseProb = 0.8398f - t * (0.8398f - 0.7000f);
    } else if (distanceM < 45.0f) {
        float t = (distanceM - 30.0f) / 15.0f;
        baseProb = 0.7000f - t * (0.7000f - 0.4513f);
    } else {
        float t = std::min(1.0f, (distanceM - 45.0f) / 25.0f);
        baseProb = 0.4513f - t * 0.20f;
    }

    // Pressure factor adjustment derived from empirical data (74.25% vs 82.68% -> ~0.898x)
    if (underPressure) {
        baseProb *= 0.898f;
    }
    return std::max(0.10f, std::min(0.98f, baseProb));
}

float EmpiricalTrainedModel::getPassDistanceWeight(float distanceM) {
    // Professional football pass frequency density peak is between 12m and 22m
    if (distanceM < 4.0f) return 0.4f;
    if (distanceM <= 22.0f) {
        return 1.25f - std::abs(distanceM - 15.0f) * 0.025f;
    } else if (distanceM <= 35.0f) {
        return 0.95f - (distanceM - 22.0f) * 0.020f;
    } else {
        return std::max(0.20f, 0.69f - (distanceM - 35.0f) * 0.025f);
    }
}

float EmpiricalTrainedModel::getPassAngleWeight(float angleRad) {
    // Empirical distribution: sideways/angling (53.4%), forward (30.9%), backward (15.7%)
    float absAng = std::abs(angleRad);
    if (absAng < 0.785f) { // Forward
        return 1.15f;
    } else if (absAng < 2.356f) { // Sideways / diagonal
        return 1.25f;
    } else { // Backward
        return 0.85f;
    }
}

float EmpiricalTrainedModel::getCalibratedShotXG(float distanceToGoalM, float angleToGoalRad) {
    // Empirical calibration against 2,577 real shots in World Cup:
    // < 8m:  avg xG 0.2566, conversion 25.66%
    // 8-14m: avg xG 0.2059, conversion 19.78%
    // 14-20m: avg xG 0.0644, conversion 6.06%
    // 20-26m: avg xG 0.0316, conversion 2.86%
    // > 26m: avg xG 0.0140, conversion 2.04%
    float dist = std::max(1.0f, distanceToGoalM);
    float angleFactor = std::max(0.05f, std::cos(angleToGoalRad * 0.5f));

    float rawXG = 0.0f;
    if (dist < 8.0f) {
        float t = dist / 8.0f;
        rawXG = 0.38f - t * (0.38f - 0.2059f);
    } else if (dist < 14.0f) {
        float t = (dist - 8.0f) / 6.0f;
        rawXG = 0.2059f - t * (0.2059f - 0.0644f);
    } else if (dist < 20.0f) {
        float t = (dist - 14.0f) / 6.0f;
        rawXG = 0.0644f - t * (0.0644f - 0.0316f);
    } else if (dist < 26.0f) {
        float t = (dist - 20.0f) / 6.0f;
        rawXG = 0.0316f - t * (0.0316f - 0.0140f);
    } else {
        float t = std::min(1.0f, (dist - 26.0f) / 14.0f);
        rawXG = std::max(0.005f, 0.0140f - t * 0.010f);
    }

    return std::max(0.005f, std::min(0.95f, rawXG * angleFactor));
}

void EmpiricalTrainedModel::getShotOutcomeProbabilities(float distanceToGoalM, float& outGoal, float& outSaved, float& outBlocked, float& outOffTarget) {
    // Empirical conditioned distributions
    if (distanceToGoalM < 10.0f) {
        outGoal = 0.26f;
        outSaved = 0.32f;
        outBlocked = 0.24f;
        outOffTarget = 0.18f;
    } else if (distanceToGoalM < 18.0f) {
        outGoal = 0.11f;
        outSaved = 0.28f;
        outBlocked = 0.29f;
        outOffTarget = 0.32f;
    } else {
        outGoal = 0.03f;
        outSaved = 0.22f;
        outBlocked = 0.30f;
        outOffTarget = 0.45f;
    }
}

int EmpiricalTrainedModel::getZone18Index(float xM, float yM) {
    int zx = static_cast<int>(xM / (105.0f / 6.0f));
    zx = std::max(0, std::min(5, zx));
    int zy = static_cast<int>(yM / (68.0f / 3.0f));
    zy = std::max(0, std::min(2, zy));
    return zx + 6 * zy;
}

Vec2 EmpiricalTrainedModel::getZone18Center(int zoneIdx) {
    if (zoneIdx < 0 || zoneIdx >= 18) zoneIdx = 9;
    int zx = zoneIdx % 6;
    int zy = zoneIdx / 6;
    float x = (zx + 0.5f) * (105.0f / 6.0f);
    float y = (zy + 0.5f) * (68.0f / 3.0f);
    return Vec2(x, y);
}

float EmpiricalTrainedModel::getZoneTransitionProbability(int fromZone, int toZone) {
    if (fromZone < 0 || fromZone >= 18 || toZone < 0 || toZone >= 18) return 0.0555f;
    return s_zoneTransitionMatrix[fromZone][toZone];
}

int EmpiricalTrainedModel::getMostProbableNextZone(int currentZone) {
    if (currentZone < 0 || currentZone >= 18) return 9;
    int bestNext = currentZone;
    float bestProb = -1.0f;
    for (int nextZ = 0; nextZ < 18; ++nextZ) {
        if (nextZ == currentZone) continue; // find next progressive/lateral zone
        if (s_zoneTransitionMatrix[currentZone][nextZ] > bestProb) {
            bestProb = s_zoneTransitionMatrix[currentZone][nextZ];
            bestNext = nextZ;
        }
    }
    return bestNext;
}

std::string EmpiricalTrainedModel::getTrainingSummaryString() {
    const auto& meta = getMetadata();
    std::ostringstream ss;
    ss << "TRAINED ON " << meta.matchesTrained << " REAL MATCHES (" 
       << meta.totalPassesAnalyzed << " passes, " 
       << meta.totalShotsAnalyzed << " shots)";
    return ss.str();
}
