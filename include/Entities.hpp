#pragma once
#include <string>
#include <algorithm>
#include <initializer_list>
#include "Vec2.hpp"

enum class TeamSide {
    Home = 0,
    Away = 1
};

enum class TacticalRole {
    GK,
    LB,
    CB_LEFT,
    CB_RIGHT,
    RB,
    DM,
    CM_LEFT,
    CM_RIGHT,
    LW,
    ST,
    RW
};

enum class PlayerState {
    HoldingFormation,
    SupportingAttack,
    Marking,
    PressingBall,
    Dribbling,
    PreparingPass,
    PreparingShot,
    ReceivingPass,
    Recovering
};

enum class BehaviorState {
    SupportTheCarrier,      // Triangles 10-25m away with clear lane
    InsideChannelRun,       // Sprints into seam between CB and FB
    OverlapUnderlap,        // Fullback overlapping outside or underlapping inside winger
    DecoyRun,               // Rest-attack decoy run dragging defenders away
    OverloadAndSwitch,      // Overload ball side, isolated far-side winger
    RestDefence,            // 3+2 or 2+3 structure behind ball
    Press,                  // Closing down ball carrier on curved approach
    CounterPress,           // Immediate 3-5s press after possession loss
    Pivot,                  // DM shielding space in front of CBs
    DefensivePositioning    // Shape and lane covering off teammates
};

enum class FCIQRole {
    // Goalkeepers
    TraditionalGK,
    SweeperKeeper,

    // Defenders
    BallPlayingDefender,
    Stopper,
    AttackingWingback,
    InvertedFullback,

    // Midfielders
    DeepLyingPlaymaker,
    BoxToBox,
    PlaymakerCAM,
    AnchorCDM,

    // Attackers
    InsideForward,
    ClassicWinger,
    PoacherST,
    TargetForward,
    FalseNine
};

enum class TeamTacticalStyle {
    TikiTaka,       // Guardiola short-passing, possession dominance, high territory
    Gegenpressing,  // Klopp/Alonso high-tempo vertical transitions, intense pressing
    CounterAttack,  // Ancelotti/Simeone compact block, deadly direct counter-attacks
    WingPlay        // Overlapping fullbacks, wide overloads, cutbacks & crosses
};

enum class DefensiveMentality {
    HighPress,      // Front-foot pressing, line pushed to halfway
    MidBlock,       // Balanced disciplined block at middle third
    LowBlock        // Compact penalty box shield, protect lead
};

// --- Tactical Control Enums & Structures ---
enum class BlockHeight {
    Low,    // ~30m from own goal line
    Mid,    // ~45m from own goal line
    High    // ~60m from own goal line
};

enum class PressTrigger {
    BackwardPass,
    PoorTouch,
    TouchlineTrap,
    AnyLoose
};

enum class CounterPressIntensity {
    Off,
    Balanced,
    Heavy
};

enum class RestDefenceShape {
    Rest3Plus2,
    Rest2Plus3,
    Rest4Back
};

enum class ForcePlay {
    ForceWide,
    ForceCentral
};

enum class BuildUpShape {
    Back4,
    Back3PivotDrops,
    GKInvolved
};

enum class WideRotation {
    WingerInsideFBOverlaps,
    FBUnderlapsWingerWide
};

enum class Directness {
    ShortPossession,
    Mixed,
    DirectLongBall
};

enum class CrossingStyle {
    Inswinging,
    Cutback,
    DrivenLow
};

enum class TransitionStyle {
    ImmediateForwardPass,
    RecyclePossession
};

enum class CornerRoutine {
    InswingerFarPost,
    NearPostFlick,
    ShortCorner
};

enum class CornerDefense {
    Zonal,
    ManMarking,
    Hybrid
};

struct OutOfPossessionTactics {
    BlockHeight blockHeight{BlockHeight::Mid};
    PressTrigger pressTrigger{PressTrigger::BackwardPass};
    CounterPressIntensity counterPressIntensity{CounterPressIntensity::Balanced};
    RestDefenceShape restDefence{RestDefenceShape::Rest3Plus2};
    ForcePlay forcePlay{ForcePlay::ForceWide};
    float compactness{32.0f}; // In mid-block: 30-35m length
};

struct InPossessionTactics {
    BuildUpShape buildUpShape{BuildUpShape::Back3PivotDrops};
    WideRotation wideRotation{WideRotation::WingerInsideFBOverlaps};
    Directness directness{Directness::ShortPossession};
    CrossingStyle crossingStyle{CrossingStyle::Inswinging};
    TransitionStyle transitionStyle{TransitionStyle::ImmediateForwardPass};
    bool invertedFullbacks{true};
    float width{58.0f}; // Expands towards 68m in attack
    float insideChannelRunFreq{0.65f};
    float overloadTempo{0.60f};
};

struct SetPieceTactics {
    CornerRoutine cornerRoutine{CornerRoutine::InswingerFarPost};
    CornerDefense cornerDefense{CornerDefense::Hybrid};
    bool longThrow{false};
};

struct TeamTactics {
    OutOfPossessionTactics outOfPossession;
    InPossessionTactics inPossession;
    SetPieceTactics setPieces;
};

struct PlayerAttributes {
    // Kinematics (in m/s and m/s^2)
    float maxSpeed{8.8f};
    float acceleration{4.5f};
    float passSkill{0.92f};
    float shotPower{26.5f};
    float tackleSkill{0.80f};

    // Granular EA FC 25 / FM Archetype Ratings (0 - 99 scale)
    int overallRating{80};
    int pace{80};
    int shooting{75};
    int passing{80};
    int dribbling{78};
    int defending{75};
    int physical{75};
    int tackling{78};
    int positioning{80};
    int aerial{74};
    int pressResistance{82};

    // Specialized positional sub-attributes [0.0 to 1.0]
    float finishing{0.75f};          // Strikers 0.88-0.93, CBs 0.35
    float vision{0.80f};             // Playmakers 0.94, Wingers 0.75
    float crossing{0.78f};           // Wingbacks / Wingers 0.86
    float composure{0.80f};          // Resistance to press
    float defensiveAwareness{0.78f}; // Position tracking & lane cutting

    // Goalkeeping (for GKs)
    float gkReflexes{0.86f};         // Reaction delay reduction
    float gkDiving{0.86f};           // Lateral dive velocity
    float gkHandling{0.82f};         // Catch probability vs parry rebound
    float gkPositioning{0.85f};      // Coverage angle alignment

    PlayerAttributes() = default;

    PlayerAttributes(float spd, float acc, float passSk, float shotPwr, float tklSk,
                     int ovr, int p, int sh, int pa, int dr, int def, int phy,
                     float fin, float vis, float crs, float comp, float defAw,
                     float gkRef = 0.5f, float gkDiv = 0.5f, float gkHnd = 0.5f, float gkPos = 0.5f)
        : maxSpeed(spd), acceleration(acc), passSkill(passSk), shotPower(shotPwr), tackleSkill(tklSk),
          overallRating(ovr), pace(p), shooting(sh), passing(pa), dribbling(dr), defending(def), physical(phy),
          tackling(def), positioning(static_cast<int>(vis * 100.0f)), aerial(phy), pressResistance(static_cast<int>(comp * 100.0f)),
          finishing(fin), vision(vis), crossing(crs), composure(comp), defensiveAwareness(defAw),
          gkReflexes(gkRef), gkDiving(gkDiv), gkHandling(gkHnd), gkPositioning(gkPos) {}

    // Backwards compatibility constructor
    PlayerAttributes(float spd, float acc, float passSk, float shotPwr, float tklSk,
                     int p, int sh, int pa, int dr, int def, int phy,
                     float fin, float vis, float crs, float comp, float defAw,
                     float gkRef = 0.5f, float gkDiv = 0.5f, float gkHnd = 0.5f, float gkPos = 0.5f)
        : maxSpeed(spd), acceleration(acc), passSkill(passSk), shotPower(shotPwr), tackleSkill(tklSk),
          overallRating(std::max({p, sh, pa, dr, def, phy})), pace(p), shooting(sh), passing(pa), dribbling(dr), defending(def), physical(phy),
          tackling(def), positioning(static_cast<int>(vis * 100.0f)), aerial(phy), pressResistance(static_cast<int>(comp * 100.0f)),
          finishing(fin), vision(vis), crossing(crs), composure(comp), defensiveAwareness(defAw),
          gkReflexes(gkRef), gkDiving(gkDiv), gkHandling(gkHnd), gkPositioning(gkPos) {}
};

struct Player {
    int id{0};
    int shirtNumber{1};
    TeamSide team{TeamSide::Home};
    std::string name;
    TacticalRole role{TacticalRole::CM_LEFT};
    FCIQRole fcIqRole{FCIQRole::BoxToBox};
    PlayerState state{PlayerState::HoldingFormation};
    BehaviorState behaviorState{BehaviorState::DefensivePositioning};
    PlayerAttributes attributes;

    Vec2 position{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    Vec2 homePosition{0.0f, 0.0f};
    Vec2 dynamicAnchor{0.0f, 0.0f};     // Layer 1 unit formation anchor
    Vec2 targetSteeringPos{0.0f, 0.0f}; // Layer 2 desired role destination
    float headingDeg{0.0f};

    float distanceCoveredM{0.0f};
    float sprintDistanceM{0.0f};        // Distance covered in high-speed sprint (> 5.5 m/s)
    float stamina{100.0f};

    float touchCooldown{0.0f};
    float possessionDuration{0.0f};     // Time spent holding the ball continuously
    bool hasPossession{false};

    float decisionTimer{0.0f};          // Re-evaluate Layer 2 role behavior every 0.25-0.5s
    float reactionDelay{0.20f};         // Asynchronous reaction delay (150-300ms)

    bool isInsideChannelRunner{false};
    bool isDecoyRunner{false};

    // RandomEngine-driven fields
    float formFactor{1.0f};      // Performance multiplier [0.75 .. 1.25], drifts over time
    float fatigue{1.0f};         // Speed/decision multiplier, decreases over match [0.70 .. 1.0]
    float passHoldTime{1.0f};    // Randomised per-possession hold-before-pass threshold
    float formTimer{0.0f};       // Seconds since last form update

    // Discipline & Bookings (Law 12)
    int yellowCards{0};          // 1 = Cautioned, 2 = Sent Off
    bool isRedCarded{false};     // Sent off (leaves pitch, team down to 10 players)
};

enum class ShotOutcome {
    None,
    Goal,
    Woodwork,
    Save,
    OffTarget
};

struct Ball {
    Vec2 position{52.5f, 34.0f};
    Vec2 velocity{0.0f, 0.0f};
    float height{0.0f};
    float verticalVelocity{0.0f};
    float radius{0.22f};

    int possessorId{-1};
    int intendedReceiverId{-1}; // Target of current pass in transit
    int lastTouchedPlayerId{-1};
    TeamSide lastTouchedTeam{TeamSide::Home};
    bool isShot{false};          // True if ball is currently a shot on goal
    bool isCross{false};         // True if ball is currently a cross delivery
    bool isOffsidePass{false};   // True if pass was kicked to a player in offside position (Law 11)
    ShotOutcome shotOutcome{ShotOutcome::None};

    void applyImpulse(const Vec2& impulse, float vertImpulse = 0.0f) {
        velocity += impulse;
        verticalVelocity += vertImpulse;
        possessorId = -1;
    }
};

enum class MatchPhase {
    Kickoff,
    InPlay,
    BuildUp,
    Progression,
    FinalThird,
    TransitionToAttack,
    TransitionToDefend,
    SetPiece,
    LooseBall,
    GoalScored,
    FreeKick,
    PenaltyKick,
    GoalKick,
    ThrowIn,
    Corner,
    HalfTime,
    FullTime
};
