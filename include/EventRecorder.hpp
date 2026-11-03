#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "Entities.hpp"

enum class MatchEventType {
    PassAttempt,
    PassCompleted,
    ShotOffTarget,
    ShotOnTarget,
    Goal,
    GoalkeeperSave,
    TackleSuccess,
    Interception,
    PossessionTurnover,
    KickoffEvent,
    SetPieceCorner,
    FoulCommitted,
    YellowCard,
    RedCard,
    PenaltyAwarded
};

struct MatchEvent {
    float timestamp{0.0f};
    MatchEventType type{MatchEventType::PassAttempt};
    TeamSide team{TeamSide::Home};
    int playerId{-1};
    int targetPlayerId{-1};
    Vec2 location{0.0f, 0.0f};
    std::string details;
};

class EventRecorder {
public:
    EventRecorder();
    ~EventRecorder();

    void reset();
    void recordEvent(const MatchEvent& event);
    void flushToCSV(const std::string& filename = "data/events.csv");

    const std::vector<MatchEvent>& getEvents() const { return m_events; }

    // Statistical queries
    int getPassCount(TeamSide team, bool completedOnly = false) const;
    int getShotCount(TeamSide team, bool onTargetOnly = false) const;
    int getGoalCount(TeamSide team) const;
    int getSaveCount(TeamSide team) const;
    int getTackleCount(TeamSide team) const;
    int getCornerCount(TeamSide team) const;
    int getFoulCount(TeamSide team) const;
    int getYellowCardCount(TeamSide team) const;
    int getRedCardCount(TeamSide team) const;
    int getPenaltyCount(TeamSide team) const;

    static std::string eventTypeToString(MatchEventType type);

private:
    std::vector<MatchEvent> m_events;
    std::ofstream m_file;
    bool m_fileOpen{false};
};
