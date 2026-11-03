#include "MatchEngine.hpp"
#include "EmpiricalTrainedModel.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

MatchEngine::MatchEngine(unsigned int randomSeed)
    : m_randomEngine(randomSeed) {
    resetMatch();
}

void MatchEngine::resetMatch() {
    m_players.clear();
    m_players.reserve(22);

    const auto& homeProf = getHomeTeamProfile();
    const auto& awayProf = getAwayTeamProfile();
    m_homeFormation = homeProf.defaultFormation;
    m_awayFormation = awayProf.defaultFormation;
    m_homeTacticalStyle = homeProf.tacticalStyle;
    m_awayTacticalStyle = awayProf.tacticalStyle;
    m_homeMentality = homeProf.defensiveMentality;
    m_awayMentality = awayProf.defensiveMentality;

    m_homeTactics = Tactics::getDefaultTacticsForStyle(m_homeTacticalStyle, m_homeMentality);
    m_awayTactics = Tactics::getDefaultTacticsForStyle(m_awayTacticalStyle, m_awayMentality);
    m_homeTactics.inPossession.width = std::clamp(m_homeTactics.inPossession.width * homeProf.widthMultiplier, 40.0f, 68.0f);
    m_awayTactics.inPossession.width = std::clamp(m_awayTactics.inPossession.width * awayProf.widthMultiplier, 40.0f, 68.0f);
    if (homeProf.pressIntensityMultiplier > 1.1f) {
        m_homeTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Heavy;
    } else if (homeProf.pressIntensityMultiplier < 0.9f) {
        m_homeTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Off;
    }
    if (awayProf.pressIntensityMultiplier > 1.1f) {
        m_awayTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Heavy;
    } else if (awayProf.pressIntensityMultiplier < 0.9f) {
        m_awayTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Off;
    }

    rebuildTeamLineup(TeamSide::Home);
    rebuildTeamLineup(TeamSide::Away);

    m_homeScore = 0;
    m_awayScore = 0;
    m_matchTime = 0.0f;
    m_spatialSamplingTimer = 0.0f;

    // Generate random stoppage/injury time (1 to 7 minutes) for both halves
    m_firstHalfAddedTime = std::round(m_randomEngine.uniform(1.0f, 7.0f)) * 60.0f;
    m_secondHalfAddedTime = std::round(m_randomEngine.uniform(1.0f, 7.0f)) * 60.0f;
    m_firstHalfEnded = false;
    m_secondHalfEnded = false;

    m_homeCounterPressTimer = 0.0f;
    m_awayCounterPressTimer = 0.0f;
    m_possessionSequenceDuration = 0.0f;
    m_lastPossessionChangeTime = 0.0f;
    m_recentPasserId = -1;
    m_possessionChainTouches.fill(0);

    m_eventRecorder.reset();
    m_analytics.reset();
    m_spatialAnalytics.reset();
    setupKickoff(TeamSide::Home);
}

const TeamProfile& MatchEngine::getHomeTeamProfile() const {
    return TeamDatabase::getTeam(m_homeTeamIndex);
}

const TeamProfile& MatchEngine::getAwayTeamProfile() const {
    return TeamDatabase::getTeam(m_awayTeamIndex);
}

void MatchEngine::selectHomeTeam(int index) {
    m_homeTeamIndex = ((index % TeamDatabase::getTeamCount()) + TeamDatabase::getTeamCount()) % TeamDatabase::getTeamCount();
    const auto& prof = getHomeTeamProfile();
    m_homeFormation = prof.defaultFormation;
    m_homeTacticalStyle = prof.tacticalStyle;
    m_homeMentality = prof.defensiveMentality;
    m_homeTactics = Tactics::getDefaultTacticsForStyle(m_homeTacticalStyle, m_homeMentality);
    m_homeTactics.inPossession.width = std::clamp(m_homeTactics.inPossession.width * prof.widthMultiplier, 40.0f, 68.0f);
    if (prof.pressIntensityMultiplier > 1.1f) {
        m_homeTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Heavy;
    } else if (prof.pressIntensityMultiplier < 0.9f) {
        m_homeTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Off;
    }
    rebuildTeamLineup(TeamSide::Home);
    if (m_phase == MatchPhase::Kickoff) {
        setupKickoff(m_kickoffTeam);
    }
    std::cout << "[CLUB SELECT] Home Team: " << prof.name << " (" << prof.shortName << ") - "
              << prof.manager << " [" << prof.league << "]\n";
}

void MatchEngine::selectAwayTeam(int index) {
    m_awayTeamIndex = ((index % TeamDatabase::getTeamCount()) + TeamDatabase::getTeamCount()) % TeamDatabase::getTeamCount();
    const auto& prof = getAwayTeamProfile();
    m_awayFormation = prof.defaultFormation;
    m_awayTacticalStyle = prof.tacticalStyle;
    m_awayMentality = prof.defensiveMentality;
    m_awayTactics = Tactics::getDefaultTacticsForStyle(m_awayTacticalStyle, m_awayMentality);
    m_awayTactics.inPossession.width = std::clamp(m_awayTactics.inPossession.width * prof.widthMultiplier, 40.0f, 68.0f);
    if (prof.pressIntensityMultiplier > 1.1f) {
        m_awayTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Heavy;
    } else if (prof.pressIntensityMultiplier < 0.9f) {
        m_awayTactics.outOfPossession.counterPressIntensity = CounterPressIntensity::Off;
    }
    rebuildTeamLineup(TeamSide::Away);
    if (m_phase == MatchPhase::Kickoff) {
        setupKickoff(m_kickoffTeam);
    }
    std::cout << "[CLUB SELECT] Away Team: " << prof.name << " (" << prof.shortName << ") - "
              << prof.manager << " [" << prof.league << "]\n";
}

void MatchEngine::cycleHomeTeam() {
    selectHomeTeam(m_homeTeamIndex + 1);
}

void MatchEngine::cycleAwayTeam() {
    selectAwayTeam(m_awayTeamIndex + 1);
}

void MatchEngine::startSecondHalf() {
    std::cout << "[WHISTLE] Second Half Kickoff!\n";
    m_kickoffTeam = (m_kickoffTeam == TeamSide::Home) ? TeamSide::Away : TeamSide::Home;
    m_matchTime = SimConfig::HALF_TIME_DURATION_S; // Exactly 45:00
    m_firstHalfEnded = true;
    setupKickoff(m_kickoffTeam);
    m_phase = MatchPhase::Kickoff;
}

void MatchEngine::setTeamFormation(TeamSide team, FormationType type) {
    if (team == TeamSide::Home) {
        m_homeFormation = type;
    } else {
        m_awayFormation = type;
    }
    rebuildTeamLineup(team);
    std::cout << "[Tactics] " << (team == TeamSide::Home ? "Home" : "Away")
              << " switched to formation " << Tactics::formationToString(type) << "\n";
}

void MatchEngine::setHomeTacticalStyle(TeamTacticalStyle s) {
    m_homeTacticalStyle = s;
    m_homeTactics = Tactics::getDefaultTacticsForStyle(s, m_homeMentality);
}

void MatchEngine::setAwayTacticalStyle(TeamTacticalStyle s) {
    m_awayTacticalStyle = s;
    m_awayTactics = Tactics::getDefaultTacticsForStyle(s, m_awayMentality);
}

void MatchEngine::cycleHomeTacticalStyle() {
    int next = (static_cast<int>(m_homeTacticalStyle) + 1) % 4;
    setHomeTacticalStyle(static_cast<TeamTacticalStyle>(next));
    std::cout << "[TACTICS] Home switched to " << Tactics::tacticalStyleToString(m_homeTacticalStyle) << "\n";
}

void MatchEngine::cycleAwayTacticalStyle() {
    int next = (static_cast<int>(m_awayTacticalStyle) + 1) % 4;
    setAwayTacticalStyle(static_cast<TeamTacticalStyle>(next));
    std::cout << "[TACTICS] Away switched to " << Tactics::tacticalStyleToString(m_awayTacticalStyle) << "\n";
}

void MatchEngine::rebuildTeamLineup(TeamSide team) {
    FormationType formation = (team == TeamSide::Home) ? m_homeFormation : m_awayFormation;
    auto slots = Tactics::getFormationSlots(formation);

    m_players.erase(
        std::remove_if(m_players.begin(), m_players.end(), [team](const Player& p) { return p.team == team; }),
        m_players.end()
    );

    int baseId = (team == TeamSide::Home) ? 1 : 12;
    const TeamProfile& profile = (team == TeamSide::Home) ? getHomeTeamProfile() : getAwayTeamProfile();

    for (size_t i = 0; i < slots.size(); ++i) {
        const auto& slot = slots[i];
        Vec2 pitchPos = Tactics::orientForTeam(slot.homePitchPos, team);

        Player p;
        p.id = baseId + static_cast<int>(i);
        p.team = team;

        if (i < profile.squad.size()) {
            const auto& realP = profile.squad[i];
            p.name = realP.name;
            p.shirtNumber = realP.shirtNumber;
            p.role = slot.role;
            p.fcIqRole = realP.fcIqRole;
            p.attributes = realP.attributes;
            p.attributes.overallRating = realP.overallRating;
        } else {
            p.shirtNumber = slot.shirtNumber;
            p.name = (team == TeamSide::Home ? "Home " : "Away ") + slot.name;
            p.role = slot.role;
            p.fcIqRole = slot.fcIqRole;
            p.attributes = slot.defaultAttributes;
        }

        p.yellowCards = 0;
        p.isRedCarded = false;

        // Apply team tactical multiplier effects
        p.attributes.passSkill = std::clamp(p.attributes.passSkill * profile.passSkillMultiplier, 0.40f, 0.99f);
        p.attributes.tackleSkill = std::clamp(p.attributes.tackleSkill * profile.defensiveTackleMultiplier, 0.35f, 0.99f);
        p.attributes.maxSpeed = std::clamp(p.attributes.maxSpeed * profile.counterSpeedMultiplier, 6.0f, 10.2f);
        p.attributes.acceleration = std::clamp(p.attributes.acceleration * profile.counterSpeedMultiplier, 3.0f, 6.0f);

        // Subtle stochastic variance
        p.attributes.maxSpeed *= m_randomEngine.uniform(0.98f, 1.02f);
        p.attributes.acceleration *= m_randomEngine.uniform(0.98f, 1.02f);
        p.attributes.passSkill *= m_randomEngine.uniform(0.98f, 1.02f);
        p.attributes.shotPower *= m_randomEngine.uniform(0.98f, 1.02f);
        p.attributes.tackleSkill *= m_randomEngine.uniform(0.98f, 1.02f);

        p.state = PlayerState::HoldingFormation;
        p.behaviorState = BehaviorState::DefensivePositioning;
        p.position = pitchPos;
        p.velocity = {0.0f, 0.0f};
        p.homePosition = pitchPos;
        p.dynamicAnchor = pitchPos;
        p.targetSteeringPos = pitchPos;

        p.formFactor = m_randomEngine.uniform(0.94f, 1.06f);
        p.passHoldTime = m_randomEngine.randomizedHoldTime(0.80f);
        p.fatigue = 1.0f;
        p.stamina = 100.0f;
        p.distanceCoveredM = 0.0f;
        p.sprintDistanceM = 0.0f;
        p.formTimer = m_randomEngine.uniform(0.0f, 2.0f);
        p.reactionDelay = m_randomEngine.uniform(0.18f, 0.28f);

        m_players.push_back(p);
    }
}

void MatchEngine::setupKickoff(TeamSide kickingTeam) {
    m_kickoffTeam = kickingTeam;
    m_possessionTeam = kickingTeam;
    m_phase = MatchPhase::Kickoff;
    m_phaseTimer = 0.8f;

    m_ball.position = {SimConfig::PITCH_LENGTH_M * 0.5f, SimConfig::PITCH_WIDTH_M * 0.5f};
    m_ball.velocity = {0.0f, 0.0f};
    m_ball.height = 0.0f;
    m_ball.verticalVelocity = 0.0f;
    m_ball.possessorId = -1;
    m_ball.intendedReceiverId = -1;
    m_ball.isShot = false;
    m_ball.isCross = false;
    m_ball.isOffsidePass = false;
    m_recentPasserId = -1;
    m_possessionChainTouches.fill(0);

    for (auto& p : m_players) {
        p.position = p.homePosition;
        p.dynamicAnchor = p.homePosition;
        p.targetSteeringPos = p.homePosition;
        p.velocity = {0.0f, 0.0f};
        p.hasPossession = false;
        p.possessionDuration = 0.0f;
        p.state = PlayerState::HoldingFormation;
        p.behaviorState = BehaviorState::DefensivePositioning;
        p.touchCooldown = 0.0f;
    }

    // Striker starts on the ball
    for (auto& p : m_players) {
        if (p.team == kickingTeam && p.role == TacticalRole::ST) {
            p.position = m_ball.position + ((kickingTeam == TeamSide::Home) ? Vec2(-0.8f, 0.0f) : Vec2(0.8f, 0.0f));
            p.hasPossession = true;
            m_ball.possessorId = p.id;
            break;
        }
    }

    m_eventRecorder.recordEvent({m_matchTime, MatchEventType::KickoffEvent, kickingTeam, -1, -1, m_ball.position, "Match Kickoff"});
}

void MatchEngine::update(float dt) {
    // 1. Clock freeze during Half Time or Full Time
    if (m_phase == MatchPhase::HalfTime || m_phase == MatchPhase::FullTime) {
        return;
    }

    m_matchTime += dt;

    // 2. Official Match Stoppages (Half-Time & Full-Time with Stoppage / Injury Time)
    float htTargetTime = SimConfig::HALF_TIME_DURATION_S + m_firstHalfAddedTime;
    if (!m_firstHalfEnded && m_matchTime >= htTargetTime) {
        m_phase = MatchPhase::HalfTime;
        m_ball.velocity = {0.0f, 0.0f};
        m_ball.possessorId = -1;
        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::Interception, TeamSide::Home, -1, -1, m_ball.position, "Referee whistle: Half Time"});
        std::cout << "[WHISTLE] Half Time reached! (45:00 +" << static_cast<int>(m_firstHalfAddedTime / 60.0f) << "') - Press [SPACE] for 2nd Half\n";
        return;
    }

    float ftTargetTime = SimConfig::FULL_TIME_DURATION_S + m_secondHalfAddedTime;
    if (m_firstHalfEnded && !m_secondHalfEnded && m_matchTime >= ftTargetTime) {
        m_phase = MatchPhase::FullTime;
        m_secondHalfEnded = true;
        m_ball.velocity = {0.0f, 0.0f};
        m_ball.possessorId = -1;
        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::Interception, TeamSide::Home, -1, -1, m_ball.position, "Referee whistle: Full Time"});
        std::cout << "[WHISTLE] Full Time reached! (90:00 +" << static_cast<int>(m_secondHalfAddedTime / 60.0f) << "')\n";
        return;
    }

    // Dynamic in-match tactical adaptation based on scoreline, match clock, and state
    Tactics::adaptTacticsDynamically(TeamSide::Home, m_homeScore, m_awayScore, m_matchTime, m_homeTacticalStyle, m_homeMentality);
    Tactics::adaptTacticsDynamically(TeamSide::Away, m_homeScore, m_awayScore, m_matchTime, m_awayTacticalStyle, m_awayMentality);

    if (m_phase == MatchPhase::GoalScored) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.0f) {
            setupKickoff(m_possessionTeam == TeamSide::Home ? TeamSide::Away : TeamSide::Home);
        }
        return;
    }

    if (m_phase == MatchPhase::Kickoff) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.0f) {
            for (auto& p : m_players) {
                if (p.hasPossession) {
                    executeKickoff(p);
                    break;
                }
            }
        }
        return;
    }

    if (m_phase == MatchPhase::BuildUp) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.0f) {
            m_phase = MatchPhase::InPlay;
        }
    }

    updatePlayerAI(dt);
    updatePlayerKinematics(dt);
    updateBallPhysics(dt);
    handleBallPlayerCollisions();
    checkPitchBoundariesAndGoals();

    m_analytics.update(m_matchTime, m_eventRecorder, m_ball, m_possessionTeam);

    m_spatialSamplingTimer += dt;
    if (m_spatialSamplingTimer >= 0.25f) {
        m_spatialSamplingTimer -= 0.25f;
        m_spatialAnalytics.recordPositions(m_players);
    }
}

void MatchEngine::updateBallPhysics(float dt) {
    if (m_ball.possessorId != -1) {
        for (const auto& p : m_players) {
            if (p.id == m_ball.possessorId) {
                Vec2 facingDir = (p.velocity.lengthSq() > 0.01f) ? p.velocity.normalized() :
                                 (p.team == TeamSide::Home ? Vec2(1.0f, 0.0f) : Vec2(-1.0f, 0.0f));
                m_ball.position = p.position + (facingDir * SimConfig::DRIBBLE_OFFSET_M);
                m_ball.velocity = p.velocity;
                m_ball.height = 0.0f;
                m_ball.verticalVelocity = 0.0f;
                return;
            }
        }
    }

    m_ball.position += m_ball.velocity * dt;

    if (m_ball.height <= 0.001f) {
        m_ball.velocity *= std::pow(SimConfig::BALL_FRICTION_ROLL, dt * 60.0f);
        if (m_ball.velocity.lengthSq() < 0.04f) {
            m_ball.velocity = {0.0f, 0.0f};
        }
    } else {
        m_ball.velocity *= std::pow(SimConfig::BALL_AIR_DRAG, dt * 60.0f);
        m_ball.verticalVelocity -= SimConfig::GRAVITY * dt;
        m_ball.height += m_ball.verticalVelocity * dt;

        if (m_ball.height < 0.0f) {
            m_ball.height = 0.0f;
            m_ball.verticalVelocity = -m_ball.verticalVelocity * SimConfig::BALL_BOUNCE_RESTITUTION;
            if (std::abs(m_ball.verticalVelocity) < 0.4f) {
                m_ball.verticalVelocity = 0.0f;
            }
        }
    }
}

void MatchEngine::updateGoalkeeperAI(Player& gk, float dt) {
    float goalLineX = (gk.team == TeamSide::Home) ? 4.5f : SimConfig::PITCH_LENGTH_M - 4.5f;
    float goalCenterY = SimConfig::PITCH_WIDTH_M * 0.5f;
    float goalX = (gk.team == TeamSide::Home) ? 0.0f : SimConfig::PITCH_LENGTH_M;

    // 1. If this goalkeeper is currently holding the ball
    if (gk.hasPossession && m_ball.possessorId == gk.id) {
        m_ball.position = gk.position;
        m_ball.velocity = {0.0f, 0.0f};
        m_ball.height = 0.0f;
        m_ball.verticalVelocity = 0.0f;

        // Law 16 / Fair Play: Opponents must retreat outside penalty area during GK restarts
        for (auto& opp : m_players) {
            if (opp.team != gk.team) {
                if (gk.team == TeamSide::Home && opp.position.x < 18.5f) {
                    opp.position.x = 20.0f;
                    opp.velocity = {0.0f, 0.0f};
                } else if (gk.team == TeamSide::Away && opp.position.x > 86.5f) {
                    opp.position.x = 85.0f;
                    opp.velocity = {0.0f, 0.0f};
                }
            }
        }

        gk.possessionDuration += dt;
        if (gk.possessionDuration >= 1.4f) {
            gk.possessionDuration = 0.0f;
            gk.hasPossession = false;
            gk.touchCooldown = 0.8f;
            executeGKDistribution(gk);
        }
        return;
    }

    // 2. 6-yard box loose ball claim
    if (m_ball.possessorId == -1 && !m_ball.isShot && m_ball.velocity.length() < 4.0f) {
        float distToBall = (m_ball.position - gk.position).length();
        bool inGoalArea = (gk.team == TeamSide::Home) ? (m_ball.position.x < 5.5f) : (m_ball.position.x > SimConfig::PITCH_LENGTH_M - 5.5f);
        if (distToBall < 5.0f && inGoalArea && std::abs(m_ball.position.y - 34.0f) < 10.0f) {
            for (auto& p : m_players) p.hasPossession = false;
            gk.hasPossession = true;
            m_ball.possessorId = gk.id;
            m_ball.intendedReceiverId = -1;
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_ball.isOffsidePass = false;
            m_ball.velocity = {0.0f, 0.0f};
            m_ball.height = 0.0f;
            m_ball.lastTouchedPlayerId = gk.id;
            m_ball.lastTouchedTeam = gk.team;
            m_possessionTeam = gk.team;
            gk.possessionDuration = 0.0f;
            gk.touchCooldown = 0.0f;
            std::cout << "[GK Claim] " << gk.name << " gathers loose ball in goal area\n";
            return;
        }
    }

    // 3. Tactical positioning along goal arc & angle bisector
    Vec2 toBall = m_ball.position - Vec2(goalLineX, goalCenterY);
    float ballDist = toBall.length();

    float advanceDist = (gk.fcIqRole == FCIQRole::SweeperKeeper)
        ? std::min(5.2f, ballDist * 0.14f)
        : std::min(3.2f, ballDist * 0.09f);

    float desiredX = (gk.team == TeamSide::Home) ? (goalLineX + advanceDist) : (goalLineX - advanceDist);

    // Goal posts are at y = 30.34m to 37.66m (center 34.0m)
    // Angle bisector positioning: lateral shift proportional to ball offset and distance
    float lateralBias = m_ball.position.y - goalCenterY;
    float angleScaling = std::min(0.24f, advanceDist / std::max(ballDist, 6.0f));
    float desiredY = goalCenterY + (lateralBias * angleScaling);
    desiredY = std::clamp(desiredY, 31.4f, 36.6f);

    Vec2 targetPos(desiredX, desiredY);

    // 4. Physics-based Real Goalkeeper Shot-Stopping Model (Only for real shots on goal!)
    bool ballMovingTowardThisGK = (gk.team == TeamSide::Home) ? (m_ball.velocity.x < -3.5f) : (m_ball.velocity.x > 3.5f);
    bool ballInDefensiveThird = (gk.team == TeamSide::Home) ? (m_ball.position.x < 35.0f) : (m_ball.position.x > 70.0f);
    bool gkNotOnCooldown = (gk.touchCooldown <= 0.0f);

    if (m_ball.isShot && m_ball.possessorId == -1 && ballMovingTowardThisGK && ballInDefensiveThird && gkNotOnCooldown) {
        float vx = std::abs(m_ball.velocity.x);
        float distToGoalX = std::abs(goalX - m_ball.position.x);
        float timeToGoal = distToGoalX / std::max(vx, 0.1f);
        float projectedY = m_ball.position.y + (m_ball.velocity.y * timeToGoal);

        bool headingOnTarget = (projectedY >= 29.8f && projectedY <= 38.2f);
        targetPos.y = std::clamp(projectedY, 30.5f, 37.5f);

        float distToGKX = std::abs(gk.position.x - m_ball.position.x);
        float timeToGKPlane = distToGKX / std::max(vx, 0.1f);

        if (timeToGKPlane <= 0.42f && distToGKX < 4.5f) {
            float reactionDelay = SimConfig::GK_BASE_REACTION_DELAY_S * (1.15f - gk.attributes.gkReflexes * 0.35f);
            float diveTime = std::max(0.0f, timeToGKPlane - reactionDelay);

            float maxDiveReach = SimConfig::GK_ARM_SPAN_M + (diveTime * gk.attributes.gkDiving * SimConfig::GK_DIVE_VELOCITY_MPS);
            float lateralDistNeeded = std::abs(projectedY - gk.position.y);

            if (m_ball.shotOutcome == ShotOutcome::Save || lateralDistNeeded <= maxDiveReach) {
                float catchProb = 0.42f * (0.8f + gk.attributes.gkHandling * 0.25f);
                float cornerTipProb = 0.38f;
                float roll = m_randomEngine.uniform(0.0f, 1.0f);

                if (roll < catchProb) {
                    std::cout << "[SAVE!] " << gk.name << " catches shot cleanly!\n";
                    m_eventRecorder.recordEvent({m_matchTime, MatchEventType::GoalkeeperSave, gk.team, gk.id, -1, gk.position, "Shot caught by GK"});

                    for (auto& p : m_players) p.hasPossession = false;
                    gk.hasPossession = true;
                    m_ball.possessorId = gk.id;
                    m_ball.intendedReceiverId = -1;
                    m_ball.isShot = false;
                    m_ball.isCross = false;
                    m_ball.isOffsidePass = false;
                    m_ball.velocity = {0.0f, 0.0f};
                    m_ball.height = 0.0f;
                    m_ball.lastTouchedPlayerId = gk.id;
                    m_ball.lastTouchedTeam = gk.team;
                    m_possessionTeam = gk.team;
                    gk.possessionDuration = 0.0f;
                    gk.touchCooldown = 0.0f;
                    return;
                } else if (roll < catchProb + cornerTipProb) {
                    // Reckless / fingertip diving save tipped around the post or over the bar -> CORNER KICK!
                    std::cout << "[RECKLESS SAVE!] " << gk.name << " makes an acrobatic fingertip save, deflecting the ball over the byline for a corner!\n";
                    m_eventRecorder.recordEvent({m_matchTime, MatchEventType::GoalkeeperSave, gk.team, gk.id, -1, gk.position, "Reckless save deflected over byline for corner"});

                    float cornerSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
                    float bylineY = 34.0f + (cornerSide * m_randomEngine.uniform(5.5f, 12.0f));
                    bylineY = std::clamp(bylineY, 2.0f, SimConfig::PITCH_WIDTH_M - 2.0f);

                    if (gk.team == TeamSide::Home) {
                        m_ball.position = Vec2(-0.6f, bylineY);
                        m_ball.velocity = Vec2(-m_randomEngine.uniform(3.5f, 6.5f), cornerSide * m_randomEngine.uniform(2.0f, 4.0f));
                    } else {
                        m_ball.position = Vec2(SimConfig::PITCH_LENGTH_M + 0.6f, bylineY);
                        m_ball.velocity = Vec2(m_randomEngine.uniform(3.5f, 6.5f), cornerSide * m_randomEngine.uniform(2.0f, 4.0f));
                    }
                    m_ball.height = 2.6f; // tipped over bar / around post
                    m_ball.verticalVelocity = m_randomEngine.uniform(0.5f, 2.0f);
                    m_ball.isShot = false;
                    m_ball.isCross = false;
                    m_ball.possessorId = -1;
                    m_ball.intendedReceiverId = -1;
                    m_ball.lastTouchedPlayerId = gk.id;
                    m_ball.lastTouchedTeam = gk.team;
                    gk.touchCooldown = 1.0f;
                    return;
                } else {
                    std::cout << "[PARRY!] " << gk.name << " parries shot into play!\n";
                    m_eventRecorder.recordEvent({m_matchTime, MatchEventType::GoalkeeperSave, gk.team, gk.id, -1, gk.position, "Shot parried into play by GK"});

                    float lateralSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
                    float fwdSign = (gk.team == TeamSide::Home) ? 0.40f : -0.40f;
                    Vec2 parryDir = Vec2(fwdSign, lateralSide * 0.90f).normalized();

                    m_ball.velocity = parryDir * m_randomEngine.uniform(8.0f, 13.0f);
                    m_ball.height = 0.35f;
                    m_ball.verticalVelocity = m_randomEngine.uniform(1.2f, 2.4f);
                    m_ball.isShot = false;
                    m_ball.isCross = false;
                    m_ball.possessorId = -1;
                    m_ball.intendedReceiverId = -1;
                    m_ball.lastTouchedPlayerId = gk.id;
                    m_ball.lastTouchedTeam = gk.team;
                    gk.touchCooldown = 0.8f;
                    return;
                }
            } else if (headingOnTarget) {
                std::cout << "[UNSTOPPABLE!] " << gk.name << " beaten into the corner!\n";
                gk.touchCooldown = 2.0f;
            }
        }
    }

    // 5. Calm Set Position with Deadband Hysteresis (Eliminates unnatural constant micro-jitter)
    float distToTarget = (targetPos - gk.position).length();

    if (m_ball.isShot) {
        // High urgency: dive/sprint to cover shot
        Vec2 moveDir = (distToTarget > 0.05f) ? (targetPos - gk.position).normalized() : Vec2(0.0f, 0.0f);
        gk.velocity = moveDir * gk.attributes.maxSpeed;
        gk.position += gk.velocity * dt;
    } else if (distToTarget < 0.70f) {
        // Comfortably in set position: stand still, no constant jittering!
        gk.velocity = {0.0f, 0.0f};
    } else {
        // Decisive repositioning step, then reset
        float shuffleSpeed = std::min(2.8f, distToTarget * 3.2f);
        gk.velocity = (targetPos - gk.position).normalized() * shuffleSpeed;
        gk.position += gk.velocity * dt;
    }

    if (ballDist > 0.1f) {
        gk.headingDeg = std::atan2(toBall.y, toBall.x) * (180.0f / 3.14159265f);
    }
}

void MatchEngine::updatePlayerAI(float dt) {
    // 1. Single designated presser per team (avoids absurd 11-player swarming)
    int homeDesignatedPresser = -1;
    float homeMinDistSq = 1e9f;
    int awayDesignatedPresser = -1;
    float awayMinDistSq = 1e9f;

    for (const auto& p : m_players) {
        if (p.role == TacticalRole::GK) continue;
        float dSq = (p.position - m_ball.position).lengthSq();
        if (p.team == TeamSide::Home) {
            if (dSq < homeMinDistSq) {
                homeMinDistSq = dSq;
                homeDesignatedPresser = p.id;
            }
        } else {
            if (dSq < awayMinDistSq) {
                awayMinDistSq = dSq;
                awayDesignatedPresser = p.id;
            }
        }
    }

    // 2. Evaluate player states & decisions
    for (auto& p : m_players) {
        if (p.isRedCarded) {
            p.hasPossession = false;
            p.velocity = {0.0f, 0.0f};
            p.position = Vec2(-200.0f, -200.0f);
            continue;
        }

        if (p.touchCooldown > 0.0f) p.touchCooldown -= dt;

        p.formTimer += dt;
        if (p.formTimer >= 2.0f) {
            p.formTimer = 0.0f;
            p.formFactor = m_randomEngine.driftForm(p.formFactor);
        }

        if (p.role == TacticalRole::GK) {
            updateGoalkeeperAI(p, dt);
            continue;
        }

        if (p.id == m_ball.intendedReceiverId && m_ball.possessorId == -1) {
            p.state = PlayerState::ReceivingPass;
            continue;
        }

        if (p.hasPossession) {
            p.possessionDuration += dt;

            float attackingGoalX = (p.team == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
            float distToGoal = std::abs(p.position.x - attackingGoalX);
            float lateralDistToGoalCenter = std::abs(p.position.y - 34.0f);
            float distToEndline = std::abs(attackingGoalX - p.position.x);

            // Pressure check: defenders in own third release quickly under closing pressure
            bool inDefensiveThird = (p.team == TeamSide::Home && p.position.x < 36.0f) ||
                                    (p.team == TeamSide::Away && p.position.x > 69.0f);
            float pressureThreshold = inDefensiveThird ? 4.8f : 2.5f;

            bool underImmediatePressure = false;
            for (const auto& opp : m_players) {
                if (opp.team != p.team && (opp.position - p.position).length() < pressureThreshold) {
                    underImmediatePressure = true;
                    break;
                }
            }

            bool nearBoundary = (distToEndline < 3.5f || p.position.y < 3.0f || p.position.y > 65.0f);
            bool inPointBlankZone = (distToGoal < 9.0f && p.possessionDuration >= 0.20f);
            bool quickDefensiveRelease = (inDefensiveThird && p.possessionDuration >= 0.35f);
            bool canAct = (p.possessionDuration >= p.passHoldTime || underImmediatePressure || nearBoundary || inPointBlankZone || quickDefensiveRelease);

            if (canAct) {
                // Check if shooting lane to goal is blocked by defender
                bool laneBlocked = false;
                Vec2 toGoal = Vec2(attackingGoalX, 34.0f) - p.position;
                Vec2 shotDir = toGoal.normalized();
                for (const auto& opp : m_players) {
                    if (opp.team != p.team) {
                        Vec2 toOpp = opp.position - p.position;
                        float proj = toOpp.dot(shotDir);
                        if (proj > 0.5f && proj < 3.5f) {
                            Vec2 closePt = p.position + shotDir * proj;
                            if ((opp.position - closePt).length() < 1.3f) {
                                laneBlocked = true;
                                break;
                            }
                        }
                    }
                }

                // 1. Contextual Finishing / Shooting Decision: Calibrated to real match shot volume (~24-26 shots/90m)
                bool inShootingZone = (distToGoal < 13.0f) || (distToGoal < 22.0f && lateralDistToGoalCenter < 12.0f);
                float shootUrge = 0.0f;
                if (inShootingZone) {
                    if (distToGoal < 10.0f) {
                        shootUrge = laneBlocked ? 0.26f : 0.58f;
                    } else if (distToGoal < 17.0f) {
                        shootUrge = (p.role == TacticalRole::ST || p.role == TacticalRole::LW ||
                                     p.role == TacticalRole::RW || p.fcIqRole == FCIQRole::PoacherST) ?
                                    (laneBlocked ? 0.11f : 0.26f) :
                                    (laneBlocked ? 0.05f : 0.13f);
                    } else if (distToGoal < 22.0f) {
                        shootUrge = (p.role == TacticalRole::ST || p.role == TacticalRole::LW ||
                                     p.role == TacticalRole::RW || p.fcIqRole == FCIQRole::PlaymakerCAM) ?
                                    (laneBlocked ? 0.03f : 0.08f) : 0.018f;
                    }
                }

                if (shootUrge > 0.0f && m_randomEngine.chance(shootUrge * p.formFactor)) {
                    executeShot(p);
                    p.possessionDuration = 0.0f;
                    p.passHoldTime = m_randomEngine.randomizedHoldTime(0.75f);
                    continue;
                }

                // 2. Wide Crossing Option
                bool inCrossingZone = (distToGoal < 32.0f && (lateralDistToGoalCenter > 15.0f || distToEndline < 6.5f));
                if (inCrossingZone) {
                    float crossUrge = (p.role == TacticalRole::LW || p.role == TacticalRole::RW ||
                                       p.role == TacticalRole::LB || p.role == TacticalRole::RB) ? 0.60f : 0.25f;
                    if (m_randomEngine.chance(crossUrge)) {
                        executeCross(p);
                        p.possessionDuration = 0.0f;
                        p.passHoldTime = m_randomEngine.randomizedHoldTime(0.75f);
                        continue;
                    }
                }

                // 3. Tactical Ball Distribution / Passing
                executePass(p);
                p.possessionDuration = 0.0f;
                p.passHoldTime = m_randomEngine.randomizedHoldTime(0.75f);
            } else {
                executeDribble(p, dt);
            }
            continue;
        }

        // Defensive and off-ball assignment
        TeamTactics tactics = (p.team == TeamSide::Home) ? m_homeTactics : m_awayTactics;
        if (p.team != m_possessionTeam) {
            if (p.id == homeDesignatedPresser || p.id == awayDesignatedPresser) {
                p.state = PlayerState::PressingBall;
                p.behaviorState = BehaviorState::Press;
            } else {
                p.state = PlayerState::HoldingFormation;
                p.behaviorState = Tactics::evaluateRoleBehavior(p, m_ball.position, m_possessionTeam, m_ball.possessorId, tactics, 0.0f, 1);
            }
        } else {
            p.state = PlayerState::SupportingAttack;
            p.behaviorState = Tactics::evaluateRoleBehavior(p, m_ball.position, m_possessionTeam, m_ball.possessorId, tactics, 0.0f, 0);
        }
    }
}

void MatchEngine::updatePlayerKinematics(float dt) {
    float offsideLine = calculateOffsideLine(m_possessionTeam == TeamSide::Home ? TeamSide::Away : TeamSide::Home);

    for (auto& p : m_players) {
        if (p.isRedCarded) {
            p.velocity = {0.0f, 0.0f};
            p.position = Vec2(-200.0f, -200.0f);
            continue;
        }

        if (p.role == TacticalRole::GK) continue;

        Vec2 targetPos;

        if (p.hasPossession) {
            float targetX = (p.team == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M - 4.0f : 4.0f;
            targetPos = Vec2(targetX, p.homePosition.y);
        } else if (p.state == PlayerState::ReceivingPass) {
            targetPos = m_ball.position + (m_ball.velocity * 0.22f);
        } else if (p.state == PlayerState::PressingBall) {
            targetPos = m_ball.position;
        } else {
            TeamTacticalStyle style = (p.team == TeamSide::Home) ? m_homeTacticalStyle : m_awayTacticalStyle;
            DefensiveMentality mentality = (p.team == TeamSide::Home) ? m_homeMentality : m_awayMentality;
            FormationType formation = (p.team == TeamSide::Home) ? m_homeFormation : m_awayFormation;

            targetPos = Tactics::calculateOffTheBallPosition(p, m_ball.position, m_possessionTeam, m_ball.possessorId, style, mentality, formation, &m_players, offsideLine);
        }

        // Offside line discipline: held onside when off-ball attacking
        if (p.team == m_possessionTeam && !p.hasPossession && p.role != TacticalRole::GK) {
            if (p.team == TeamSide::Home && targetPos.x > offsideLine - 0.60f && offsideLine >= 52.5f) {
                targetPos.x = std::max(52.4f, offsideLine - 0.60f);
            } else if (p.team == TeamSide::Away && targetPos.x < offsideLine + 0.60f && offsideLine <= 52.5f) {
                targetPos.x = std::min(52.6f, offsideLine + 0.60f);
            }
        }

        p.targetSteeringPos = targetPos;

        Vec2 desiredOffset = targetPos - p.position;
        float distToTarget = desiredOffset.length();
        Vec2 desiredVelocity{0.0f, 0.0f};

        if (distToTarget > 0.35f) {
            float maxSpd = p.hasPossession ? SimConfig::PLAYER_DRIBBLE_SPEED_MPS :
                           (p.state == PlayerState::PressingBall || p.state == PlayerState::ReceivingPass)
                           ? SimConfig::PLAYER_MAX_SPRINT_MPS : SimConfig::PLAYER_JOG_SPEED_MPS;

            maxSpd *= (p.attributes.maxSpeed / 8.6f) * p.formFactor * p.fatigue;

            // Turn deceleration
            if (p.velocity.lengthSq() > 1.0f) {
                Vec2 currDir = p.velocity.normalized();
                Vec2 newDir = desiredOffset.normalized();
                float cosTheta = std::clamp(currDir.dot(newDir), -1.0f, 1.0f);
                float turnFactor = 0.65f + 0.35f * std::max(0.0f, cosTheta);
                maxSpd *= turnFactor;
            }

            desiredVelocity = desiredOffset.normalized() * std::min(maxSpd, distToTarget * 4.2f);
        }

        Vec2 velocityDiff = desiredVelocity - p.velocity;
        float maxAccel = (desiredVelocity.lengthSq() < p.velocity.lengthSq())
                         ? SimConfig::PLAYER_DECELERATION_MPS2
                         : SimConfig::PLAYER_ACCELERATION_MPS2;

        float accelMag = velocityDiff.length();
        if (accelMag > maxAccel * dt) {
            p.velocity += (velocityDiff / accelMag) * (maxAccel * dt);
        } else {
            p.velocity = desiredVelocity;
        }

        Vec2 oldPos = p.position;
        p.position += p.velocity * dt;

        p.position.x = std::clamp(p.position.x, 1.2f, SimConfig::PITCH_LENGTH_M - 1.2f);
        p.position.y = std::clamp(p.position.y, 1.2f, SimConfig::PITCH_WIDTH_M - 1.2f);

        float frameDist = (p.position - oldPos).length();
        p.distanceCoveredM += frameDist;

        float currentSpeed = p.velocity.length();
        if (currentSpeed > 5.5f) {
            p.sprintDistanceM += frameDist;
            p.stamina = std::max(40.0f, p.stamina - (dt * 1.0f));
        } else if (currentSpeed < 2.5f) {
            p.stamina = std::min(100.0f, p.stamina + (dt * 0.45f));
        }
        p.fatigue = 0.78f + (p.stamina / 100.0f) * 0.22f;

        // Dynamic authentic human body orientation (facing direction)
        float targetHeadingDeg = p.headingDeg;
        if (p.hasPossession || p.velocity.lengthSq() > 1.2f) {
            // Running or dribbling: face movement direction
            targetHeadingDeg = std::atan2(p.velocity.y, p.velocity.x) * (180.0f / 3.14159265f);
        } else {
            // Tactical positioning: ball-oriented body shape (face the ball)
            Vec2 toBall = m_ball.position - p.position;
            if (toBall.lengthSq() > 0.04f) {
                targetHeadingDeg = std::atan2(toBall.y, toBall.x) * (180.0f / 3.14159265f);
            }
        }
        float headingDiff = targetHeadingDeg - p.headingDeg;
        while (headingDiff > 180.0f) headingDiff -= 360.0f;
        while (headingDiff < -180.0f) headingDiff += 360.0f;
        float maxTurnDeg = 360.0f * dt;
        p.headingDeg += std::clamp(headingDiff, -maxTurnDeg, maxTurnDeg);
    }
}

void MatchEngine::handleBallPlayerCollisions() {
    if (m_phase == MatchPhase::Kickoff || m_phase == MatchPhase::GoalScored ||
        m_phase == MatchPhase::HalfTime || m_phase == MatchPhase::FullTime) return;

    bool isHighSpeedBall = (m_ball.velocity.lengthSq() > (14.0f * 14.0f));

    for (auto& p : m_players) {
        if (p.isRedCarded) continue;
        if (p.touchCooldown > 0.0f) continue;

        // Goalkeepers do not trap high-speed shots with feet (dive saves handled in updateGoalkeeperAI)
        if (p.role == TacticalRole::GK && isHighSpeedBall) {
            continue;
        }

        Vec2 toBall = m_ball.position - p.position;
        float dist = toBall.length();

        // 1. Controlling a loose pass or loose ball (must be below head height)
        if (m_ball.possessorId == -1 && dist < SimConfig::TOUCH_RADIUS_M && m_ball.height < 1.85f) {
            // High-speed shots / clearances: outfield defenders attempt a block in defensive zone
            if (isHighSpeedBall && p.team != m_ball.lastTouchedTeam) {
                p.touchCooldown = 0.5f;
                bool inDefensiveThird = (p.team == TeamSide::Home && p.position.x < 38.0f) ||
                                        (p.team == TeamSide::Away && p.position.x > 67.0f);
                if (inDefensiveThird) {
                    float blockChance = (p.attributes.defending / 100.0f) * 0.08f;
                    if (m_randomEngine.chance(blockChance)) {
                        std::cout << "[BLOCK!] " << p.name << " blocks the shot!\n";
                        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::TackleSuccess, p.team, p.id, -1, p.position, "Shot blocked by defender"});
                        Vec2 deflectDir = m_randomEngine.deviateDirection(-m_ball.velocity.normalized(), 60.0f);
                        m_ball.velocity = deflectDir * m_randomEngine.uniform(6.0f, 12.0f);
                        m_ball.isShot = false;
                        m_ball.lastTouchedPlayerId = p.id;
                        m_ball.lastTouchedTeam = p.team;
                    }
                }
                continue;
            } else if (isHighSpeedBall) {
                continue;
            }

            // OFFICIAL OFFSIDE ENFORCEMENT (Law 11): Active involvement by offside receiver
            if (m_ball.isOffsidePass && p.id == m_ball.intendedReceiverId) {
                std::cout << "[OFFSIDE!] Assistant referee flags offside against " << p.name << "!\n";
                m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PossessionTurnover, p.team, p.id, -1, p.position, "Offside offense (Law 11)"});
                m_ball.isOffsidePass = false;
                m_ball.possessorId = -1;
                executeFreeKick(p.team == TeamSide::Home ? TeamSide::Away : TeamSide::Home, p.position);
                return;
            }

            for (auto& other : m_players) other.hasPossession = false;
            p.hasPossession = true;
            p.possessionDuration = 0.0f;
            p.passHoldTime = m_randomEngine.randomizedHoldTime(0.85f);
            m_ball.possessorId = p.id;
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_ball.isOffsidePass = false;

            Vec2 touchJitter = m_randomEngine.touchScatter(p.attributes.passSkill * p.formFactor, m_ball.velocity.length());
            m_ball.position += touchJitter;

            if (m_ball.lastTouchedPlayerId != -1 && m_ball.lastTouchedTeam == p.team) {
                m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassCompleted, p.team, m_ball.lastTouchedPlayerId, p.id, p.position, "Pass received"});
                m_spatialAnalytics.recordPass(m_ball.lastTouchedPlayerId, p.id, p.team);
            } else if (m_ball.lastTouchedTeam != p.team) {
                m_eventRecorder.recordEvent({m_matchTime, MatchEventType::Interception, p.team, p.id, -1, p.position, "Ball intercepted"});
                float seqDuration = m_matchTime - m_lastPossessionChangeTime;
                m_analytics.recordPassSequenceDuration(seqDuration);
                m_analytics.recordRegain(p.team, seqDuration);
                m_analytics.recordTransition(p.behaviorState == BehaviorState::Press);
                m_lastPossessionChangeTime = m_matchTime;
                m_possessionChainTouches.fill(0);
            }

            if (p.id >= 0 && p.id < static_cast<int>(m_possessionChainTouches.size())) {
                m_possessionChainTouches[p.id]++;
            }

            m_ball.lastTouchedPlayerId = p.id;
            m_ball.lastTouchedTeam = p.team;
            m_possessionTeam = p.team;
            m_ball.intendedReceiverId = -1;
            break;
        }

        // 2. Tackling an opposing player holding the ball (Stochastic Multi-Outcome Duel)
        if (m_ball.possessorId != -1 && m_ball.possessorId != p.id && p.team != m_possessionTeam) {
            if (dist < SimConfig::TACKLE_DISTANCE_M) {
                Player* carrier = nullptr;
                for (auto& other : m_players) {
                    if (other.id == m_ball.possessorId) {
                        carrier = &other;
                        break;
                    }
                }

                if (carrier) {
                    int duelOutcome = m_randomEngine.resolveTackleDuel(
                        p.attributes.tackleSkill * p.formFactor,
                        carrier->attributes.passSkill * carrier->formFactor
                    );

                    if (duelOutcome == 0) {
                        // Clean win: defender dispossesses attacker
                        carrier->hasPossession = false;
                        carrier->touchCooldown = 0.7f;

                        p.hasPossession = true;
                        p.possessionDuration = 0.0f;
                        p.passHoldTime = m_randomEngine.randomizedHoldTime(0.85f);
                        m_ball.possessorId = p.id;
                        m_ball.isShot = false;
                        m_ball.isCross = false;
                        m_ball.isOffsidePass = false;
                        m_ball.lastTouchedPlayerId = p.id;
                        m_ball.lastTouchedTeam = p.team;
                        m_possessionTeam = p.team;
                        m_ball.intendedReceiverId = -1;
                        m_recentPasserId = -1;
                        m_possessionChainTouches.fill(0);
                        if (p.id >= 0 && p.id < static_cast<int>(m_possessionChainTouches.size())) {
                            m_possessionChainTouches[p.id] = 1;
                        }

                        float seqDuration = m_matchTime - m_lastPossessionChangeTime;
                        m_analytics.recordPassSequenceDuration(seqDuration);
                        m_analytics.recordRegain(p.team, seqDuration);
                        m_analytics.recordTransition(true);
                        m_lastPossessionChangeTime = m_matchTime;

                        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::TackleSuccess, p.team, p.id, -1, p.position, "Tackle won"});
                    } else if (duelOutcome == 1) {
                        // Loose ball deflection / 50-50 duel
                        carrier->hasPossession = false;
                        carrier->touchCooldown = 0.45f;
                        p.touchCooldown = 0.35f;

                        m_ball.possessorId = -1;
                        m_ball.intendedReceiverId = -1;
                        m_recentPasserId = -1;
                        m_possessionChainTouches.fill(0);
                        if (p.id >= 0 && p.id < static_cast<int>(m_possessionChainTouches.size())) {
                            m_possessionChainTouches[p.id] = 1;
                        }
                        m_ball.isShot = false;
                        m_ball.isCross = false;
                        m_ball.isOffsidePass = false;
                        Vec2 deflectDir = m_randomEngine.deviateDirection(
                            (p.position - carrier->position).normalized(),
                            55.0f
                        );
                        m_ball.velocity = deflectDir * m_randomEngine.uniform(6.0f, 13.0f);
                        m_ball.lastTouchedPlayerId = p.id;
                        m_ball.lastTouchedTeam = p.team;

                        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::TackleSuccess, p.team, p.id, -1, p.position, "Tackle deflection"});
                    } else if (duelOutcome == 3) {
                        // Law 12: Foul committed by tackler!
                        carrier->hasPossession = false;
                        carrier->touchCooldown = 0.6f;
                        carrier->velocity = {0.0f, 0.0f};

                        p.touchCooldown = 1.0f;
                        p.velocity = {0.0f, 0.0f};

                        m_ball.possessorId = -1;
                        m_ball.intendedReceiverId = -1;
                        m_ball.velocity = {0.0f, 0.0f};
                        m_ball.height = 0.0f;
                        m_ball.verticalVelocity = 0.0f;

                        Vec2 foulSpot = carrier->position;
                        m_ball.position = foulSpot;

                        TeamSide fouledTeam = carrier->team;
                        TeamSide offendingTeam = p.team;

                        // Check penalty area:
                        // Left goal (Home defended): x in [0.0, 16.5], y in [13.84, 54.16]
                        // Right goal (Away defended): x in [88.5, 105.0], y in [13.84, 54.16]
                        bool isPenalty = false;
                        if (offendingTeam == TeamSide::Home && foulSpot.x <= 16.5f && foulSpot.y >= 13.84f && foulSpot.y <= 54.16f) {
                            isPenalty = true;
                        } else if (offendingTeam == TeamSide::Away && foulSpot.x >= 88.5f && foulSpot.y >= 13.84f && foulSpot.y <= 54.16f) {
                            isPenalty = true;
                        }

                        // Evaluate Disciplinary Booking (Yellow / Red Card)
                        float cardRoll = m_randomEngine.uniform(0.0f, 1.0f);
                        float distToDefendedGoal = (offendingTeam == TeamSide::Home) ? foulSpot.x : (SimConfig::PITCH_LENGTH_M - foulSpot.x);
                        bool isDOGSO = (distToDefendedGoal < 22.0f && std::abs(foulSpot.y - 34.0f) < 14.0f && m_randomEngine.chance(0.06f));

                        if (isDOGSO || cardRoll < 0.015f) {
                            // Straight Red Card (DOGSO / Dangerous Tackle)
                            p.isRedCarded = true;
                            p.hasPossession = false;
                            p.position = Vec2(-200.0f, -200.0f);
                            p.velocity = {0.0f, 0.0f};
                            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::RedCard, offendingTeam, p.id, carrier->id, foulSpot, "Straight red card (DOGSO / reckless tackle)"});
                            std::cout << "[RED CARD!] " << p.name << " is shown a STRAIGHT RED CARD and sent off! "
                                      << (offendingTeam == TeamSide::Home ? "Home" : "Away") << " down to 10 players!\n";
                        } else if (cardRoll < 0.22f) {
                            // Yellow Card Caution
                            p.yellowCards++;
                            if (p.yellowCards >= 2) {
                                // Second Yellow -> Red Card
                                p.isRedCarded = true;
                                p.hasPossession = false;
                                p.position = Vec2(-200.0f, -200.0f);
                                p.velocity = {0.0f, 0.0f};
                                m_eventRecorder.recordEvent({m_matchTime, MatchEventType::RedCard, offendingTeam, p.id, carrier->id, foulSpot, "Second yellow card -> RED CARD ejection"});
                                std::cout << "[RED CARD!] " << p.name << " receives a second yellow card and is SENT OFF!\n";
                            } else {
                                m_eventRecorder.recordEvent({m_matchTime, MatchEventType::YellowCard, offendingTeam, p.id, carrier->id, foulSpot, "Yellow card caution"});
                                std::cout << "[YELLOW CARD!] Referee books " << p.name << " for a reckless foul!\n";
                            }
                        } else {
                            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::FoulCommitted, offendingTeam, p.id, carrier->id, foulSpot, "Foul committed"});
                            std::cout << "[FOUL!] " << p.name << " fouls " << carrier->name << "!\n";
                        }

                        if (isPenalty) {
                            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PenaltyAwarded, fouledTeam, carrier->id, p.id, foulSpot, "Penalty kick awarded"});
                            std::cout << "[PENALTY!] Foul inside the box! Referee points to the penalty spot!\n";
                            executePenaltyKick(fouledTeam);
                        } else {
                            executeFreeKick(fouledTeam, foulSpot);
                        }
                        return;
                    } else {
                        // Carrier retains possession / dribbles past tackler
                        p.touchCooldown = 0.6f;
                    }
                }
                break;
            }
        }
    }
}

void MatchEngine::checkPitchBoundariesAndGoals() {
    float goalTop = (SimConfig::PITCH_WIDTH_M - SimConfig::GOAL_WIDTH_M) * 0.5f;
    float goalBottom = goalTop + SimConfig::GOAL_WIDTH_M;

    // 1. Left Goal (Away scores against Home)
    if (m_ball.position.x <= 0.0f) {
        if (m_ball.position.y >= goalTop && m_ball.position.y <= goalBottom && m_ball.height <= SimConfig::GOAL_HEIGHT_M) {
            bool hitPost = (std::abs(m_ball.position.y - goalTop) < 0.25f) || (std::abs(m_ball.position.y - goalBottom) < 0.25f);
            bool hitBar = std::abs(m_ball.height - SimConfig::GOAL_HEIGHT_M) < 0.25f;
            if (m_ball.isShot && (hitPost || hitBar)) {
                std::cout << "[WOODWORK!] Away shot strikes the post!\n";
                m_ball.position.x = 0.6f;
                m_ball.velocity.x = std::abs(m_ball.velocity.x) * 0.70f;
                m_ball.velocity.y = m_randomEngine.uniform(-6.0f, 6.0f);
                m_ball.height = 0.4f;
                m_ball.verticalVelocity = m_randomEngine.uniform(1.0f, 2.8f);
                m_ball.isShot = false;
                m_ball.isCross = false;
                return;
            }

            m_awayScore++;
            m_analytics.recordGoal(m_phase == MatchPhase::SetPiece);
            m_phase = MatchPhase::GoalScored;
            m_phaseTimer = 2.2f;
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::Goal, TeamSide::Away, m_ball.lastTouchedPlayerId, -1, m_ball.position, "Goal for Away Team"});
            std::cout << "[GOAL!] Away scores! (" << m_homeScore << " - " << m_awayScore << ")\n";
            return;
        } else {
            // Ball crossed left endline outside goal
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_ball.isOffsidePass = false;
            if (m_ball.lastTouchedTeam == TeamSide::Home) {
                executeCorner(TeamSide::Away);
            } else {
                Player* kicker = nullptr;
                if (m_randomEngine.chance(0.50f)) {
                    for (auto& p : m_players) if (p.team == TeamSide::Home && p.role == TacticalRole::GK) { kicker = &p; break; }
                } else {
                    for (auto& p : m_players) if (p.team == TeamSide::Home && (p.role == TacticalRole::CB_LEFT || p.role == TacticalRole::CB_RIGHT)) { kicker = &p; break; }
                }
                if (!kicker) { for (auto& p : m_players) if (p.team == TeamSide::Home) { kicker = &p; break; } }
                if (kicker) executeGoalKick(*kicker);
            }
            return;
        }
    }

    // 2. Right Goal (Home scores against Away)
    if (m_ball.position.x >= SimConfig::PITCH_LENGTH_M) {
        if (m_ball.position.y >= goalTop && m_ball.position.y <= goalBottom && m_ball.height <= SimConfig::GOAL_HEIGHT_M) {
            bool hitPost = (std::abs(m_ball.position.y - goalTop) < 0.25f) || (std::abs(m_ball.position.y - goalBottom) < 0.25f);
            bool hitBar = std::abs(m_ball.height - SimConfig::GOAL_HEIGHT_M) < 0.25f;
            if (m_ball.isShot && (hitPost || hitBar)) {
                std::cout << "[WOODWORK!] Home shot strikes the post!\n";
                m_ball.position.x = SimConfig::PITCH_LENGTH_M - 0.6f;
                m_ball.velocity.x = -std::abs(m_ball.velocity.x) * 0.70f;
                m_ball.velocity.y = m_randomEngine.uniform(-6.0f, 6.0f);
                m_ball.height = 0.4f;
                m_ball.verticalVelocity = m_randomEngine.uniform(1.0f, 2.8f);
                m_ball.isShot = false;
                m_ball.isCross = false;
                return;
            }

            m_homeScore++;
            m_analytics.recordGoal(m_phase == MatchPhase::SetPiece);
            m_phase = MatchPhase::GoalScored;
            m_phaseTimer = 2.2f;
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::Goal, TeamSide::Home, m_ball.lastTouchedPlayerId, -1, m_ball.position, "Goal for Home Team"});
            std::cout << "[GOAL!] Home scores! (" << m_homeScore << " - " << m_awayScore << ")\n";
            return;
        } else {
            // Ball crossed right endline outside goal
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_ball.isOffsidePass = false;
            if (m_ball.lastTouchedTeam == TeamSide::Away) {
                executeCorner(TeamSide::Home);
            } else {
                Player* kicker = nullptr;
                if (m_randomEngine.chance(0.50f)) {
                    for (auto& p : m_players) if (p.team == TeamSide::Away && p.role == TacticalRole::GK) { kicker = &p; break; }
                } else {
                    for (auto& p : m_players) if (p.team == TeamSide::Away && (p.role == TacticalRole::CB_LEFT || p.role == TacticalRole::CB_RIGHT)) { kicker = &p; break; }
                }
                if (!kicker) { for (auto& p : m_players) if (p.team == TeamSide::Away) { kicker = &p; break; } }
                if (kicker) executeGoalKick(*kicker);
            }
            return;
        }
    }

    // 3. Official Touchlines (Law 15: Throw-in)
    if (m_ball.position.y <= 0.0f || m_ball.position.y >= SimConfig::PITCH_WIDTH_M) {
        m_ball.isShot = false;
        m_ball.isCross = false;
        m_ball.isOffsidePass = false;
        float touchY = (m_ball.position.y <= 0.0f) ? 0.0f : SimConfig::PITCH_WIDTH_M;
        float touchX = std::clamp(m_ball.position.x, 2.0f, SimConfig::PITCH_LENGTH_M - 2.0f);
        TeamSide awardedTeam = (m_ball.lastTouchedTeam == TeamSide::Home) ? TeamSide::Away : TeamSide::Home;
        executeThrowIn(awardedTeam, Vec2(touchX, touchY));
    }
}

void MatchEngine::executeKickoff(Player& player) {
    // In realistic football, kickoffs start by passing backwards into own half to initiate patient build-up
    Player* chosenReceiver = nullptr;
    std::vector<Player*> backOptions;
    std::vector<float> weights;

    for (auto& tm : m_players) {
        if (tm.team == player.team && tm.id != player.id) {
            if (tm.role == TacticalRole::CB_LEFT || tm.role == TacticalRole::CB_RIGHT) {
                backOptions.push_back(&tm);
                weights.push_back(3.5f); // High preference to CBs for patient possession build-up
            } else if (tm.role == TacticalRole::DM) {
                backOptions.push_back(&tm);
                weights.push_back(3.0f); // Pivot
            } else if (tm.role == TacticalRole::CM_LEFT || tm.role == TacticalRole::CM_RIGHT) {
                backOptions.push_back(&tm);
                weights.push_back(2.0f); // Central midfielder
            }
        }
    }

    if (!backOptions.empty()) {
        int idx = m_randomEngine.selectWeightedOption(weights);
        if (idx >= 0 && idx < static_cast<int>(backOptions.size())) {
            chosenReceiver = backOptions[idx];
        } else {
            chosenReceiver = backOptions[0];
        }
    }

    if (chosenReceiver) {
        Vec2 passDir = (chosenReceiver->position - player.position).normalized();
        player.hasPossession = false;
        player.touchCooldown = 0.8f;
        m_ball.possessorId = -1;
        m_ball.intendedReceiverId = chosenReceiver->id;
        m_ball.isShot = false;
        m_ball.isCross = false;
        m_ball.isOffsidePass = false;
        m_ball.velocity = passDir * (SimConfig::SHORT_PASS_SPEED_MPS * 0.90f);
        m_ball.lastTouchedPlayerId = player.id;
        m_ball.lastTouchedTeam = player.team;
        m_recentPasserId = player.id;
        m_possessionChainTouches.fill(0);
        if (player.id >= 0 && player.id < static_cast<int>(m_possessionChainTouches.size())) {
            m_possessionChainTouches[player.id] = 1;
        }

        m_phase = MatchPhase::BuildUp;
        m_phaseTimer = 5.5f;

        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassAttempt, player.team, player.id, chosenReceiver->id, player.position, "Kickoff build-up pass"});
        return;
    }

    player.hasPossession = false;
    m_ball.possessorId = -1;
    m_ball.velocity = (player.team == TeamSide::Home ? Vec2(-1.0f, 0.0f) : Vec2(1.0f, 0.0f)) * 12.0f;
    m_phase = MatchPhase::BuildUp;
    m_phaseTimer = 5.0f;
}

void MatchEngine::executeDribble(Player& player, float /*dt*/) {
    float attackingDir = (player.team == TeamSide::Home) ? 1.0f : -1.0f;
    float attackingGoalX = (player.team == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
    float distToGoalX = std::abs(attackingGoalX - player.position.x);

    // Boundary proximity check: never walk into boundary walls!
    float distToEndline = std::abs(attackingGoalX - player.position.x);
    float distToTouchline = std::min(player.position.y, SimConfig::PITCH_WIDTH_M - player.position.y);

    if (distToEndline < 3.5f || distToTouchline < 3.0f) {
        Vec2 centerDir = (Vec2(player.position.x, 34.0f) - player.position).normalized();
        player.velocity = centerDir * (SimConfig::PLAYER_DRIBBLE_SPEED_MPS * 0.7f);
        executePass(player);
        return;
    }

    float targetY = player.homePosition.y;
    if (distToGoalX < 24.0f) {
        targetY = 34.0f; // Cut inside towards goal
    }

    Vec2 targetPos(player.position.x + (attackingDir * 6.0f), targetY);
    Vec2 dribbleDir = (targetPos - player.position).normalized();
    float dribbleSpeed = SimConfig::PLAYER_DRIBBLE_SPEED_MPS * (player.attributes.maxSpeed / 8.6f) * player.formFactor * player.fatigue;
    player.velocity = dribbleDir * dribbleSpeed;
}

void MatchEngine::executeCross(Player& crosser) {
    float penaltyX = (crosser.team == TeamSide::Home) ? 94.0f : 11.0f;
    Vec2 boxTarget(penaltyX, 34.0f + m_randomEngine.gaussian(0.0f, 2.5f));

    Player* bestRunner = nullptr;
    float closestDist = 1e9f;
    for (auto& tm : m_players) {
        if (tm.team == crosser.team && tm.id != crosser.id && tm.role != TacticalRole::GK) {
            float d = (tm.position - boxTarget).length();
            if (d < closestDist) {
                closestDist = d;
                bestRunner = &tm;
            }
        }
    }

    Vec2 toTarget = boxTarget - crosser.position;
    float passSpeed = m_randomEngine.passSpeedVariance(19.0f);
    Vec2 idealDir = toTarget.normalized();
    float devDeg = m_randomEngine.passDeviationDeg(crosser.attributes.crossing * crosser.formFactor);
    Vec2 crossDir = m_randomEngine.deviateDirection(idealDir, devDeg);

    crosser.hasPossession = false;
    crosser.touchCooldown = 0.8f;
    m_ball.possessorId = -1;
    m_ball.intendedReceiverId = bestRunner ? bestRunner->id : -1;
    m_ball.isShot = false;
    m_ball.isCross = true;
    m_ball.isOffsidePass = false;
    m_ball.velocity = crossDir * passSpeed;
    m_ball.height = 0.25f;
    m_ball.verticalVelocity = m_randomEngine.uniform(3.2f, 4.4f);
    m_ball.lastTouchedPlayerId = crosser.id;
    m_ball.lastTouchedTeam = crosser.team;
    m_phase = MatchPhase::InPlay;

    m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassAttempt, crosser.team, crosser.id, bestRunner ? bestRunner->id : -1, crosser.position, "Curled cross into box"});
}

void MatchEngine::executeCorner(TeamSide attackingTeam) {
    m_phase = MatchPhase::Corner;
    m_possessionTeam = attackingTeam;

    float cornerX = (attackingTeam == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
    float cornerY = (m_ball.position.y < 34.0f) ? 0.5f : SimConfig::PITCH_WIDTH_M - 0.5f;
    Vec2 cornerSpot(cornerX, cornerY);

    float boxTargetX = (attackingTeam == TeamSide::Home) ? 96.0f : 9.0f;
    Vec2 boxTarget(boxTargetX, 34.0f + m_randomEngine.uniform(-3.5f, 3.5f));

    Player* taker = nullptr;
    for (auto& p : m_players) {
        if (p.isRedCarded) continue;
        if (p.team == attackingTeam && (p.role == TacticalRole::RW || p.role == TacticalRole::LW || p.role == TacticalRole::RB || p.role == TacticalRole::LB)) {
            taker = &p;
            break;
        }
    }
    if (!taker) {
        for (auto& p : m_players) if (p.team == attackingTeam && !p.isRedCarded && p.role != TacticalRole::GK) { taker = &p; break; }
    }

    if (taker) {
        taker->position = cornerSpot;
        taker->velocity = {0.0f, 0.0f};

        // Authentic Corner Cluster: Push attackers and markers into danger area
        for (auto& tm : m_players) {
            if (tm.isRedCarded || tm.id == taker->id) continue;
            if (tm.team == attackingTeam && (tm.role == TacticalRole::ST || tm.role == TacticalRole::CB_LEFT || tm.role == TacticalRole::CB_RIGHT || tm.role == TacticalRole::CM_LEFT || tm.role == TacticalRole::CM_RIGHT)) {
                float targetOffX = m_randomEngine.uniform(-2.5f, 2.5f);
                float targetOffY = m_randomEngine.uniform(-5.0f, 5.0f);
                tm.position = Vec2(boxTargetX + targetOffX, 34.0f + targetOffY);
                tm.velocity = {0.0f, 0.0f};
                tm.hasPossession = false;
            } else if (tm.team != attackingTeam && tm.role != TacticalRole::GK) {
                float defOffX = m_randomEngine.uniform(-2.0f, 2.0f);
                float defOffY = m_randomEngine.uniform(-5.0f, 5.0f);
                tm.position = Vec2(boxTargetX + defOffX, 34.0f + defOffY);
                tm.velocity = {0.0f, 0.0f};
                tm.hasPossession = false;
            }
        }

        Player* runner = nullptr;
        for (auto& tm : m_players) {
            if (tm.team == attackingTeam && !tm.isRedCarded && tm.id != taker->id && (tm.role == TacticalRole::ST || tm.role == TacticalRole::CB_LEFT || tm.role == TacticalRole::CB_RIGHT)) {
                runner = &tm;
                break;
            }
        }

        Vec2 toTarget = boxTarget - cornerSpot;
        Vec2 cornerDir = toTarget.normalized();
        float passSpeed = m_randomEngine.uniform(18.0f, 22.0f);

        taker->hasPossession = false;
        taker->touchCooldown = 0.8f;
        m_ball.position = cornerSpot;
        m_ball.possessorId = -1;
        m_ball.intendedReceiverId = runner ? runner->id : -1;
        m_ball.isShot = false;
        m_ball.isCross = true;
        m_ball.isOffsidePass = false; // Law 11: No offside from corner kick!
        m_ball.velocity = cornerDir * passSpeed;
        m_ball.height = 0.35f;
        m_ball.verticalVelocity = m_randomEngine.uniform(3.8f, 5.0f);
        m_ball.lastTouchedPlayerId = taker->id;
        m_ball.lastTouchedTeam = attackingTeam;

        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::SetPieceCorner, attackingTeam, taker->id, -1, cornerSpot, "Corner kick set-piece"});
        std::cout << "[CORNER!] " << (attackingTeam == TeamSide::Home ? "Home" : "Away") << " swings corner kick into box!\n";
    }

    m_phase = MatchPhase::InPlay;
}

void MatchEngine::executeGoalKick(Player& kicker) {
    m_phase = MatchPhase::GoalKick;
    m_possessionTeam = kicker.team;

    bool isGK = (kicker.role == TacticalRole::GK);
    m_analytics.recordGoalKick(isGK);

    float goalX = (kicker.team == TeamSide::Home) ? 5.5f : SimConfig::PITCH_LENGTH_M - 5.5f;
    Vec2 kickSpot(goalX, 34.0f);

    kicker.position = kickSpot;
    kicker.velocity = {0.0f, 0.0f};
    m_ball.position = kickSpot;
    m_ball.velocity = {0.0f, 0.0f};
    m_ball.height = 0.0f;
    m_ball.isShot = false;
    m_ball.isCross = false;
    m_ball.isOffsidePass = false;
    m_recentPasserId = -1;
    m_possessionChainTouches.fill(0);
    kicker.hasPossession = true;
    m_ball.possessorId = kicker.id;
    m_ball.lastTouchedPlayerId = kicker.id;
    m_ball.lastTouchedTeam = kicker.team;

    executeGKDistribution(kicker);
    m_phase = MatchPhase::InPlay;
}

void MatchEngine::executeThrowIn(TeamSide awardedTeam, const Vec2& touchlineSpot) {
    m_possessionTeam = awardedTeam;
    m_phase = MatchPhase::ThrowIn;

    Player* thrower = nullptr;
    float minDist = 1e9f;
    for (auto& p : m_players) {
        if (p.team == awardedTeam && p.role != TacticalRole::GK) {
            float d = (p.position - touchlineSpot).length();
            if (d < minDist) {
                minDist = d;
                thrower = &p;
            }
        }
    }

    if (!thrower) {
        for (auto& p : m_players) if (p.team == awardedTeam) { thrower = &p; break; }
    }

    if (thrower) {
        thrower->position = touchlineSpot;
        thrower->velocity = {0.0f, 0.0f};

        Player* receiver = nullptr;
        float bestScore = -1e9f;
        float attackingDir = (awardedTeam == TeamSide::Home) ? 1.0f : -1.0f;

        for (auto& tm : m_players) {
            if (tm.team == awardedTeam && tm.id != thrower->id && tm.role != TacticalRole::GK) {
                Vec2 toTm = tm.position - touchlineSpot;
                float d = toTm.length();
                if (d >= 4.0f && d <= 22.0f) {
                    float score = 20.0f - d + (toTm.x * attackingDir * 0.5f);
                    if (score > bestScore) {
                        bestScore = score;
                        receiver = &tm;
                    }
                }
            }
        }

        if (!receiver) {
            for (auto& tm : m_players) if (tm.team == awardedTeam && tm.id != thrower->id) { receiver = &tm; break; }
        }

        if (receiver) {
            Vec2 toRec = receiver->position - thrower->position;
            Vec2 throwDir = toRec.normalized();
            float throwDist = toRec.length();

            thrower->hasPossession = false;
            thrower->touchCooldown = 0.8f;
            m_ball.position = touchlineSpot;
            m_ball.possessorId = -1;
            m_ball.intendedReceiverId = receiver->id;
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_ball.isOffsidePass = false; // Law 11: No offside from throw-in!
            m_ball.velocity = throwDir * std::clamp(throwDist * 1.6f, 8.0f, 14.0f);
            m_ball.height = 0.15f;
            m_ball.verticalVelocity = 1.0f;
            m_ball.lastTouchedPlayerId = thrower->id;
            m_ball.lastTouchedTeam = awardedTeam;

            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassAttempt, awardedTeam, thrower->id, receiver->id, touchlineSpot, "Throw-in delivery"});
        }
    }

    m_phase = MatchPhase::InPlay;
}

void MatchEngine::executeFreeKick(TeamSide awardedTeam, const Vec2& spot) {
    m_possessionTeam = awardedTeam;
    m_phase = MatchPhase::FreeKick;

    Player* kicker = nullptr;
    float minDist = 1e9f;
    for (auto& p : m_players) {
        if (p.isRedCarded) continue;
        if (p.team == awardedTeam) {
            float d = (p.position - spot).length();
            if (d < minDist) {
                minDist = d;
                kicker = &p;
            }
        }
    }

    if (kicker) {
        kicker->position = spot;
        kicker->velocity = {0.0f, 0.0f};

        float goalX = (awardedTeam == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
        float distToGoal = std::abs(goalX - spot.x);

        // Dangerous direct shooting or crossing range in final third (Law 13)
        if (distToGoal < 28.0f && std::abs(spot.y - 34.0f) < 16.0f) {
            if (m_randomEngine.chance(0.45f)) {
                std::cout << "[DIRECT FREE KICK!] " << kicker->name << " curls a direct free kick over the wall towards goal!\n";
                executeShot(*kicker);
                m_phase = MatchPhase::InPlay;
                return;
            } else {
                std::cout << "[SET PIECE CROSS!] " << kicker->name << " whips free kick into the penalty area!\n";
                executeCross(*kicker);
                m_phase = MatchPhase::InPlay;
                return;
            }
        }

        Player* receiver = nullptr;
        float bestDist = 1e9f;
        for (auto& tm : m_players) {
            if (tm.isRedCarded) continue;
            if (tm.team == awardedTeam && tm.id != kicker->id) {
                float d = (tm.position - spot).length();
                if (d >= 5.0f && d < bestDist) {
                    bestDist = d;
                    receiver = &tm;
                }
            }
        }

        if (receiver) {
            Vec2 toRec = receiver->position - kicker->position;
            Vec2 kickDir = toRec.normalized();
            float kickDist = toRec.length();

            kicker->hasPossession = false;
            kicker->touchCooldown = 0.8f;
            m_ball.position = spot;
            m_ball.possessorId = -1;
            m_ball.intendedReceiverId = receiver->id;
            m_ball.isShot = false;
            m_ball.isCross = false;
            m_ball.isOffsidePass = false;
            m_ball.velocity = kickDir * std::clamp(kickDist * 1.5f, 9.0f, 15.0f);
            m_ball.height = 0.0f;
            m_ball.verticalVelocity = 0.0f;
            m_ball.lastTouchedPlayerId = kicker->id;
            m_ball.lastTouchedTeam = awardedTeam;

            m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassAttempt, awardedTeam, kicker->id, receiver->id, spot, "Free kick restart"});
        }
    }

    m_phase = MatchPhase::InPlay;
}

void MatchEngine::executePenaltyKick(TeamSide attackingTeam) {
    m_phase = MatchPhase::PenaltyKick;
    m_possessionTeam = attackingTeam;

    float spotX = (attackingTeam == TeamSide::Home) ? (SimConfig::PITCH_LENGTH_M - 11.0f) : 11.0f;
    Vec2 penaltySpot(spotX, 34.0f);

    // 1. Select best penalty taker (highest finishing + composure)
    Player* taker = nullptr;
    float bestScore = -1.0f;
    for (auto& p : m_players) {
        if (p.team == attackingTeam && !p.isRedCarded && p.role != TacticalRole::GK) {
            float s = (p.attributes.finishing * 0.60f) + (p.attributes.composure * 0.40f);
            if (s > bestScore) {
                bestScore = s;
                taker = &p;
            }
        }
    }
    if (!taker) {
        for (auto& p : m_players) if (p.team == attackingTeam && !p.isRedCarded) { taker = &p; break; }
    }

    // 2. Position goalkeeper on goal line
    float goalLineX = (attackingTeam == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
    Player* gk = nullptr;
    for (auto& p : m_players) {
        if (p.team != attackingTeam && p.role == TacticalRole::GK) {
            gk = &p;
            p.position = Vec2(goalLineX, 34.0f);
            p.velocity = {0.0f, 0.0f};
            p.touchCooldown = 0.0f;
            break;
        }
    }

    // 3. Keep all other outfield players outside penalty area & penalty D arc
    for (auto& p : m_players) {
        if (p.isRedCarded) continue;
        if (p.role == TacticalRole::GK) continue;
        if (taker && p.id == taker->id) continue;

        if (attackingTeam == TeamSide::Home) {
            if (p.position.x > 86.0f) {
                p.position.x = m_randomEngine.uniform(76.0f, 85.0f);
            }
        } else {
            if (p.position.x < 19.0f) {
                p.position.x = m_randomEngine.uniform(20.0f, 29.0f);
            }
        }
        p.velocity = {0.0f, 0.0f};
        p.hasPossession = false;
    }

    if (taker) {
        taker->position = penaltySpot + ((attackingTeam == TeamSide::Home) ? Vec2(-1.0f, 0.0f) : Vec2(1.0f, 0.0f));
        taker->velocity = {0.0f, 0.0f};

        // Real penalty conversion: ~76% goal, ~18% saved, ~6% off-target/woodwork
        float takerSkill = (taker->attributes.finishing * 0.55f) + (taker->attributes.shooting / 100.0f * 0.45f);
        float gkSkill = gk ? ((gk->attributes.gkReflexes * 0.50f) + (gk->attributes.gkDiving * 0.50f)) : 0.85f;
        float goalProb = std::clamp(0.76f + (takerSkill - gkSkill) * 0.15f, 0.65f, 0.85f);

        float roll = m_randomEngine.uniform(0.0f, 1.0f);
        float targetY = 34.0f;
        ShotOutcome outcome = ShotOutcome::Goal;

        if (roll < goalProb) {
            outcome = ShotOutcome::Goal;
            float cornerSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
            targetY = 34.0f + (cornerSide * m_randomEngine.uniform(2.4f, 3.2f));
        } else if (roll < goalProb + 0.18f) {
            outcome = ShotOutcome::Save;
            float gkSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
            targetY = 34.0f + (gkSide * m_randomEngine.uniform(1.5f, 2.8f));
        } else {
            outcome = m_randomEngine.chance(0.5f) ? ShotOutcome::Woodwork : ShotOutcome::OffTarget;
            float missSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
            targetY = 34.0f + (missSide * (outcome == ShotOutcome::Woodwork ? 3.66f : 5.2f));
        }

        Vec2 shotTarget(goalLineX, targetY);
        Vec2 shotDir = (shotTarget - penaltySpot).normalized();
        float shotSpeed = m_randomEngine.uniform(24.0f, 28.0f);

        m_ball.position = penaltySpot;
        m_ball.possessorId = -1;
        m_ball.intendedReceiverId = -1;
        m_ball.isShot = true;
        m_ball.isCross = false;
        m_ball.isOffsidePass = false;
        m_ball.shotOutcome = outcome;
        m_ball.velocity = shotDir * shotSpeed;
        m_ball.height = 0.2f;
        m_ball.verticalVelocity = m_randomEngine.uniform(0.5f, 2.0f);
        m_ball.lastTouchedPlayerId = taker->id;
        m_ball.lastTouchedTeam = attackingTeam;

        taker->hasPossession = false;
        taker->touchCooldown = 1.2f;

        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::ShotOnTarget, attackingTeam, taker->id, -1, penaltySpot, "Penalty kick taken"});
        std::cout << "[PENALTY KICK!] " << taker->name << " steps up and strikes the penalty kick!\n";
    }

    m_phase = MatchPhase::InPlay;
}

void MatchEngine::executeGKDistribution(Player& gk) {
    int bestId = -1;
    float attackingDir = (gk.team == TeamSide::Home) ? 1.0f : -1.0f;

    struct GKCandidate {
        int id;
        float score;
    };
    std::vector<GKCandidate> candidates;

    for (const auto& tm : m_players) {
        if (tm.team != gk.team || tm.id == gk.id || tm.role == TacticalRole::GK) continue;

        // GK build-up distribution targets defenders and central/wide midfielders
        float rolePref = 0.0f;
        if (tm.role == TacticalRole::CB_LEFT || tm.role == TacticalRole::CB_RIGHT) rolePref = 10.0f;
        else if (tm.role == TacticalRole::LB || tm.role == TacticalRole::RB)       rolePref = 10.0f;
        else if (tm.role == TacticalRole::DM)                                       rolePref = 9.0f;
        else if (tm.role == TacticalRole::CM_LEFT || tm.role == TacticalRole::CM_RIGHT) rolePref = 8.0f;
        else continue; // Never distribute directly to strikers or wingers in build-up!

        Vec2 toTm = tm.position - gk.position;
        float dist = toTm.length();
        if (dist < 4.5f || dist > 45.0f) continue;

        float forwardness = toTm.x * attackingDir;
        float distPenalty = std::max(0.0f, dist - 22.0f) * 0.25f;
        float score = rolePref + (forwardness * 0.20f) - distPenalty;

        // Discourage dangerous short balls (< 6.5m)
        if (dist < 6.5f) {
            score -= 8.0f;
        }

        // Check if receiver is marked by an opponent
        float minOppDistToTm = 100.0f;
        for (const auto& opp : m_players) {
            if (opp.team != gk.team) {
                float oppDist = (opp.position - tm.position).length();
                if (oppDist < minOppDistToTm) minOppDistToTm = oppDist;
            }
        }
        if (minOppDistToTm < 3.5f) {
            score -= 20.0f; // Marked teammate
        }

        Vec2 passDir = toTm.normalized();
        for (const auto& opp : m_players) {
            if (opp.team != gk.team) {
                Vec2 toOpp = opp.position - gk.position;
                float proj = toOpp.dot(passDir);
                if (proj > 1.5f && proj < dist - 1.5f) {
                    Vec2 closestPt = gk.position + passDir * proj;
                    if ((opp.position - closestPt).length() < 2.0f) {
                        score -= 20.0f;
                        break;
                    }
                }
            }
        }

        if (score > 0.0f) {
            candidates.push_back({tm.id, score});
        }
    }

    if (!candidates.empty()) {
        std::sort(candidates.begin(), candidates.end(), [](const GKCandidate& a, const GKCandidate& b) {
            return a.score > b.score;
        });
        float maxScore = candidates[0].score;
        std::vector<float> weights;
        for (const auto& c : candidates) {
            weights.push_back(std::exp((c.score - maxScore) * 0.35f));
        }
        int chosenIdx = m_randomEngine.selectWeightedOption(weights);
        if (chosenIdx >= 0 && chosenIdx < static_cast<int>(candidates.size())) {
            bestId = candidates[chosenIdx].id;
        } else {
            bestId = candidates[0].id;
        }
    }

    if (bestId == -1) {
        // Fallback: Safe long distribution upfield to forwards or wingers to bypass press
        for (const auto& tm : m_players) {
            if (tm.team == gk.team && (tm.role == TacticalRole::LW || tm.role == TacticalRole::RW || tm.role == TacticalRole::ST)) {
                bestId = tm.id;
                break;
            }
        }
    }

    if (bestId == -1) {
        m_ball.possessorId = -1;
        m_ball.velocity = Vec2(attackingDir * SimConfig::LONG_PASS_SPEED_MPS, 0.0f);
        m_ball.lastTouchedPlayerId = gk.id;
        m_ball.lastTouchedTeam = gk.team;
        m_possessionTeam = gk.team;
        std::cout << "[GK Distribution] " << gk.name << " punts upfield (no open target)\n";
        return;
    }

    for (const auto& tm : m_players) {
        if (tm.id != bestId) continue;
        Vec2 toTm = tm.position - gk.position;
        float dist = toTm.length();
        float basePassSpeed = (dist > 18.0f) ? SimConfig::LONG_PASS_SPEED_MPS : SimConfig::SHORT_PASS_SPEED_MPS;
        float passSpeed = m_randomEngine.passSpeedVariance(basePassSpeed);
        float timeToArrive = dist / passSpeed;
        Vec2 anticipatedPos = tm.position + tm.velocity * (timeToArrive * 0.55f);
        Vec2 passDir = (anticipatedPos - gk.position).normalized();

        m_ball.possessorId = -1;
        m_ball.intendedReceiverId = tm.id;
        m_ball.isShot = false;
        m_ball.isCross = false;
        m_ball.isOffsidePass = false;
        m_ball.velocity = passDir * passSpeed;
        m_ball.lastTouchedPlayerId = gk.id;
        m_ball.lastTouchedTeam = gk.team;
        m_possessionTeam = gk.team;

        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassAttempt, gk.team, gk.id, tm.id,
            gk.position, "Goalkeeper distribution restart"});
        std::cout << "[GK Distribution] " << gk.name << " -> " << tm.name
                  << " (dist=" << dist << "m)\n";
        break;
    }
}

void MatchEngine::executePass(Player& passer) {
    int targetId = findNearestTeammateForPass(passer);
    if (targetId == -1) {
        float attackingDir = (passer.team == TeamSide::Home) ? 1.0f : -1.0f;
        for (const auto& tm : m_players) {
            if (tm.team == passer.team && tm.id != passer.id && tm.role != TacticalRole::GK) {
                targetId = tm.id;
                break;
            }
        }

        if (targetId == -1) {
            passer.hasPossession = false;
            passer.touchCooldown = 0.9f;
            m_ball.possessorId = -1;
            m_ball.velocity = Vec2(attackingDir * 18.0f, 0.0f);
            return;
        }
    }

    DecisionOption opt;
    opt.type = DecisionType::ShortPass;
    opt.targetPlayerId = targetId;
    opt.description = "Pass";
    executePass(passer, opt);
}

void MatchEngine::executePass(Player& passer, const DecisionOption& option) {
    int targetId = option.targetPlayerId;
    if (targetId == -1) {
        targetId = findNearestTeammateForPass(passer);
    }

    Player* receiver = nullptr;
    for (auto& tm : m_players) {
        if (tm.id == targetId) {
            receiver = &tm;
            break;
        }
    }

    if (!receiver) return;

    Vec2 toTeammate = receiver->position - passer.position;
    float dist = toTeammate.length();

    bool isLongSwitch = (dist > 22.0f && std::abs(toTeammate.y) > 20.0f);
    float basePassSpeed = isLongSwitch ? SimConfig::CHIP_PASS_SPEED_MPS :
                          (dist > 22.0f ? SimConfig::LONG_PASS_SPEED_MPS : SimConfig::SHORT_PASS_SPEED_MPS);

    float passSpeed = m_randomEngine.passSpeedVariance(basePassSpeed);
    float timeToArrive = dist / passSpeed;

    Vec2 anticipatedPos = receiver->position + (receiver->velocity * (timeToArrive * 0.70f));
    Vec2 idealPassDir = (anticipatedPos - passer.position).normalized();

    float devDeg = m_randomEngine.passDeviationDeg(passer.attributes.passSkill * passer.formFactor);
    Vec2 passDir = m_randomEngine.deviateDirection(idealPassDir, devDeg);

    // LAW 11: Real Offside Evaluation at instant of pass release
    TeamSide oppTeam = (passer.team == TeamSide::Home) ? TeamSide::Away : TeamSide::Home;
    float offsideLine = calculateOffsideLine(oppTeam);
    bool isOffside = false;
    if (passer.team == TeamSide::Home) {
        if (receiver->position.x > 52.5f && receiver->position.x > passer.position.x && receiver->position.x > offsideLine) {
            isOffside = true;
        }
    } else {
        if (receiver->position.x < 52.5f && receiver->position.x < passer.position.x && receiver->position.x < offsideLine) {
            isOffside = true;
        }
    }

    passer.hasPossession = false;
    passer.touchCooldown = 0.8f;
    m_ball.possessorId = -1;
    m_ball.intendedReceiverId = receiver->id;
    m_ball.velocity = passDir * passSpeed;
    m_ball.isShot = false;
    m_ball.isCross = false;
    m_ball.isOffsidePass = isOffside;
    m_recentPasserId = passer.id;
    if (m_phase == MatchPhase::BuildUp) {
        if ((passer.team == TeamSide::Home && receiver->position.x > 50.0f) ||
            (passer.team == TeamSide::Away && receiver->position.x < 55.0f)) {
            m_phase = MatchPhase::InPlay;
        }
    }

    if (isLongSwitch) {
        m_ball.height = 0.2f;
        m_ball.verticalVelocity = 3.2f;
    } else {
        m_ball.height = 0.0f;
        m_ball.verticalVelocity = 0.0f;
    }

    m_ball.lastTouchedPlayerId = passer.id;
    m_ball.lastTouchedTeam = passer.team;
    m_phase = MatchPhase::InPlay;

    std::string passTypeDesc = isLongSwitch ? "Lofted switch pass" :
                              (passer.role == TacticalRole::GK ? "Goalkeeper restart" : "Progressive pass");

    m_eventRecorder.recordEvent({m_matchTime, MatchEventType::PassAttempt, passer.team, passer.id, receiver->id, passer.position, passTypeDesc});
}

void MatchEngine::executeShot(Player& shooter) {
    float goalX = (shooter.team == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
    float goalCenterY = 34.0f;

    // Calculate real Expected Goals (xG) based on StatsBomb analytical model
    float xg = MatchAnalytics::calculateShotXG(shooter.position, shooter.team);

    Player* opposingGK = nullptr;
    for (auto& p : m_players) {
        if (p.team != shooter.team && p.role == TacticalRole::GK) {
            opposingGK = &p;
            break;
        }
    }

    float shooterSkill = (shooter.attributes.finishing * 0.70f) + (shooter.attributes.shooting / 100.0f * 0.30f);
    shooterSkill *= shooter.formFactor;

    float gkSkill = 0.85f;
    if (opposingGK) {
        gkSkill = (opposingGK->attributes.gkReflexes * 0.40f) +
                  (opposingGK->attributes.gkDiving * 0.35f) +
                  (opposingGK->attributes.gkPositioning * 0.25f);
        gkSkill *= opposingGK->formFactor;
    }
    gkSkill = std::max(0.40f, gkSkill);

    float skillRatio = std::clamp(shooterSkill / gkSkill, 0.60f, 1.60f);
    float goalProb = std::clamp(xg * std::pow(skillRatio, 0.75f) * 0.92f, 0.040f, 0.19f);

    float roll = m_randomEngine.uniform(0.0f, 1.0f);
    float targetY = goalCenterY;
    float shotSpeed = SimConfig::SHOT_SPEED_MPS * (0.90f + shooter.attributes.shotPower / 100.0f * 0.20f);
    bool onTarget = false;
    ShotOutcome outcome = ShotOutcome::OffTarget;

    // Calibrated shot distribution (~40% on target, ~72.5% Opta save rate)
    float saveWindow = goalProb * 2.75f;

    if (roll < goalProb) {
        outcome = ShotOutcome::Goal;
        onTarget = true;
        float cornerSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
        targetY = goalCenterY + (cornerSide * m_randomEngine.uniform(2.70f, 3.15f));
    } else if (roll < goalProb + 0.030f) {
        outcome = ShotOutcome::Woodwork;
        onTarget = false;
        float postSide = m_randomEngine.chance(0.5f) ? -3.66f : 3.66f;
        targetY = goalCenterY + postSide;
    } else if (roll < goalProb + 0.030f + saveWindow) {
        outcome = ShotOutcome::Save;
        onTarget = true;
        float gkY = opposingGK ? opposingGK->position.y : goalCenterY;
        targetY = gkY + m_randomEngine.uniform(-1.2f, 1.2f);
        targetY = std::clamp(targetY, 31.5f, 36.5f);
    } else {
        outcome = ShotOutcome::OffTarget;
        onTarget = false;
        float missSide = m_randomEngine.chance(0.5f) ? -1.0f : 1.0f;
        targetY = goalCenterY + (missSide * m_randomEngine.uniform(4.5f, 9.5f));
    }

    Vec2 shotTarget(goalX, targetY);
    Vec2 idealShotDir = (shotTarget - shooter.position).normalized();
    Vec2 shotDir = idealShotDir;
    if (outcome == ShotOutcome::OffTarget) {
        float devDeg = m_randomEngine.shotDeviationDeg(shooter.attributes.shotPower * shooter.formFactor);
        shotDir = m_randomEngine.deviateDirection(idealShotDir, devDeg);
    } else if (outcome == ShotOutcome::Save) {
        shotDir = m_randomEngine.deviateDirection(idealShotDir, m_randomEngine.uniform(-0.8f, 0.8f));
    } else {
        shotDir = idealShotDir;
    }

    shooter.hasPossession = false;
    shooter.touchCooldown = 1.2f;
    m_ball.possessorId = -1;
    m_ball.intendedReceiverId = -1;
    m_ball.isShot = true;
    m_ball.isCross = false;
    m_ball.isOffsidePass = false;
    m_ball.shotOutcome = outcome;
    m_phase = MatchPhase::InPlay;

    m_ball.velocity = shotDir * shotSpeed;
    m_ball.height = 0.2f;
    m_ball.verticalVelocity = m_randomEngine.uniform(0.5f, 2.0f);
    m_ball.lastTouchedPlayerId = shooter.id;
    m_ball.lastTouchedTeam = shooter.team;

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Shot on goal [xG: " << xg << "]";
    std::string shotDetails = ss.str();

    if (onTarget) {
        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::ShotOnTarget, shooter.team, shooter.id, -1, shooter.position, shotDetails});
    } else {
        m_eventRecorder.recordEvent({m_matchTime, MatchEventType::ShotOffTarget, shooter.team, shooter.id, -1, shooter.position, "Shot off target"});
    }
    std::cout << "[SHOT!] " << shooter.name << (onTarget ? " hits target! " : " fires wide! ") << "(xG: " << xg << ")\n";
}

int MatchEngine::findNearestTeammateForPass(const Player& passer) {
    struct Candidate {
        int id;
        float score;
        float xP;
    };
    std::vector<Candidate> candidates;

    float attackingDir = (passer.team == TeamSide::Home) ? 1.0f : -1.0f;
    float passerProgression = (passer.position.x - 52.5f) * attackingDir;
    TeamSide oppTeam = (passer.team == TeamSide::Home) ? TeamSide::Away : TeamSide::Home;
    float offsideLine = calculateOffsideLine(oppTeam);

    bool allowBackpassToGK = false;
    float ownGoalDist = (passer.team == TeamSide::Home) ? passer.position.x : (SimConfig::PITCH_LENGTH_M - passer.position.x);
    if (ownGoalDist < 26.0f && ownGoalDist > 7.5f) {
        allowBackpassToGK = true;
    }

    for (const auto& tm : m_players) {
        if (tm.team != passer.team || tm.id == passer.id) {
            continue;
        }

        if (tm.role == TacticalRole::GK) {
            if (!allowBackpassToGK) continue;
            if (ownGoalDist < 7.5f) continue;
        }

        Vec2 toTm = tm.position - passer.position;
        float dist = toTm.length();
        if (dist < 4.0f || dist > 52.0f) continue;

        float passSpeed = (dist > 22.0f) ? SimConfig::LONG_PASS_SPEED_MPS : SimConfig::SHORT_PASS_SPEED_MPS;
        Vec2 passDir = toTm.normalized();

        // Spearman Potential Pitch Control Field (PPCF) Model
        float minOppArrivalMargin = 10.0f;
        bool directlyBlocked = false;

        for (const auto& opp : m_players) {
            if (opp.team != passer.team) {
                Vec2 toOpp = opp.position - passer.position;
                float proj = toOpp.dot(passDir);

                if (proj > 2.0f && proj < dist - 1.5f) {
                    Vec2 closestPoint = passer.position + (passDir * proj);
                    float oppDistToPoint = (opp.position - closestPoint).length();
                    float timeBallReachesPoint = proj / passSpeed;

                    float oppTime = SimConfig::SPEARMAN_REACTION_TIME_S + (oppDistToPoint / opp.attributes.maxSpeed);
                    float arrivalMargin = oppTime - timeBallReachesPoint;

                    if (arrivalMargin < minOppArrivalMargin) {
                        minOppArrivalMargin = arrivalMargin;
                    }
                    if (arrivalMargin < -0.15f && oppDistToPoint < 1.8f) {
                        directlyBlocked = true;
                    }
                }
            }
        }

        float xP = 1.0f / (1.0f + std::exp(-SimConfig::SPEARMAN_SIGMA_BETA * std::min(minOppArrivalMargin, 2.5f)));
        if (directlyBlocked) xP = 0.05f;

        float forwardness = toTm.x * attackingDir;
        float lateralSpread = std::abs(toTm.y);
        bool isSwitch = (lateralSpread > 20.0f);
        bool isBoxEntry = (std::abs(tm.position.x - (attackingDir > 0 ? 105.0f : 0.0f)) < 18.0f);

        // Check if receiver is in offside position
        bool isOffside = false;
        if (passer.team == TeamSide::Home) {
            isOffside = (tm.position.x > 52.5f && tm.position.x > passer.position.x && tm.position.x > offsideLine);
        } else {
            isOffside = (tm.position.x < 52.5f && tm.position.x < passer.position.x && tm.position.x < offsideLine);
        }

        // Receiver openness / marking distance (find least-marked teammate)
        float minOppDistToReceiver = 999.0f;
        for (const auto& opp : m_players) {
            if (opp.team != passer.team) {
                float d = (opp.position - tm.position).length();
                if (d < minOppDistToReceiver) {
                    minOppDistToReceiver = d;
                }
            }
        }

        // Openness bonus (least-marked teammate among progressive and midfield options)
        float opennessBonus = 0.0f;
        if (forwardness > -5.0f) {
            if (minOppDistToReceiver >= 4.5f) {
                opennessBonus = 6.0f + std::min(minOppDistToReceiver - 4.5f, 4.0f) * 1.5f;
            } else if (minOppDistToReceiver >= 2.5f) {
                opennessBonus = (minOppDistToReceiver - 2.5f) * 2.5f;
            } else {
                opennessBonus = -((2.5f - minOppDistToReceiver) * 5.0f);
            }
        }

        float score = 0.0f;
        if (passerProgression < -15.0f) {
            score = (xP * 38.0f) + opennessBonus + (lateralSpread * 0.35f) + (forwardness * 0.8f);
        } else if (passerProgression < 18.0f) {
            score = (xP * 26.0f) + opennessBonus + (forwardness * 1.8f) + (isSwitch ? 6.5f : 0.0f);
        } else {
            score = (xP * 18.0f) + opennessBonus + (forwardness * 3.2f) + (isBoxEntry ? 12.0f : 0.0f);
        }

        // Anti-ping-pong recency penalty: avoid immediately passing back to the same player
        if (tm.id == m_recentPasserId) {
            score -= 15.0f;
        }

        // Team Involvement: Ensure all outfield players participate in the build-up
        if (tm.role != TacticalRole::GK && tm.id >= 0 && tm.id < static_cast<int>(m_possessionChainTouches.size())) {
            int chainTouches = m_possessionChainTouches[tm.id];
            if (chainTouches == 0) {
                score += 8.0f; // Big incentive to involve outfield players who haven't touched the ball yet in this sequence
            } else if (chainTouches == 1) {
                score += 1.5f;
            } else {
                score -= 5.0f * (chainTouches - 1); // Penalize over-reliance on the same players
            }
        }

        // Build-up probing: encourage switching and using fullbacks/wingers to find weaknesses in the opposition
        if (m_phase == MatchPhase::BuildUp || passerProgression < 0.0f) {
            if (tm.role == TacticalRole::LB || tm.role == TacticalRole::RB) {
                score += 6.5f;
            } else if (tm.role == TacticalRole::LW || tm.role == TacticalRole::RW) {
                score += 4.5f;
            }
            if (isSwitch || lateralSpread > 15.0f) {
                score += 7.0f;
            }
            if (tm.role == TacticalRole::GK && minOppDistToReceiver >= 5.0f) {
                score += 3.0f;
            }
        }

        // Empirical Real Match Model (105 real matches / 108,807 passes)
        float passAngle = std::atan2(toTm.y, toTm.x * attackingDir);
        float empiricalDistWeight = EmpiricalTrainedModel::getPassDistanceWeight(dist);
        float empiricalAngleWeight = EmpiricalTrainedModel::getPassAngleWeight(passAngle);
        float empiricalCompletionProb = EmpiricalTrainedModel::getPassCompletionProbability(dist, minOppDistToReceiver < 2.2f);

        int fromZone = EmpiricalTrainedModel::getZone18Index(passer.position.x, passer.position.y);
        int toZone = EmpiricalTrainedModel::getZone18Index(tm.position.x, tm.position.y);
        float zoneTransitionProb = EmpiricalTrainedModel::getZoneTransitionProbability(fromZone, toZone);
        float zoneTransitionBonus = (zoneTransitionProb - 0.0555f) * 15.0f;

        score = (score * empiricalDistWeight * empiricalAngleWeight) + (empiricalCompletionProb * 6.0f) + zoneTransitionBonus;

        if (isOffside) {
            score -= 45.0f; // Heavily penalize passing to offside runners
        }

        if (xP > 0.20f || !directlyBlocked) {
            candidates.push_back({tm.id, score, xP});
        }
    }

    if (candidates.empty()) {
        return -1;
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        return a.score > b.score;
    });

    size_t poolSize = std::min(candidates.size(), size_t(4));
    float maxScore = candidates[0].score;
    std::vector<float> weights;
    weights.reserve(poolSize);

    for (size_t i = 0; i < poolSize; ++i) {
        weights.push_back(std::exp((candidates[i].score - maxScore) * 0.38f));
    }

    int chosenIdx = m_randomEngine.selectWeightedOption(weights);
    if (chosenIdx >= 0 && chosenIdx < static_cast<int>(poolSize)) {
        return candidates[chosenIdx].id;
    }

    return candidates[0].id;
}

DecisionOption MatchEngine::evaluateBestOnBallDecision(Player& player, bool allowDribble) {
    DecisionOption opt;
    float attackingGoalX = (player.team == TeamSide::Home) ? SimConfig::PITCH_LENGTH_M : 0.0f;
    float distToGoal = std::abs(player.position.x - attackingGoalX);
    float lateralDistToCenter = std::abs(player.position.y - 34.0f);
    float distToEndline = std::abs(attackingGoalX - player.position.x);

    // 1. Cross check
    if (distToGoal < 34.0f && (lateralDistToCenter > 16.0f || distToEndline < 8.0f)) {
        opt.type = DecisionType::CrossIntoBox;
        opt.targetPlayerId = -1;
        opt.utility = 2.4f;
        opt.description = "Cross into box";
        return opt;
    }

    // 2. Shot check (High-decision realism: inside prime box zone or clear opening, calibrated from 105 real matches)
    bool isAttackingRole = (player.role == TacticalRole::ST || player.role == TacticalRole::LW ||
                            player.role == TacticalRole::RW || player.fcIqRole == FCIQRole::PlaymakerCAM ||
                            player.fcIqRole == FCIQRole::PoacherST || player.fcIqRole == FCIQRole::InsideForward ||
                            player.fcIqRole == FCIQRole::FalseNine);
    float angleToGoal = std::atan2(lateralDistToCenter, distToGoal);
    float shotXG = EmpiricalTrainedModel::getCalibratedShotXG(distToGoal, angleToGoal);
    bool inPrimeShootingZone = (distToGoal < 15.5f && lateralDistToCenter < 11.0f);
    bool inSecondaryZone = (distToGoal < 21.0f && lateralDistToCenter < 7.5f && isAttackingRole && shotXG > 0.045f);

    if (inPrimeShootingZone || inSecondaryZone) {
        int passTarget = findNearestTeammateForPass(player);
        bool shouldCutback = false;
        if (passTarget != -1 && lateralDistToCenter > 5.5f) {
            for (const auto& tm : m_players) {
                if (tm.id == passTarget && tm.team == player.team) {
                    float tmDistToGoal = std::abs(tm.position.x - attackingGoalX);
                    if (tmDistToGoal < distToGoal - 2.0f && std::abs(tm.position.y - 34.0f) < 8.0f) {
                        shouldCutback = true;
                    }
                    break;
                }
            }
        }

        if (!shouldCutback) {
            opt.type = DecisionType::Shot;
            opt.targetPlayerId = -1;
            opt.utility = 2.6f;
            opt.description = "Shot on goal";
            return opt;
        }
    }

    // 3. Pass check
    int passTarget = findNearestTeammateForPass(player);
    if (passTarget != -1) {
        opt.type = DecisionType::ShortPass;
        opt.targetPlayerId = passTarget;
        opt.utility = 2.0f;
        opt.description = "Progressive pass";
        return opt;
    }

    // 4. Dribble
    if (allowDribble && distToEndline > 3.5f) {
        opt.type = DecisionType::DribbleDrive;
        opt.targetPlayerId = -1;
        opt.utility = 1.4f;
        opt.description = "Dribble forward";
        return opt;
    }

    opt.type = DecisionType::ShortPass;
    opt.targetPlayerId = passTarget;
    opt.utility = 1.0f;
    opt.description = "Release pass";
    return opt;
}

void MatchEngine::executeDecision(Player& player, const DecisionOption& decision) {
    switch (decision.type) {
        case DecisionType::Shot:
            executeShot(player);
            break;
        case DecisionType::CrossIntoBox:
            executeCross(player);
            break;
        case DecisionType::DribbleDrive:
            executeDribble(player, 0.05f);
            break;
        case DecisionType::InsideChannelThroughBall:
        case DecisionType::OverloadSwitch:
        case DecisionType::ShortPass:
        default:
            executePass(player, decision);
            break;
    }
}

float MatchEngine::calculateOffsideLine(TeamSide defendingTeam) const {
    std::vector<float> defenderX;
    defenderX.reserve(11);

    for (const auto& p : m_players) {
        if (p.team == defendingTeam) {
            defenderX.push_back(p.position.x);
        }
    }

    if (defenderX.size() < 2) {
        return (defendingTeam == TeamSide::Home) ? 20.0f : 85.0f;
    }

    if (defendingTeam == TeamSide::Home) {
        // Home defends x = 0 (Away attacks towards x = 0)
        // 2nd-last defender has 2nd smallest x
        std::sort(defenderX.begin(), defenderX.end());
        float secondLastDefX = defenderX[1];
        return std::min(52.5f, std::min(secondLastDefX, m_ball.position.x));
    } else {
        // Away defends x = 105 (Home attacks towards x = 105)
        // 2nd-last defender has 2nd largest x
        std::sort(defenderX.begin(), defenderX.end(), std::greater<float>());
        float secondLastDefX = defenderX[1];
        return std::max(52.5f, std::max(secondLastDefX, m_ball.position.x));
    }
}
