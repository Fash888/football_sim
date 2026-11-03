#pragma once
#include <vector>
#include <random>
#include <array>
#include "Config.hpp"
#include "Entities.hpp"
#include "Tactics.hpp"
#include "EventRecorder.hpp"
#include "MatchAnalytics.hpp"
#include "SpatialAnalytics.hpp"
#include "RandomEngine.hpp"
#include "Teams.hpp"

enum class DecisionType {
    ShortPass,
    InsideChannelThroughBall,
    OverloadSwitch,
    DribbleDrive,
    CrossIntoBox,
    Shot
};

struct DecisionOption {
    DecisionType type{DecisionType::ShortPass};
    int targetPlayerId{-1};
    Vec2 targetPos{0.0f, 0.0f};
    float value{0.0f};
    float successProb{0.0f};
    float utility{0.0f};
    std::string description;
};

class MatchEngine {
public:
    MatchEngine(unsigned int randomSeed = 1337);

    void resetMatch();
    void setTeamFormation(TeamSide team, FormationType type);
    void setupKickoff(TeamSide kickingTeam);
    void startSecondHalf();

    // Reseed the stochastic engine
    void reseed(unsigned int seed) { m_randomEngine.reseed(seed); }
    unsigned int getSeed() const { return m_randomEngine.currentSeed(); }
    RandomEngine& getRandomEngine() { return m_randomEngine; }
    const RandomEngine& getRandomEngine() const { return m_randomEngine; }

    // Fixed timestep physics update
    void update(float dt);

    // Getters
    const std::vector<Player>& getPlayers() const { return m_players; }
    std::vector<Player>& getPlayers() { return m_players; }
    const Ball& getBall() const { return m_ball; }
    int getHomeScore() const { return m_homeScore; }
    int getAwayScore() const { return m_awayScore; }
    float getMatchTime() const { return m_matchTime; }
    MatchPhase getPhase() const { return m_phase; }
    TeamSide getPossessionTeam() const { return m_possessionTeam; }
    FormationType getHomeFormation() const { return m_homeFormation; }
    FormationType getAwayFormation() const { return m_awayFormation; }
    TeamTacticalStyle getHomeTacticalStyle() const { return m_homeTacticalStyle; }
    TeamTacticalStyle getAwayTacticalStyle() const { return m_awayTacticalStyle; }
    DefensiveMentality getHomeMentality() const { return m_homeMentality; }
    DefensiveMentality getAwayMentality() const { return m_awayMentality; }

    // Comprehensive TeamTactics
    const TeamTactics& getHomeTactics() const { return m_homeTactics; }
    const TeamTactics& getAwayTactics() const { return m_awayTactics; }
    TeamTactics& getHomeTactics() { return m_homeTactics; }
    TeamTactics& getAwayTactics() { return m_awayTactics; }

    void setHomeTacticalStyle(TeamTacticalStyle s);
    void setAwayTacticalStyle(TeamTacticalStyle s);
    void cycleHomeTacticalStyle();
    void cycleAwayTacticalStyle();

    // Real-World Club Teams & Profiles
    int getHomeTeamIndex() const { return m_homeTeamIndex; }
    int getAwayTeamIndex() const { return m_awayTeamIndex; }
    const TeamProfile& getHomeTeamProfile() const;
    const TeamProfile& getAwayTeamProfile() const;
    void selectHomeTeam(int index);
    void selectAwayTeam(int index);
    void cycleHomeTeam();
    void cycleAwayTeam();

    // Halves & Stoppage / Injury Time
    float getFirstHalfAddedTime() const { return m_firstHalfAddedTime; }
    float getSecondHalfAddedTime() const { return m_secondHalfAddedTime; }
    bool isHalfTime() const { return m_phase == MatchPhase::HalfTime; }
    bool isFullTime() const { return m_phase == MatchPhase::FullTime; }

    float calculateOffsideLine(TeamSide defendingTeam) const;

    const EventRecorder& getEventRecorder() const { return m_eventRecorder; }
    const MatchAnalytics& getAnalytics() const { return m_analytics; }
    const SpatialAnalytics& getSpatialAnalytics() const { return m_spatialAnalytics; }

private:
    void rebuildTeamLineup(TeamSide team);
    void updateBallPhysics(float dt);
    void updatePlayerAI(float dt);
    void updateGoalkeeperAI(Player& gk, float dt);
    void updatePlayerKinematics(float dt);
    void handleBallPlayerCollisions();
    void checkPitchBoundariesAndGoals();

    // Creative On-Ball Decision Evaluation & Execution
    DecisionOption evaluateBestOnBallDecision(Player& player, bool allowDribble = true);
    void executeDecision(Player& player, const DecisionOption& decision);

    // Specific Action Executions
    void executeKickoff(Player& player);
    void executeDribble(Player& player, float dt);
    void executePass(Player& passer, const DecisionOption& option);
    void executePass(Player& passer);
    void executeCross(Player& crosser);
    void executeGKDistribution(Player& gk);
    void executeShot(Player& shooter);
    void executeCorner(TeamSide attackingTeam);
    void executeGoalKick(Player& kicker);
    void executeThrowIn(TeamSide awardedTeam, const Vec2& touchlineSpot);
    void executeFreeKick(TeamSide awardedTeam, const Vec2& spot);
    void executePenaltyKick(TeamSide attackingTeam);

    int findNearestTeammateForPass(const Player& passer);

    std::vector<Player> m_players;
    Ball m_ball;

    int m_homeTeamIndex{0};
    int m_awayTeamIndex{1};

    FormationType m_homeFormation{FormationType::F_4_3_3};
    FormationType m_awayFormation{FormationType::F_4_4_2};

    TeamTacticalStyle m_homeTacticalStyle{TeamTacticalStyle::TikiTaka};
    TeamTacticalStyle m_awayTacticalStyle{TeamTacticalStyle::CounterAttack};
    DefensiveMentality m_homeMentality{DefensiveMentality::MidBlock};
    DefensiveMentality m_awayMentality{DefensiveMentality::MidBlock};

    TeamTactics m_homeTactics;
    TeamTactics m_awayTactics;

    int m_homeScore{0};
    int m_awayScore{0};
    float m_matchTime{0.0f};
    MatchPhase m_phase{MatchPhase::Kickoff};
    TeamSide m_possessionTeam{TeamSide::Home};
    TeamSide m_kickoffTeam{TeamSide::Home};

    // Stoppage / Injury Time (1 to 7 minutes randomly selected)
    float m_firstHalfAddedTime{180.0f};  // e.g. 3 mins
    float m_secondHalfAddedTime{240.0f}; // e.g. 4 mins
    bool m_firstHalfEnded{false};
    bool m_secondHalfEnded{false};

    // Counter-Press Window (3-5s after possession loss)
    float m_homeCounterPressTimer{0.0f};
    float m_awayCounterPressTimer{0.0f};
    TeamSide m_previousPossessionTeam{TeamSide::Home};

    // Sequence & Regain Tracking
    float m_possessionSequenceDuration{0.0f};
    float m_lastPossessionChangeTime{0.0f};

    // Patient Build-Up & Team Involvement Tracking
    int m_recentPasserId{-1};
    std::array<int, 22> m_possessionChainTouches{};

    float m_homeOffsideLineX{85.0f};
    float m_awayOffsideLineX{20.0f};

    float m_phaseTimer{0.0f};
    RandomEngine m_randomEngine;

    EventRecorder m_eventRecorder;
    MatchAnalytics m_analytics;
    SpatialAnalytics m_spatialAnalytics;
    float m_spatialSamplingTimer{0.0f};
};
