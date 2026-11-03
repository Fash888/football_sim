#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics/Color.hpp>
#include "Config.hpp"
#include "Entities.hpp"
#include "Tactics.hpp"

struct RealTeamPlayer {
    std::string name;
    int shirtNumber;
    TacticalRole role;
    FCIQRole fcIqRole;
    int overallRating{80};
    PlayerAttributes attributes;
};

struct TeamProfile {
    int id{0};
    std::string name;
    std::string shortName;
    std::string league;
    std::string manager;
    std::string styleDescription;

    sf::Color primaryColor{218, 41, 28};
    sf::Color secondaryColor{255, 255, 255};
    sf::Color gkColor{243, 156, 18};

    FormationType defaultFormation{FormationType::F_4_3_3};
    TeamTacticalStyle tacticalStyle{TeamTacticalStyle::TikiTaka};
    DefensiveMentality defensiveMentality{DefensiveMentality::MidBlock};

    std::vector<std::string> strengths;
    std::vector<std::string> weaknesses;

    // In-Engine Tactical & Matchup Modifiers
    float passSkillMultiplier{1.0f};
    float pressIntensityMultiplier{1.0f};
    float counterSpeedMultiplier{1.0f};
    float defensiveTackleMultiplier{1.0f};
    float setPieceBonus{1.0f};
    float offsideLineOffsetM{0.0f};
    float widthMultiplier{1.0f};
    float staminaDrainMultiplier{1.0f};

    std::vector<RealTeamPlayer> squad; // Starting XI
};

class TeamDatabase {
public:
    static const std::vector<TeamProfile>& getAllTeams();
    static const TeamProfile& getTeam(int index);
    static int getTeamCount();
    static int findTeamIndex(const std::string& shortNameOrName);
};
