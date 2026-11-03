#pragma once
#include <vector>
#include <deque>
#include "Config.hpp"
#include "EventRecorder.hpp"

// Momentum sample recorded every second of simulation (SofaScore Attack Momentum style)
struct MomentumPoint {
    float minute{0.0f};
    float momentumValue{0.0f}; // Positive = Home dominance, Negative = Away dominance (-100 to +100)
};

struct CalibrationMetrics {
    float simulatedGoalsPerMatch{0.0f};          // Target: 2.7 - 3.0
    float simulatedRegainTimeDiffS{0.0f};        // Target: ~4.0s faster for winner
    float simulatedPressTransitionPct{0.0f};     // Target: ~40%
    float simulatedGKGoalKickPct{0.0f};          // Target: ~50%
    float simulatedSetPieceGoalPct{0.0f};        // Target: ~26%
    float simulatedAvgPassSequenceS{0.0f};       // Target: ~9.5s
    float simulatedTotalShots{0.0f};             // Target: ~25
    float simulatedAvgXG{0.0f};                  // Target: ~0.10
};

class MatchAnalytics {
public:
    MatchAnalytics();

    void reset();
    void update(float simTime, const EventRecorder& eventRecorder, const Ball& ball, TeamSide currentPossession);

    // Calibration Target Tracking
    void recordRegain(TeamSide team, float durationS);
    void recordTransition(bool fromPress);
    void recordGoalKick(bool byGK);
    void recordGoal(bool fromSetPiece);
    void recordPassSequenceDuration(float durationS);
    CalibrationMetrics getCalibrationMetrics(int homeScore, int awayScore, float matchTime) const;

    // Official SofaScore / Opta Live Statistics
    float getHomePossessionPct() const;
    float getAwayPossessionPct() const;

    // Field Tilt (% of passes in attacking 3rd)
    float getHomeFieldTiltPct() const;
    float getAwayFieldTiltPct() const;

    // Expected Goals (xG) Analytics (StatsBomb calibrated model)
    float getHomeXG() const { return m_homeXG; }
    float getAwayXG() const { return m_awayXG; }
    static float calculateShotXG(const Vec2& shotLocation, TeamSide team);

    int getHomePassesAttempted() const { return m_homePassesAttempted; }
    int getAwayPassesAttempted() const { return m_awayPassesAttempted; }
    int getHomePassesCompleted() const { return m_homePassesCompleted; }
    int getAwayPassesCompleted() const { return m_awayPassesCompleted; }

    const std::vector<MomentumPoint>& getMomentumHistory() const { return m_momentumHistory; }
    float getCurrentMomentum() const { return m_currentSmoothedMomentum; }

private:
    // Continuous sequence possession time
    float m_homePossessionTime{0.0f};
    float m_awayPossessionTime{0.0f};

    // Expected Goals (xG) cumulative sum
    float m_homeXG{0.0f};
    float m_awayXG{0.0f};

    // Opta event counters
    int m_homePassesAttempted{0};
    int m_awayPassesAttempted{0};
    int m_homePassesCompleted{0};
    int m_awayPassesCompleted{0};
    int m_homeAttackingThirdPasses{0};
    int m_awayAttackingThirdPasses{0};

    // Calibration tracking counters
    float m_homeRegainTimeSum{0.0f};
    int m_homeRegainCount{0};
    float m_awayRegainTimeSum{0.0f};
    int m_awayRegainCount{0};

    int m_pressTransitionCount{0};
    int m_totalTransitionCount{0};

    int m_gkGoalKicks{0};
    int m_cbGoalKicks{0};

    int m_setPieceGoals{0};
    int m_totalGoals{0};

    float m_passSequenceDurationSum{0.0f};
    int m_passSequenceCount{0};

    int m_totalShots{0};
    float m_totalXGSum{0.0f};

    float m_timeSinceLastSample{0.0f};
    float m_currentSmoothedMomentum{0.0f}; // Exponential moving average

    std::vector<MomentumPoint> m_momentumHistory;
    size_t m_lastProcessedEventIndex{0};
};
