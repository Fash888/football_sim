#pragma once
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
