#include "EventRecorder.hpp"
#include <iomanip>
#include <iostream>

EventRecorder::EventRecorder() {
    reset();
}

EventRecorder::~EventRecorder() {
    if (m_file.is_open()) {
        m_file.flush();
        m_file.close();
    }
}

void EventRecorder::reset() {
    m_events.clear();
    if (m_file.is_open()) m_file.close();

    m_file.open("data/events.csv", std::ios::out | std::ios::trunc);
    if (m_file.is_open()) {
        m_fileOpen = true;
        m_file << "timestamp,event_type,team,player_id,target_player_id,x_m,y_m,details\n";
    } else {
        m_fileOpen = false;
        std::cerr << "[EventRecorder] Warning: Could not open data/events.csv\n";
    }
}

void EventRecorder::recordEvent(const MatchEvent& event) {
    m_events.push_back(event);

    if (m_fileOpen && m_file.is_open()) {
        m_file << std::fixed << std::setprecision(2);
        m_file << event.timestamp << ","
               << eventTypeToString(event.type) << ","
               << (event.team == TeamSide::Home ? "Home" : "Away") << ","
               << event.playerId << ","
               << event.targetPlayerId << ","
               << event.location.x << "," << event.location.y << ","
               << "\"" << event.details << "\"\n";
        m_file.flush();
    }
}

void EventRecorder::flushToCSV(const std::string& /*filename*/) {
    if (m_file.is_open()) {
        m_file.flush();
    }
}

int EventRecorder::getPassCount(TeamSide team, bool completedOnly) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team) {
            if (completedOnly && ev.type == MatchEventType::PassCompleted) count++;
            else if (!completedOnly && (ev.type == MatchEventType::PassAttempt || ev.type == MatchEventType::PassCompleted)) count++;
        }
    }
    return count;
}

int EventRecorder::getShotCount(TeamSide team, bool onTargetOnly) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team) {
            if (onTargetOnly && ev.type == MatchEventType::ShotOnTarget) count++;
            else if (!onTargetOnly && (ev.type == MatchEventType::ShotOffTarget || ev.type == MatchEventType::ShotOnTarget)) count++;
        }
    }
    return count;
}

int EventRecorder::getGoalCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && ev.type == MatchEventType::Goal) count++;
    }
    return count;
}

int EventRecorder::getSaveCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && ev.type == MatchEventType::GoalkeeperSave) count++;
    }
    return count;
}

int EventRecorder::getTackleCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && (ev.type == MatchEventType::TackleSuccess || ev.type == MatchEventType::Interception)) count++;
    }
    return count;
}

int EventRecorder::getCornerCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && ev.type == MatchEventType::SetPieceCorner) count++;
    }
    return count;
}

int EventRecorder::getFoulCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && (ev.type == MatchEventType::FoulCommitted || ev.type == MatchEventType::YellowCard || ev.type == MatchEventType::RedCard)) count++;
    }
    return count;
}

int EventRecorder::getYellowCardCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && ev.type == MatchEventType::YellowCard) count++;
    }
    return count;
}

int EventRecorder::getRedCardCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && ev.type == MatchEventType::RedCard) count++;
    }
    return count;
}

int EventRecorder::getPenaltyCount(TeamSide team) const {
    int count = 0;
    for (const auto& ev : m_events) {
        if (ev.team == team && ev.type == MatchEventType::PenaltyAwarded) count++;
    }
    return count;
}

std::string EventRecorder::eventTypeToString(MatchEventType type) {
    switch (type) {
        case MatchEventType::PassAttempt:        return "PASS_ATTEMPT";
        case MatchEventType::PassCompleted:      return "PASS_COMPLETED";
        case MatchEventType::ShotOffTarget:      return "SHOT_OFF_TARGET";
        case MatchEventType::ShotOnTarget:       return "SHOT_ON_TARGET";
        case MatchEventType::Goal:               return "GOAL";
        case MatchEventType::GoalkeeperSave:     return "GK_SAVE";
        case MatchEventType::TackleSuccess:      return "TACKLE";
        case MatchEventType::Interception:       return "INTERCEPTION";
        case MatchEventType::PossessionTurnover:  return "TURNOVER";
        case MatchEventType::KickoffEvent:       return "KICKOFF";
        case MatchEventType::SetPieceCorner:     return "SET_PIECE_CORNER";
        case MatchEventType::FoulCommitted:      return "FOUL";
        case MatchEventType::YellowCard:         return "YELLOW_CARD";
        case MatchEventType::RedCard:            return "RED_CARD";
        case MatchEventType::PenaltyAwarded:     return "PENALTY";
        default:                                 return "UNKNOWN";
    }
}
