#pragma once
#include <string>
#include <vector>
#include "Config.hpp"
#include "Entities.hpp"

enum class FormationType {
    F_4_3_3,
    F_4_4_2,
    F_4_2_3_1,
    F_3_4_2_1
};

struct RoleSlot {
    TacticalRole role;
    FCIQRole fcIqRole;
    int shirtNumber;
    std::string name;
    Vec2 homePitchPos;
    PlayerAttributes defaultAttributes;
};

class Tactics {
public:
    static std::vector<RoleSlot> getFormationSlots(FormationType type);
    static std::string formationToString(FormationType type);
    static std::string fcIqRoleToString(FCIQRole role);
    static std::string tacticalStyleToString(TeamTacticalStyle style);
    static std::string defensiveMentalityToString(DefensiveMentality mentality);
    static std::string behaviorStateToString(BehaviorState state);

    static Vec2 orientForTeam(const Vec2& homePos, TeamSide team);

    // Default tactical bundle per style & mentality
    static TeamTactics getDefaultTacticsForStyle(TeamTacticalStyle style, DefensiveMentality mentality);

    // Dynamic in-match tactical adaptation based on scoreline, match minute, and momentum
    static void adaptTacticsDynamically(
        TeamSide team,
        int homeScore,
        int awayScore,
        float matchTime,
        TeamTacticalStyle& style,
        DefensiveMentality& mentality
    );

    // --- THREE-LAYER MOVEMENT SYSTEM ---

    // Layer 1: Team Shape - Dynamic formation anchor with ball pull, block height, width, and phase shapes
    static Vec2 calculateDynamicAnchor(
        const Player& player,
        const Vec2& ballPos,
        TeamSide possessionTeam,
        FormationType formation,
        const TeamTactics& tactics,
        float offsideLineX
    );

    // Layer 2: Role Behaviour Evaluation - Re-evaluated every 0.25 - 0.5s per player
    static BehaviorState evaluateRoleBehavior(
        const Player& player,
        const Vec2& ballPos,
        TeamSide possessionTeam,
        int activePossessorId,
        const TeamTactics& tactics,
        float counterPressTimer,
        int presserCount
    );

    // Layer 2: Role Behaviour Target Position (inside-channel, overlap, decoy, pivot shield, rest-defence, etc.)
    static Vec2 calculateBehaviorTarget(
        const Player& player,
        BehaviorState behavior,
        const Vec2& ballPos,
        TeamSide possessionTeam,
        int activePossessorId,
        const std::vector<Player>& allPlayers,
        const TeamTactics& tactics,
        float offsideLineX
    );

    // High-fidelity off-the-ball positioning: blends Layer 1 & Layer 2 with spatial dismarking & lane screening
    static Vec2 calculateOffTheBallPosition(
        const Player& player,
        const Vec2& ballPos,
        TeamSide possessionTeam,
        int activePossessorId,
        TeamTacticalStyle tacticalStyle = TeamTacticalStyle::TikiTaka,
        DefensiveMentality mentality = DefensiveMentality::MidBlock,
        FormationType formation = FormationType::F_4_3_3,
        const std::vector<Player>* allPlayers = nullptr,
        float offsideLineX = 52.5f
    );
};
