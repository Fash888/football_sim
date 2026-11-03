#include "TrackingSystem.hpp"
#include <iomanip>
#include <iostream>

TrackingSystem::TrackingSystem(float sampleFrequencyHz)
    : m_sampleRateHz(sampleFrequencyHz),
      m_sampleInterval(1.0f / sampleFrequencyHz),
      m_timeSinceLastSample(0.0f),
      m_sampleCount(0) {
    reset();
}

TrackingSystem::~TrackingSystem() {
    if (m_trackingFile.is_open()) {
        m_trackingFile.flush();
        m_trackingFile.close();
    }
}

void TrackingSystem::reset() {
    if (m_trackingFile.is_open()) {
        m_trackingFile.close();
    }

    m_trackingFile.open("data/tracking.csv", std::ios::out | std::ios::trunc);
    if (m_trackingFile.is_open()) {
        // Write header
        m_trackingFile << "timestamp,entity_type,entity_id,team_id,shirt_number,role,x_m,y_m,z_m,speed_mps,distance_covered_m,state,has_possession\n";
        m_fileInitialized = true;
    } else {
        std::cerr << "[TrackingSystem] Warning: Unable to open data/tracking.csv for writing." << std::endl;
        m_fileInitialized = false;
    }

    m_timeSinceLastSample = 0.0f;
    m_sampleCount = 0;
}

void TrackingSystem::update(float simTime, const std::vector<Player>& players, const Ball& ball) {
    m_timeSinceLastSample += SimConfig::SIMULATION_TIMESTEP;

    if (m_timeSinceLastSample < m_sampleInterval) {
        return; // Wait for exact 10 Hz sampling period
    }
    m_timeSinceLastSample -= m_sampleInterval;

    if (!m_fileInitialized || !m_trackingFile.is_open()) return;

    m_trackingFile << std::fixed << std::setprecision(2);

    // 1. Record Ball Sample
    float ballSpeed = ball.velocity.length();
    m_trackingFile << simTime << ",ball,0,"
                   << (ball.lastTouchedTeam == TeamSide::Home ? 0 : 1) << ",-1,BALL,"
                   << ball.position.x << "," << ball.position.y << "," << ball.height << ","
                   << ballSpeed << ",0.00,"
                   << (ball.possessorId != -1 ? "Possessed" : "Loose") << ","
                   << (ball.possessorId != -1 ? 1 : 0) << "\n";
    m_sampleCount++;

    // 2. Record 22 Player Samples
    for (const auto& p : players) {
        float playerSpeed = p.velocity.length();
        m_trackingFile << simTime << ",player,"
                       << p.id << ","
                       << (p.team == TeamSide::Home ? 0 : 1) << ","
                       << p.shirtNumber << ","
                       << roleToString(p.role) << ","
                       << p.position.x << "," << p.position.y << ",0.00,"
                       << playerSpeed << ","
                       << p.distanceCoveredM << ","
                       << stateToString(p.state) << ","
                       << (p.hasPossession ? 1 : 0) << "\n";
        m_sampleCount++;
    }

    // Flush occasionally to ensure durability without excessive I/O penalty
    if (m_sampleCount % 230 == 0) { // Every ~1 second of simulation
        m_trackingFile.flush();
    }
}

bool TrackingSystem::flushToCSV(const std::string& /*trackingPath*/) {
    if (m_trackingFile.is_open()) {
        m_trackingFile.flush();
        return true;
    }
    return false;
}

bool TrackingSystem::exportPlayersCSV(const std::vector<Player>& players, const std::string& playersPath) {
    std::ofstream file(playersPath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[TrackingSystem] Error: Could not write to " << playersPath << "\n";
        return false;
    }

    file << "player_id,team_name,shirt_number,role,name,distance_covered_m,max_speed_attribute,acceleration_attribute,pass_skill,shot_power\n";
    file << std::fixed << std::setprecision(2);

    for (const auto& p : players) {
        file << p.id << ","
             << (p.team == TeamSide::Home ? "Home" : "Away") << ","
             << p.shirtNumber << ","
             << roleToString(p.role) << ","
             << "\"" << p.name << "\","
             << p.distanceCoveredM << ","
             << p.attributes.maxSpeed << ","
             << p.attributes.acceleration << ","
             << p.attributes.passSkill << ","
             << p.attributes.shotPower << "\n";
    }

    file.close();
    std::cout << "[TrackingSystem] Successfully exported: " << playersPath << " (" << players.size() << " players)\n";
    return true;
}

bool TrackingSystem::exportMatchCSV(int homeScore, int awayScore, float matchDurationSeconds, const std::string& matchPath) {
    std::ofstream file(matchPath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[TrackingSystem] Error: Could not write to " << matchPath << "\n";
        return false;
    }

    int minutes = static_cast<int>(matchDurationSeconds) / 60;
    int seconds = static_cast<int>(matchDurationSeconds) % 60;

    file << "match_id,home_team,away_team,home_score,away_score,duration_seconds,duration_formatted,tracking_samples_count\n";
    file << "MATCH_001,Home (Red),Away (Blue),"
         << homeScore << "," << awayScore << ","
         << std::fixed << std::setprecision(1) << matchDurationSeconds << ","
         << minutes << "m " << seconds << "s,"
         << m_sampleCount << "\n";

    file.close();
    std::cout << "[TrackingSystem] Successfully exported: " << matchPath << "\n";
    return true;
}

std::string TrackingSystem::roleToString(TacticalRole role) const {
    switch (role) {
        case TacticalRole::GK:       return "GK";
        case TacticalRole::LB:       return "LB";
        case TacticalRole::CB_LEFT:  return "CB_L";
        case TacticalRole::CB_RIGHT: return "CB_R";
        case TacticalRole::RB:       return "RB";
        case TacticalRole::DM:       return "DM";
        case TacticalRole::CM_LEFT:  return "CM_L";
        case TacticalRole::CM_RIGHT: return "CM_R";
        case TacticalRole::LW:       return "LW";
        case TacticalRole::ST:       return "ST";
        case TacticalRole::RW:       return "RW";
        default:                     return "UNKNOWN";
    }
}

std::string TrackingSystem::stateToString(PlayerState state) const {
    switch (state) {
        case PlayerState::HoldingFormation: return "HoldingFormation";
        case PlayerState::SupportingAttack: return "SupportingAttack";
        case PlayerState::Marking:          return "Marking";
        case PlayerState::PressingBall:     return "PressingBall";
        case PlayerState::Dribbling:        return "Dribbling";
        case PlayerState::PreparingPass:    return "PreparingPass";
        case PlayerState::PreparingShot:    return "PreparingShot";
        case PlayerState::Recovering:       return "Recovering";
        default:                            return "Idle";
    }
}
