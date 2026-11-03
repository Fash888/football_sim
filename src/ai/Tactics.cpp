#include "Tactics.hpp"
#include <algorithm>
#include <cmath>

std::string Tactics::formationToString(FormationType type) {
    switch (type) {
        case FormationType::F_4_3_3:   return "4-3-3";
        case FormationType::F_4_4_2:   return "4-4-2";
        case FormationType::F_4_2_3_1: return "4-2-3-1";
        case FormationType::F_3_4_2_1: return "3-4-2-1";
        default:                       return "Custom";
    }
}

std::string Tactics::behaviorStateToString(BehaviorState state) {
    switch (state) {
        case BehaviorState::SupportTheCarrier:    return "Support Carrier";
        case BehaviorState::InsideChannelRun:     return "Inside-Channel Run";
        case BehaviorState::OverlapUnderlap:      return "Overlap / Underlap";
        case BehaviorState::DecoyRun:             return "Decoy Run";
        case BehaviorState::OverloadAndSwitch:    return "Overload & Switch";
        case BehaviorState::RestDefence:          return "Rest Defence";
        case BehaviorState::Press:                return "Pressing Ball";
        case BehaviorState::CounterPress:         return "Counter-Press";
        case BehaviorState::Pivot:                return "Pivot Shield";
        case BehaviorState::DefensivePositioning: return "Shape Cover";
        default:                                  return "Positioning";
    }
}

std::string Tactics::fcIqRoleToString(FCIQRole role) {
    switch (role) {
        case FCIQRole::TraditionalGK:        return "Goalkeeper (Stopper)";
        case FCIQRole::SweeperKeeper:        return "Sweeper Keeper";
        case FCIQRole::BallPlayingDefender:  return "Ball-Playing Defender";
        case FCIQRole::Stopper:              return "Stopper (No-Nonsense)";
        case FCIQRole::AttackingWingback:    return "Attacking Wingback";
        case FCIQRole::InvertedFullback:     return "Inverted Fullback";
        case FCIQRole::DeepLyingPlaymaker:   return "Deep-Lying Playmaker";
        case FCIQRole::BoxToBox:             return "Box-To-Box Engine";
        case FCIQRole::PlaymakerCAM:         return "Advanced Playmaker";
        case FCIQRole::AnchorCDM:            return "Anchor / Destroyer";
        case FCIQRole::InsideForward:        return "Inside Forward";
        case FCIQRole::ClassicWinger:        return "Classic Winger";
        case FCIQRole::PoacherST:            return "Poacher / Clinical ST";
        case FCIQRole::TargetForward:        return "Target Forward";
        case FCIQRole::FalseNine:            return "False 9 Playmaker";
        default:                             return "Balanced Role";
    }
}

std::string Tactics::tacticalStyleToString(TeamTacticalStyle style) {
    switch (style) {
        case TeamTacticalStyle::TikiTaka:      return "Tiki-Taka (Guardiola)";
        case TeamTacticalStyle::Gegenpressing: return "Gegenpress (High Tempo)";
        case TeamTacticalStyle::CounterAttack: return "Counter-Attack (Direct)";
        case TeamTacticalStyle::WingPlay:      return "Wing Play (Overloads)";
        default:                               return "Balanced";
    }
}

std::string Tactics::defensiveMentalityToString(DefensiveMentality mentality) {
    switch (mentality) {
        case DefensiveMentality::HighPress: return "High Press";
        case DefensiveMentality::MidBlock:  return "Mid Block";
        case DefensiveMentality::LowBlock:  return "Low Block";
        default:                            return "Balanced";
    }
}

std::vector<RoleSlot> Tactics::getFormationSlots(FormationType type) {
    std::vector<RoleSlot> slots;
    slots.reserve(11);

    // Differentiated real-world archetypes: Pace, Shooting, Passing, Dribbling, Defending, Physical
    PlayerAttributes gkAttrs{
        7.2f, 6.8f, 0.80f, 22.0f, 0.60f,
        62, 18, 75, 52, 55, 76,
        0.20f, 0.72f, 0.40f, 0.84f, 0.65f,
        0.88f, 0.87f, 0.82f, 0.86f // Reflexes, Diving, Handling, Positioning
    };

    PlayerAttributes cbLeftAttrs{ // Ball-playing defender (e.g., Van Dijk / Saliba)
        7.9f, 7.3f, 0.87f, 23.0f, 0.90f,
        76, 42, 84, 72, 90, 86,
        0.35f, 0.84f, 0.55f, 0.88f, 0.92f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes cbRightAttrs{ // Aggressive Stopper (e.g., Rudiger / Dias)
        7.6f, 7.0f, 0.76f, 24.0f, 0.93f,
        71, 38, 72, 64, 93, 91,
        0.30f, 0.68f, 0.45f, 0.82f, 0.95f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes lbAttrs{ // Attacking Wingback (e.g., Robertson / Davies)
        8.8f, 8.4f, 0.85f, 24.0f, 0.78f,
        89, 66, 83, 82, 78, 80,
        0.65f, 0.80f, 0.87f, 0.80f, 0.80f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes rbAttrs{ // Inverted Fullback (e.g., Walker / White)
        8.4f, 8.0f, 0.84f, 24.0f, 0.84f,
        84, 60, 83, 80, 84, 82,
        0.55f, 0.82f, 0.78f, 0.84f, 0.85f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes dmAttrs{ // Deep-Lying Playmaker (e.g., Rodri / Pirlo)
        7.8f, 7.4f, 0.95f, 26.0f, 0.86f,
        74, 75, 93, 84, 85, 84,
        0.72f, 0.94f, 0.82f, 0.93f, 0.88f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes cmRightAttrs{ // Box-to-Box Engine (e.g., Valverde / Bellingham)
        8.5f, 8.1f, 0.88f, 26.5f, 0.82f,
        85, 82, 85, 84, 80, 87,
        0.78f, 0.85f, 0.80f, 0.88f, 0.82f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes cmLeftAttrs{ // Advanced Playmaker CAM (e.g., De Bruyne / Odegaard)
        8.1f, 7.7f, 0.94f, 27.5f, 0.65f,
        80, 85, 93, 88, 64, 72,
        0.82f, 0.95f, 0.88f, 0.92f, 0.68f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes lwAttrs{ // Inside Forward (e.g., Vinicius / Son)
        9.0f, 8.7f, 0.84f, 28.5f, 0.50f,
        92, 86, 81, 90, 46, 74,
        0.86f, 0.82f, 0.78f, 0.86f, 0.52f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes rwAttrs{ // Classic Winger (e.g., Saka / Salah)
        9.1f, 8.8f, 0.85f, 28.0f, 0.52f,
        93, 85, 83, 91, 48, 75,
        0.85f, 0.84f, 0.88f, 0.88f, 0.55f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    PlayerAttributes stAttrs{ // Clinical Poacher (e.g., Haaland / Mbappe)
        8.8f, 8.6f, 0.78f, 29.5f, 0.40f,
        89, 93, 76, 85, 40, 88,
        0.94f, 0.76f, 0.65f, 0.90f, 0.45f,
        0.50f, 0.50f, 0.50f, 0.50f
    };

    slots.push_back({TacticalRole::GK, FCIQRole::SweeperKeeper, 1, "Goalkeeper", {5.0f, 34.0f}, gkAttrs});

    switch (type) {
        case FormationType::F_4_3_3:
            slots.push_back({TacticalRole::RB,       FCIQRole::InvertedFullback,    2,  "Right Back",           {24.0f, 56.0f}, rbAttrs});
            slots.push_back({TacticalRole::CB_RIGHT, FCIQRole::Stopper,             4,  "Center Back Right",    {19.0f, 42.0f}, cbRightAttrs});
            slots.push_back({TacticalRole::CB_LEFT,  FCIQRole::BallPlayingDefender, 5,  "Center Back Left",     {19.0f, 26.0f}, cbLeftAttrs});
            slots.push_back({TacticalRole::LB,       FCIQRole::AttackingWingback,   3,  "Left Back",            {24.0f, 12.0f}, lbAttrs});
            slots.push_back({TacticalRole::DM,       FCIQRole::DeepLyingPlaymaker,  6,  "Defensive Midfielder", {34.0f, 34.0f}, dmAttrs});
            slots.push_back({TacticalRole::CM_RIGHT, FCIQRole::BoxToBox,            8,  "Central Midfielder",   {44.0f, 46.0f}, cmRightAttrs});
            slots.push_back({TacticalRole::CM_LEFT,  FCIQRole::PlaymakerCAM,        10, "Central Midfielder",   {44.0f, 22.0f}, cmLeftAttrs});
            slots.push_back({TacticalRole::RW,       FCIQRole::ClassicWinger,       7,  "Right Winger",         {56.0f, 55.0f}, rwAttrs});
            slots.push_back({TacticalRole::ST,       FCIQRole::PoacherST,           9,  "Striker",              {58.0f, 34.0f}, stAttrs});
            slots.push_back({TacticalRole::LW,       FCIQRole::InsideForward,       11, "Left Winger",          {56.0f, 13.0f}, lwAttrs});
            break;

        case FormationType::F_4_4_2:
            slots.push_back({TacticalRole::RB,       FCIQRole::AttackingWingback,   2,  "Right Back",           {24.0f, 56.0f}, rbAttrs});
            slots.push_back({TacticalRole::CB_RIGHT, FCIQRole::Stopper,             4,  "Center Back Right",    {19.0f, 42.0f}, cbRightAttrs});
            slots.push_back({TacticalRole::CB_LEFT,  FCIQRole::BallPlayingDefender, 5,  "Center Back Left",     {19.0f, 26.0f}, cbLeftAttrs});
            slots.push_back({TacticalRole::LB,       FCIQRole::AttackingWingback,   3,  "Left Back",            {24.0f, 12.0f}, lbAttrs});
            slots.push_back({TacticalRole::RW,       FCIQRole::ClassicWinger,       7,  "Right Midfielder",     {42.0f, 56.0f}, rwAttrs});
            slots.push_back({TacticalRole::CM_RIGHT, FCIQRole::BoxToBox,            8,  "Central Midfielder",   {40.0f, 40.0f}, cmRightAttrs});
            slots.push_back({TacticalRole::CM_LEFT,  FCIQRole::DeepLyingPlaymaker,  6,  "Central Midfielder",   {40.0f, 28.0f}, dmAttrs});
            slots.push_back({TacticalRole::LW,       FCIQRole::InsideForward,       11, "Left Midfielder",      {42.0f, 12.0f}, lwAttrs});
            slots.push_back({TacticalRole::ST,       FCIQRole::TargetForward,       9,  "Striker Left",         {56.0f, 27.0f}, stAttrs});
            slots.push_back({TacticalRole::ST,       FCIQRole::PoacherST,           10, "Striker Right",        {56.0f, 41.0f}, stAttrs});
            break;

        case FormationType::F_4_2_3_1:
            slots.push_back({TacticalRole::RB,       FCIQRole::InvertedFullback,    2,  "Right Back",           {24.0f, 56.0f}, rbAttrs});
            slots.push_back({TacticalRole::CB_RIGHT, FCIQRole::Stopper,             4,  "Center Back Right",    {19.0f, 42.0f}, cbRightAttrs});
            slots.push_back({TacticalRole::CB_LEFT,  FCIQRole::BallPlayingDefender, 5,  "Center Back Left",     {19.0f, 26.0f}, cbLeftAttrs});
            slots.push_back({TacticalRole::LB,       FCIQRole::AttackingWingback,   3,  "Left Back",            {24.0f, 12.0f}, lbAttrs});
            slots.push_back({TacticalRole::DM,       FCIQRole::AnchorCDM,           6,  "Pivot Midfielder",     {34.0f, 41.0f}, dmAttrs});
            slots.push_back({TacticalRole::DM,       FCIQRole::DeepLyingPlaymaker,  8,  "Pivot Midfielder",     {34.0f, 27.0f}, dmAttrs});
            slots.push_back({TacticalRole::RW,       FCIQRole::ClassicWinger,       7,  "Right Winger",         {52.0f, 56.0f}, rwAttrs});
            slots.push_back({TacticalRole::CM_LEFT,  FCIQRole::PlaymakerCAM,        10, "Central CAM",          {50.0f, 34.0f}, cmLeftAttrs});
            slots.push_back({TacticalRole::LW,       FCIQRole::InsideForward,       11, "Left Winger",          {52.0f, 12.0f}, lwAttrs});
            slots.push_back({TacticalRole::ST,       FCIQRole::PoacherST,           9,  "Target Striker",       {58.0f, 34.0f}, stAttrs});
            break;

        case FormationType::F_3_4_2_1:
            // 3-Centerback Foundation
            slots.push_back({TacticalRole::CB_RIGHT, FCIQRole::Stopper,             4,  "Center Back Right",    {18.0f, 48.0f}, cbRightAttrs});
            slots.push_back({TacticalRole::CB_LEFT,  FCIQRole::BallPlayingDefender, 5,  "Center Back Middle",   {16.0f, 34.0f}, cbLeftAttrs});
            slots.push_back({TacticalRole::CB_LEFT,  FCIQRole::Stopper,             6,  "Center Back Left",     {18.0f, 20.0f}, cbRightAttrs});
            // Dynamic Wingbacks providing width
            slots.push_back({TacticalRole::RB,       FCIQRole::AttackingWingback,   2,  "Right Wingback",       {32.0f, 60.0f}, rbAttrs});
            slots.push_back({TacticalRole::LB,       FCIQRole::AttackingWingback,   3,  "Left Wingback",        {32.0f, 8.0f},  lbAttrs});
            // Double Pivot in midfield
            slots.push_back({TacticalRole::DM,       FCIQRole::AnchorCDM,           8,  "Central Midfielder R", {38.0f, 42.0f}, dmAttrs});
            slots.push_back({TacticalRole::CM_LEFT,  FCIQRole::BoxToBox,            10, "Central Midfielder L", {38.0f, 26.0f}, cmRightAttrs});
            // Dual Attacking Playmakers in the half-spaces
            slots.push_back({TacticalRole::RW,       FCIQRole::InsideForward,       7,  "Right Inside Forward", {50.0f, 46.0f}, rwAttrs});
            slots.push_back({TacticalRole::LW,       FCIQRole::PlaymakerCAM,        11, "Left Inside Forward",  {50.0f, 22.0f}, cmLeftAttrs});
            // Advanced Poacher
            slots.push_back({TacticalRole::ST,       FCIQRole::PoacherST,           9,  "Striker",              {58.0f, 34.0f}, stAttrs});
            break;
    }

    return slots;
}

Vec2 Tactics::orientForTeam(const Vec2& homePos, TeamSide team) {
    if (team == TeamSide::Home) {
        return homePos;
    }
    return Vec2(SimConfig::PITCH_LENGTH_M - homePos.x, SimConfig::PITCH_WIDTH_M - homePos.y);
}

void Tactics::adaptTacticsDynamically(
    TeamSide team,
    int homeScore,
    int awayScore,
    float matchTime,
    TeamTacticalStyle& style,
    DefensiveMentality& mentality
) {
    int goalDiff = (team == TeamSide::Home) ? (homeScore - awayScore) : (awayScore - homeScore);
    float matchMinutes = matchTime / 60.0f;

    // 1. Chasing a game late (> 55 minutes, losing)
    if (goalDiff < 0 && matchMinutes >= 55.0f) {
        style = TeamTacticalStyle::Gegenpressing;
        mentality = DefensiveMentality::HighPress; // Aggressive all-out press
        return;
    }

    // 2. Protecting a lead late (> 65 minutes, winning)
    if (goalDiff > 0 && matchMinutes >= 65.0f) {
        style = TeamTacticalStyle::CounterAttack;
        mentality = (goalDiff >= 2) ? DefensiveMentality::LowBlock : DefensiveMentality::MidBlock;
        return;
    }

    // 3. Match tied or early game: Team tactical DNA
    if (team == TeamSide::Home) {
        style = TeamTacticalStyle::TikiTaka; // Home side dominates possession
        mentality = DefensiveMentality::MidBlock;
    } else {
        style = TeamTacticalStyle::CounterAttack; // Away side lethal on the break
        mentality = DefensiveMentality::MidBlock;
    }
}

TeamTactics Tactics::getDefaultTacticsForStyle(TeamTacticalStyle style, DefensiveMentality mentality) {
    TeamTactics t;
    if (mentality == DefensiveMentality::HighPress) {
        t.outOfPossession.blockHeight = BlockHeight::High;
        t.outOfPossession.compactness = 30.0f;
    } else if (mentality == DefensiveMentality::LowBlock) {
        t.outOfPossession.blockHeight = BlockHeight::Low;
        t.outOfPossession.compactness = 24.0f;
    } else {
        t.outOfPossession.blockHeight = BlockHeight::Mid;
        t.outOfPossession.compactness = 32.0f;
    }

    switch (style) {
        case TeamTacticalStyle::TikiTaka:
            t.inPossession.buildUpShape = BuildUpShape::Back3PivotDrops;
            t.inPossession.invertedFullbacks = true;
            t.inPossession.directness = Directness::ShortPossession;
            t.inPossession.wideRotation = WideRotation::WingerInsideFBOverlaps;
            t.inPossession.width = 62.0f;
            t.inPossession.insideChannelRunFreq = 0.75f;
            t.inPossession.overloadTempo = 0.70f;
            t.outOfPossession.counterPressIntensity = CounterPressIntensity::Heavy;
            t.outOfPossession.restDefence = RestDefenceShape::Rest3Plus2;
            break;

        case TeamTacticalStyle::Gegenpressing:
            t.inPossession.buildUpShape = BuildUpShape::GKInvolved;
            t.inPossession.directness = Directness::Mixed;
            t.inPossession.transitionStyle = TransitionStyle::ImmediateForwardPass;
            t.inPossession.insideChannelRunFreq = 0.85f;
            t.outOfPossession.pressTrigger = PressTrigger::AnyLoose;
            t.outOfPossession.counterPressIntensity = CounterPressIntensity::Heavy;
            t.outOfPossession.blockHeight = BlockHeight::High;
            t.outOfPossession.restDefence = RestDefenceShape::Rest2Plus3;
            break;

        case TeamTacticalStyle::CounterAttack:
            t.inPossession.buildUpShape = BuildUpShape::Back4;
            t.inPossession.invertedFullbacks = false;
            t.inPossession.directness = Directness::DirectLongBall;
            t.inPossession.transitionStyle = TransitionStyle::ImmediateForwardPass;
            t.inPossession.insideChannelRunFreq = 0.90f;
            t.outOfPossession.blockHeight = (mentality == DefensiveMentality::HighPress) ? BlockHeight::Mid : BlockHeight::Low;
            t.outOfPossession.counterPressIntensity = CounterPressIntensity::Balanced;
            t.outOfPossession.restDefence = RestDefenceShape::Rest4Back;
            t.outOfPossession.forcePlay = ForcePlay::ForceWide;
            break;

        case TeamTacticalStyle::WingPlay:
            t.inPossession.buildUpShape = BuildUpShape::Back4;
            t.inPossession.invertedFullbacks = false;
            t.inPossession.wideRotation = WideRotation::WingerInsideFBOverlaps;
            t.inPossession.crossingStyle = CrossingStyle::Inswinging;
            t.inPossession.width = 66.0f;
            t.inPossession.insideChannelRunFreq = 0.60f;
            t.inPossession.overloadTempo = 0.80f;
            t.outOfPossession.restDefence = RestDefenceShape::Rest3Plus2;
            break;
    }
    return t;
}

// ─────────────────────────────────────────────────────────────────────────────
// LAYER 1: TEAM SHAPE (Sliding Unit Formation Anchor)
// ─────────────────────────────────────────────────────────────────────────────
Vec2 Tactics::calculateDynamicAnchor(
    const Player& player,
    const Vec2& ballPos,
    TeamSide possessionTeam,
    FormationType formation,
    const TeamTactics& tactics,
    float /*offsideLineX*/
) {
    bool hasPossession = (player.team == possessionTeam);
    float pitchCenterX = SimConfig::PITCH_LENGTH_M * 0.5f;
    float pitchCenterY = SimConfig::PITCH_WIDTH_M * 0.5f;
    float attackingDir = (player.team == TeamSide::Home) ? 1.0f : -1.0f;
    float ownGoalX = (player.team == TeamSide::Home) ? 0.0f : SimConfig::PITCH_LENGTH_M;

    // 1. Ball Pull: Subtle unit shift (20% longitudinal, 15% lateral) preserving natural formation shape
    float ballOffsetX = ballPos.x - pitchCenterX;
    float ballOffsetY = ballPos.y - pitchCenterY;
    float ballPullX = ballOffsetX * 0.20f;
    float ballPullY = ballOffsetY * 0.15f;

    // 2. Block Height Calibration (30m Low, 45m Mid, 60m High from own goal)
    float baseDefensiveLineDist = 45.0f;
    if (tactics.outOfPossession.blockHeight == BlockHeight::Low) {
        baseDefensiveLineDist = 30.0f;
    } else if (tactics.outOfPossession.blockHeight == BlockHeight::High) {
        baseDefensiveLineDist = 60.0f;
    }

    float blockShiftX = (baseDefensiveLineDist - 45.0f) * attackingDir;
    if (hasPossession) {
        blockShiftX += attackingDir * 14.0f; // Dynamic progressive push forward in possession
    } else {
        blockShiftX -= attackingDir * 6.0f; // Drop and compress out of possession
    }

    // 3. Compactness and Width scaling
    // In defense: narrow unit to 36-40m. In attack: stretch to 58-66m.
    float widthRatio = hasPossession ? (tactics.inPossession.width / SimConfig::PITCH_WIDTH_M) : 0.56f;

    Vec2 basePos = player.homePosition;
    float anchorX = basePos.x + ballPullX + blockShiftX;
    float anchorY = pitchCenterY + ((basePos.y - pitchCenterY) * widthRatio) + ballPullY;

    // Far-side tucks in: if ball is on one side, players on the opposite flank shift inwards
    if (std::abs(ballOffsetY) > 8.0f) {
        bool ballIsUpper = (ballOffsetY < 0.0f);
        bool playerIsOpposite = (ballIsUpper && (basePos.y > pitchCenterY)) || (!ballIsUpper && (basePos.y < pitchCenterY));
        if (playerIsOpposite && !hasPossession) {
            anchorY += (pitchCenterY - anchorY) * 0.32f; // Far side tucks in
        }
    }

    // 4. In-Possession Phase Structural Shapes (e.g. 3-2-5 or 2-3-5)
    if (hasPossession) {
        // Build-up shape with dropping pivot between CBs
        if (tactics.inPossession.buildUpShape == BuildUpShape::Back3PivotDrops &&
            (player.role == TacticalRole::DM || player.fcIqRole == FCIQRole::DeepLyingPlaymaker || player.fcIqRole == FCIQRole::AnchorCDM)) {
            anchorX -= attackingDir * 6.5f; // Drops deep between split CBs
            anchorY = pitchCenterY;
        }

        // Inverted Fullbacks tuck inside into the half-spaces
        if (tactics.inPossession.invertedFullbacks) {
            if (player.role == TacticalRole::LB) {
                anchorY = std::clamp(anchorY, 22.0f, 32.0f);
                anchorX += attackingDir * 4.0f;
            } else if (player.role == TacticalRole::RB) {
                anchorY = std::clamp(anchorY, 36.0f, 46.0f);
                anchorX += attackingDir * 4.0f;
            }
        }

        // Formation-specific structure adjustments
        if (formation == FormationType::F_4_3_3) {
            if (player.role == TacticalRole::DM) {
                anchorY = std::clamp(anchorY, 30.0f, 38.0f);
            }
        } else if (formation == FormationType::F_4_4_2) {
            if (player.role == TacticalRole::CM_LEFT) {
                anchorY = std::clamp(anchorY, 24.0f, 32.0f);
            } else if (player.role == TacticalRole::CM_RIGHT) {
                anchorY = std::clamp(anchorY, 36.0f, 44.0f);
            } else if (player.role == TacticalRole::ST) {
                // In 4-4-2, split the two strikers
                if (player.shirtNumber == 9) anchorX -= attackingDir * 2.5f;
                else anchorX += attackingDir * 2.5f;
            }
        } else if (formation == FormationType::F_4_2_3_1) {
            if (player.role == TacticalRole::DM) {
                if (player.shirtNumber == 6) anchorY = std::clamp(anchorY, 36.0f, 44.0f);
                else anchorY = std::clamp(anchorY, 24.0f, 32.0f);
            } else if (player.fcIqRole == FCIQRole::PlaymakerCAM) {
                anchorY = std::clamp(anchorY, 28.0f, 40.0f);
                anchorX += attackingDir * 3.5f;
            }
        } else if (formation == FormationType::F_3_4_2_1) {
            if (player.role == TacticalRole::LB || player.role == TacticalRole::RB) {
                anchorX += attackingDir * 6.0f;
                anchorY = (player.role == TacticalRole::LB) ? 8.0f : 60.0f;
            } else if (player.role == TacticalRole::LW || player.role == TacticalRole::RW) {
                anchorY = (player.role == TacticalRole::LW) ? 22.0f : 46.0f;
            }
        }
    }

    // Strikers and Wingers maintain attacking outlet presence and lead the attacking line
    if (player.role == TacticalRole::ST || player.fcIqRole == FCIQRole::PoacherST || player.fcIqRole == FCIQRole::TargetForward) {
        if (hasPossession) {
            if (player.team == TeamSide::Home) {
                anchorX = std::max(anchorX, 68.0f);
            } else {
                anchorX = std::min(anchorX, 37.0f);
            }
        } else {
            if (player.team == TeamSide::Home) {
                anchorX = std::max(anchorX, 42.0f);
            } else {
                anchorX = std::min(anchorX, 63.0f);
            }
        }
    } else if (player.role == TacticalRole::LW || player.role == TacticalRole::RW) {
        if (hasPossession) {
            if (player.team == TeamSide::Home) {
                anchorX = std::max(anchorX, 60.0f);
            } else {
                anchorX = std::min(anchorX, 45.0f);
            }
        } else {
            if (player.team == TeamSide::Home) {
                anchorX = std::max(anchorX, 36.0f);
            } else {
                anchorX = std::min(anchorX, 69.0f);
            }
        }
    }

    // Clamp inside boundaries with safety margin
    anchorX = std::clamp(anchorX, 5.0f, SimConfig::PITCH_LENGTH_M - 5.0f);
    anchorY = std::clamp(anchorY, 4.0f, SimConfig::PITCH_WIDTH_M - 4.0f);

    return Vec2(anchorX, anchorY);
}

// ─────────────────────────────────────────────────────────────────────────────
// LAYER 2: ROLE BEHAVIOURS EVALUATION (Every 0.25 - 0.5s)
// ─────────────────────────────────────────────────────────────────────────────
BehaviorState Tactics::evaluateRoleBehavior(
    const Player& player,
    const Vec2& ballPos,
    TeamSide possessionTeam,
    int activePossessorId,
    const TeamTactics& tactics,
    float counterPressTimer,
    int presserCount
) {
    if (player.role == TacticalRole::GK) {
        return BehaviorState::DefensivePositioning;
    }

    bool hasPossession = (player.team == possessionTeam);
    float attackingDir = (player.team == TeamSide::Home) ? 1.0f : -1.0f;
    float distToBall = (player.position - ballPos).length();

    // Loose ball: If no player currently has possession, nearby players contest and recover the ball
    if (activePossessorId == -1 && distToBall < 16.0f) {
        return BehaviorState::Press;
    }

    // Out of Possession
    if (!hasPossession) {
        // Counter-press trigger: immediate 3-5 seconds after ball loss
        if (counterPressTimer > 0.0f && tactics.outOfPossession.counterPressIntensity != CounterPressIntensity::Off) {
            if (distToBall < 18.0f && presserCount < 3) {
                return BehaviorState::CounterPress;
            }
        }

        // Nearest 2-3 players press; others cover lanes and hold shape
        if (distToBall < 14.0f && presserCount < 2) {
            return BehaviorState::Press;
        }

        // Single holding pivot shields space in front of CBs
        if (player.role == TacticalRole::DM || player.fcIqRole == FCIQRole::AnchorCDM) {
            return BehaviorState::Pivot;
        }

        return BehaviorState::DefensivePositioning;
    }

    // In Possession
    // Ball Carrier handles on-ball decision elsewhere
    if (player.id == activePossessorId) {
        return BehaviorState::SupportTheCarrier;
    }

    // Rest Defence: 3+2 or 2+3 structure staying behind ball
    float ballAdvancement = (ballPos.x - 52.5f) * attackingDir;
    if (ballAdvancement > 0.0f) {
        if (player.role == TacticalRole::CB_LEFT || player.role == TacticalRole::CB_RIGHT) {
            return BehaviorState::RestDefence;
        }
        if (tactics.outOfPossession.restDefence == RestDefenceShape::Rest3Plus2) {
            if (player.role == TacticalRole::DM || (player.role == TacticalRole::RB && tactics.inPossession.invertedFullbacks)) {
                return BehaviorState::RestDefence;
            }
        }
    }

    // Inside-Channel Run: Strikers, Inside Forwards, and CAMs attacking the seam
    bool inAttackingHalf = (player.team == TeamSide::Home && ballPos.x > 50.0f) || (player.team == TeamSide::Away && ballPos.x < 55.0f);
    if (inAttackingHalf) {
        if (player.role == TacticalRole::ST || player.fcIqRole == FCIQRole::PoacherST || player.fcIqRole == FCIQRole::InsideForward) {
            return BehaviorState::InsideChannelRun;
        }
        if (player.fcIqRole == FCIQRole::PlaymakerCAM && tactics.inPossession.insideChannelRunFreq > 0.6f) {
            return BehaviorState::InsideChannelRun;
        }
    }

    // Decoy Run ("rest-attack" run to drag defenders away)
    if (inAttackingHalf && (player.fcIqRole == FCIQRole::TargetForward || player.role == TacticalRole::LW || player.role == TacticalRole::RW)) {
        float lateralDistFromBall = std::abs(player.position.y - ballPos.y);
        if (lateralDistFromBall > 24.0f) {
            return BehaviorState::OverloadAndSwitch; // Isolated on far touchline
        }
        return BehaviorState::DecoyRun;
    }

    // Overlap / Underlap: Fullbacks
    if (player.role == TacticalRole::LB || player.role == TacticalRole::RB) {
        if (!tactics.inPossession.invertedFullbacks) {
            return BehaviorState::OverlapUnderlap;
        }
    }

    // Midfield pivot support
    if (player.role == TacticalRole::DM || player.fcIqRole == FCIQRole::DeepLyingPlaymaker) {
        return BehaviorState::Pivot;
    }

    return BehaviorState::SupportTheCarrier;
}

// ─────────────────────────────────────────────────────────────────────────────
// LAYER 2: ROLE BEHAVIOUR TARGET POSITION
// ─────────────────────────────────────────────────────────────────────────────
Vec2 Tactics::calculateBehaviorTarget(
    const Player& player,
    BehaviorState behavior,
    const Vec2& ballPos,
    TeamSide possessionTeam,
    int activePossessorId,
    const std::vector<Player>& allPlayers,
    const TeamTactics& tactics,
    float offsideLineX
) {
    Vec2 anchor = player.dynamicAnchor;
    float attackingDir = (player.team == TeamSide::Home) ? 1.0f : -1.0f;
    float pitchCenterY = SimConfig::PITCH_WIDTH_M * 0.5f;

    switch (behavior) {
        case BehaviorState::InsideChannelRun: {
            // Sprints into the seam between opponent CB and FB
            // Left inside channel: y ~ 20.0m; Right inside channel: y ~ 48.0m
            float channelY = (player.position.y < pitchCenterY) ? 21.0f : 47.0f;
            float targetX = anchor.x + (attackingDir * 14.0f);

            // Strictly clamp behind the offside line to avoid straying offside
            if (player.team == TeamSide::Home) {
                targetX = std::min(targetX, offsideLineX - 0.40f);
            } else {
                targetX = std::max(targetX, offsideLineX + 0.40f);
            }
            return Vec2(targetX, channelY);
        }

        case BehaviorState::OverlapUnderlap: {
            // Fullback drives down flank outside or inside winger
            float overlapY = (player.position.y < pitchCenterY) ? 7.0f : 61.0f;
            float targetX = anchor.x + (attackingDir * 11.0f);
            if (player.team == TeamSide::Home) {
                targetX = std::min(targetX, offsideLineX - 0.50f);
            } else {
                targetX = std::max(targetX, offsideLineX + 0.50f);
            }
            return Vec2(targetX, overlapY);
        }

        case BehaviorState::DecoyRun: {
            // Runs diagonally across the box dragging defenders away from the channel
            float crossY = (player.position.y < pitchCenterY) ? 38.0f : 30.0f;
            float targetX = anchor.x + (attackingDir * 9.0f);
            if (player.team == TeamSide::Home) {
                targetX = std::min(targetX, offsideLineX - 0.50f);
            } else {
                targetX = std::max(targetX, offsideLineX + 0.50f);
            }
            return Vec2(targetX, crossY);
        }

        case BehaviorState::OverloadAndSwitch: {
            // Isolated on far touchline awaiting lofted diagonal switch
            float isolatedY = (ballPos.y < pitchCenterY) ? 61.5f : 6.5f;
            float targetX = anchor.x + (attackingDir * 7.0f);
            if (player.team == TeamSide::Home) {
                targetX = std::min(targetX, offsideLineX - 0.50f);
            } else {
                targetX = std::max(targetX, offsideLineX + 0.50f);
            }
            return Vec2(targetX, isolatedY);
        }

        case BehaviorState::RestDefence: {
            // Holds behind ball, matched to opponent forwards to prevent counter-attack
            float safeX = (player.team == TeamSide::Home) ?
                std::min(anchor.x, ballPos.x - 14.0f) :
                std::max(anchor.x, ballPos.x + 14.0f);
            return Vec2(safeX, anchor.y);
        }

        case BehaviorState::Pivot: {
            // Holding midfielder shields space between ball and centerbacks
            float pivotX = (player.team == TeamSide::Home) ?
                ballPos.x - 12.0f : ballPos.x + 12.0f;
            float pivotY = std::clamp(ballPos.y, 24.0f, 44.0f);
            return Vec2(pivotX, pivotY);
        }

        case BehaviorState::Press:
        case BehaviorState::CounterPress: {
            // Curved approach closing down carrier while blocking central pass lane
            Vec2 toBall = ballPos - player.position;
            Vec2 curvedTarget = ballPos - (toBall.normalized() * 0.45f);
            return curvedTarget;
        }

        case BehaviorState::SupportTheCarrier: {
            // Maintain tactical formation anchor; only step back slightly if cluttering carrier space (< 7m)
            Vec2 toCarrier = player.position - ballPos;
            float dist = toCarrier.length();
            if (dist < 7.0f && dist > 0.1f) {
                return ballPos + (toCarrier.normalized() * 11.0f);
            }
            return anchor;
        }

        case BehaviorState::DefensivePositioning:
        default:
            return anchor;
    }
}

Vec2 Tactics::calculateOffTheBallPosition(
    const Player& player,
    const Vec2& ballPos,
    TeamSide possessionTeam,
    int activePossessorId,
    TeamTacticalStyle tacticalStyle,
    DefensiveMentality mentality,
    FormationType formation,
    const std::vector<Player>* allPlayers,
    float offsideLineX
) {
    TeamTactics tactics = getDefaultTacticsForStyle(tacticalStyle, mentality);
    bool hasPossession = (player.team == possessionTeam);
    float attackingDir = (player.team == TeamSide::Home) ? 1.0f : -1.0f;
    float ownGoalX = (player.team == TeamSide::Home) ? 0.0f : SimConfig::PITCH_LENGTH_M;

    // 1. Layer 1: Base dynamic formation anchor
    Vec2 target = calculateDynamicAnchor(player, ballPos, possessionTeam, formation, tactics, offsideLineX);

    // 2. Layer 2: Dynamic role runs (inside-channel, overlap, decoy)
    if (allPlayers != nullptr) {
        BehaviorState behavior = player.behaviorState;
        if (behavior == BehaviorState::InsideChannelRun || behavior == BehaviorState::OverlapUnderlap || behavior == BehaviorState::DecoyRun) {
            Vec2 behaviorTarget = calculateBehaviorTarget(player, behavior, ballPos, possessionTeam, activePossessorId, *allPlayers, tactics, offsideLineX);
            target = target * 0.65f + behaviorTarget * 0.35f;
        }
    }

    // 3. SPATIAL DYNAMICS: Creating Space (In Possession) vs Closing Space & Marking (Out of Possession)
    if (allPlayers != nullptr && !allPlayers->empty()) {
        if (hasPossession && player.id != activePossessorId) {
            // --- IN POSSESSION: CREATE SPACES TO RECEIVE PASSES ---

            // A. Dismarking: Check away from the nearest defender to create an open pocket
            const Player* nearestOpp = nullptr;
            float minOppDist = 999.0f;
            for (const auto& opp : *allPlayers) {
                if (opp.team != player.team) {
                    float d = (opp.position - target).length();
                    if (d < minOppDist) {
                        minOppDist = d;
                        nearestOpp = &opp;
                    }
                }
            }

            if (nearestOpp && minOppDist < 2.8f) {
                // Defender is close! Check into free space
                Vec2 awayFromMarker = target - nearestOpp->position;
                if (awayFromMarker.lengthSq() > 0.01f) {
                    awayFromMarker = awayFromMarker.normalized();
                } else {
                    awayFromMarker = Vec2(attackingDir, 0.0f);
                }
                float separation = std::min(2.0f, 2.8f - minOppDist);
                target += awayFromMarker * separation;
            }

            // B. Triangulation & Line of Sight: Form open passing lane to ball carrier
            Vec2 toPlayer = target - ballPos;
            float passDist = toPlayer.length();
            if (passDist > 4.0f && passDist < 35.0f) {
                Vec2 passDir = toPlayer.normalized();
                Vec2 perpDir(-passDir.y, passDir.x); // Perpendicular lateral vector

                for (const auto& opp : *allPlayers) {
                    if (opp.team != player.team) {
                        Vec2 toOpp = opp.position - ballPos;
                        float proj = toOpp.dot(passDir);
                        if (proj > 2.0f && proj < passDist - 1.5f) {
                            Vec2 ptOnLane = ballPos + (passDir * proj);
                            float distToLane = (opp.position - ptOnLane).length();
                            if (distToLane < 1.6f) {
                                // Defender is screening the passing lane! Shift laterally to open triangle
                                float side = ((opp.position - ptOnLane).dot(perpDir) >= 0.0f) ? -1.0f : 1.0f;
                                target += perpDir * (side * 1.8f);
                                break;
                            }
                        }
                    }
                }
            }

            // C. Attack Style nuances:
            if (tacticalStyle == TeamTacticalStyle::TikiTaka) {
                // Short passing triangles: keep close to ball carrier (12-22m)
                Vec2 toBall = ballPos - target;
                float dBall = toBall.length();
                if (dBall > 22.0f && (player.role == TacticalRole::CM_LEFT || player.role == TacticalRole::CM_RIGHT || player.role == TacticalRole::DM)) {
                    target += toBall.normalized() * (dBall - 18.0f) * 0.25f;
                }
            } else if (tacticalStyle == TeamTacticalStyle::WingPlay) {
                // Stretch wide to create crossing width
                if (player.role == TacticalRole::LW || player.role == TacticalRole::LB) {
                    target.y = std::min(target.y, 11.0f);
                } else if (player.role == TacticalRole::RW || player.role == TacticalRole::RB) {
                    target.y = std::max(target.y, 57.0f);
                }
            } else if (tacticalStyle == TeamTacticalStyle::CounterAttack) {
                // Vertical release into space
                if (player.role == TacticalRole::ST || player.role == TacticalRole::LW || player.role == TacticalRole::RW) {
                    target.x += attackingDir * 3.5f;
                }
            }
        } else if (!hasPossession) {
            // --- OUT OF POSSESSION: CLOSE SPACES & INHIBIT OPPONENT PASSES ---

            // A. Screen passing lanes to dangerous nearby attackers (Cover Shadow)
            const Player* dangerousAttacker = nullptr;
            float minAttackerDist = 14.0f;
            for (const auto& att : *allPlayers) {
                if (att.team == possessionTeam && att.id != activePossessorId && att.role != TacticalRole::GK) {
                    float d = (att.position - target).length();
                    if (d < minAttackerDist) {
                        minAttackerDist = d;
                        dangerousAttacker = &att;
                    }
                }
            }

            if (dangerousAttacker) {
                bool inDefensiveThird = (player.team == TeamSide::Home && player.position.x < 36.0f) ||
                                        (player.team == TeamSide::Away && player.position.x > 69.0f);

                if (inDefensiveThird && (player.role == TacticalRole::CB_LEFT || player.role == TacticalRole::CB_RIGHT ||
                                         player.role == TacticalRole::LB || player.role == TacticalRole::RB)) {
                    // Goal-side marking: stay goal-side of dangerous attacker while maintaining defensive line
                    Vec2 toGoal = Vec2(ownGoalX, 34.0f) - dangerousAttacker->position;
                    Vec2 goalSidePos = dangerousAttacker->position + toGoal.normalized() * 2.8f;
                    target = target * 0.90f + goalSidePos * 0.10f;
                } else {
                    // Midfield / wing cover shadow: position in the passing corridor between ball and receiver
                    Vec2 toReceiver = dangerousAttacker->position - ballPos;
                    if (toReceiver.lengthSq() > 1.0f) {
                        Vec2 screenPos = dangerousAttacker->position - toReceiver.normalized() * 3.5f;
                        target = target * 0.92f + screenPos * 0.08f;
                    }
                }
            }

            // B. Defensive block compactness
            float lateralCenterShift = (ballPos.y - 34.0f) * 0.15f;
            target.y += lateralCenterShift;

            if (mentality == DefensiveMentality::HighPress) {
                target.x += attackingDir * 4.0f; // Push forward to squeeze space
            } else if (mentality == DefensiveMentality::LowBlock) {
                target.x -= attackingDir * 4.0f; // Pack the box
            }
        }
    }

    // 4. Offside line discipline for attackers (stay on the shoulder of the last defender)
    if (hasPossession && player.role != TacticalRole::GK) {
        if (player.team == TeamSide::Home && target.x > offsideLineX - 0.60f && offsideLineX >= 52.5f) {
            target.x = std::max(52.4f, offsideLineX - 0.60f);
        } else if (player.team == TeamSide::Away && target.x < offsideLineX + 0.60f && offsideLineX <= 52.5f) {
            target.x = std::min(52.6f, offsideLineX + 0.60f);
        }
    }

    // 5. Boundary safety clamps
    target.x = std::clamp(target.x, 4.5f, SimConfig::PITCH_LENGTH_M - 4.5f);
    target.y = std::clamp(target.y, 3.5f, SimConfig::PITCH_WIDTH_M - 3.5f);

    return target;
}
