#include "PitchRenderer.hpp"
#include "Tactics.hpp"
#include "EmpiricalTrainedModel.hpp"
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdint>

PitchRenderer::PitchRenderer() {
    const std::vector<std::string> fontCandidates = {
        "assets/fonts/DejaVuSans-Bold.ttf",
        "assets/fonts/font.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf"
    };

    for (const auto& path : fontCandidates) {
#if SFML_VERSION_MAJOR >= 3
        if (m_font.openFromFile(path)) {
            m_fontLoaded = true;
            break;
        }
#else
        if (m_font.loadFromFile(path)) {
            m_fontLoaded = true;
            break;
        }
#endif
    }

    if (!m_fontLoaded) {
        std::cerr << "[PitchRenderer] Warning: Could not find system TTF font." << std::endl;
    }
}

void PitchRenderer::updateView(const sf::Vector2u& windowSize) {
    float totalPitchWidthM = SimConfig::PITCH_LENGTH_M + 2.0f * SimConfig::PITCH_MARGIN_M;
    float totalPitchHeightM = SimConfig::PITCH_WIDTH_M + 2.0f * SimConfig::PITCH_MARGIN_M;

    float topHudHeightPx = 52.0f;
    float bottomAnalyticsHeightPx = 120.0f;
    float availWidthPx = static_cast<float>(windowSize.x);
    float availHeightPx = static_cast<float>(windowSize.y) - topHudHeightPx - bottomAnalyticsHeightPx;

    float scaleX = availWidthPx / totalPitchWidthM;
    float scaleY = availHeightPx / totalPitchHeightM;
    m_scale = std::min(scaleX, scaleY);

    float renderedWidth = totalPitchWidthM * m_scale;
    float renderedHeight = totalPitchHeightM * m_scale;

    m_offsetX = (availWidthPx - renderedWidth) * 0.5f + (SimConfig::PITCH_MARGIN_M * m_scale);
    m_offsetY = topHudHeightPx + (availHeightPx - renderedHeight) * 0.5f + (SimConfig::PITCH_MARGIN_M * m_scale);
}

sf::Vector2f PitchRenderer::pitchToScreen(const Vec2& pitchPos) const {
    return sf::Vector2f(m_offsetX + pitchPos.x * m_scale, m_offsetY + pitchPos.y * m_scale);
}

Vec2 PitchRenderer::screenToPitch(const sf::Vector2f& screenPos) const {
    if (m_scale <= 0.0001f) return {0.0f, 0.0f};
    return Vec2((screenPos.x - m_offsetX) / m_scale, (screenPos.y - m_offsetY) / m_scale);
}

float PitchRenderer::metresToPixels(float metres) const {
    return metres * m_scale;
}

int PitchRenderer::selectPlayerAtScreenPos(const sf::Vector2f& mousePos, const std::vector<Player>& players) const {
    Vec2 pitchPos = screenToPitch(mousePos);
    for (const auto& p : players) {
        float dist = (p.position - pitchPos).length();
        if (dist <= SimConfig::PLAYER_RADIUS_M * 1.8f) {
            return p.id;
        }
    }
    return -1;
}

void PitchRenderer::render(sf::RenderWindow& window,
                           const std::vector<Player>& players,
                           const Ball& ball,
                           int homeScore,
                           int awayScore,
                           float matchClockSeconds,
                           const MatchAnalytics& analytics,
                           const EventRecorder& eventRecorder,
                           const SpatialAnalytics& spatialAnalytics,
                           int selectedPlayerId,
                           bool showHeatmap,
                           bool showPassingNetwork,
                           bool cameraFollowBall,
                           bool showCalibrationPanel,
                           MatchPhase phase,
                           float firstHalfAddedTime,
                           float secondHalfAddedTime,
                           TeamTacticalStyle homeStyle,
                           TeamTacticalStyle awayStyle,
                           bool showTacticalLines,
                           float simSpeed,
                           const TeamProfile* homeTeam,
                           const TeamProfile* awayTeam,
                           bool showTeamDetails,
                           FormationType homeFormation,
                           FormationType awayFormation,
                           bool inRealMatchMode,
                           const std::string& realMatchEventBanner) {
    if (cameraFollowBall) {
        // Camera smoothly follows the ball
        float targetOffsetX = (window.getSize().x * 0.5f) - (ball.position.x * m_scale);
        float targetOffsetY = (window.getSize().y * 0.5f) - (ball.position.y * m_scale);
        m_offsetX += (targetOffsetX - m_offsetX) * 0.08f;
        m_offsetY += (targetOffsetY - m_offsetY) * 0.08f;
    }

    drawGrassAndStripes(window);
    drawTechnicalAreas(window, homeTeam, awayTeam);
    drawPitchMarkings(window);
    drawCornerFlags(window);
    drawGoals(window);

    // Dynamic Team Spatial Shape / Compactness Convex Hulls
    if (showTacticalLines) {
        drawTeamHulls(window, players, homeTeam, awayTeam);
    }

    // Official VAR-Style Tactical Offside Line
    drawOffsideLine(window, players, ball.lastTouchedTeam, ball);

    // Passing network live lines (if enabled)
    if (showPassingNetwork) {
        drawPassingNetwork(window, spatialAnalytics, players);
    }

    // Dynamic tactical connection lines (updates for every position)
    if (showTacticalLines) {
        drawTacticalLines(window, players, ball);
    }

    // Ball flight trail (comet tail)
    drawBallTrail(window);

    drawPlayers(window, players, selectedPlayerId, homeTeam, awayTeam);
    drawBall(window, ball);
    drawHUD(window, homeScore, awayScore, matchClockSeconds, analytics, phase, firstHalfAddedTime, secondHalfAddedTime, homeStyle, awayStyle, simSpeed, showTacticalLines, homeTeam, awayTeam, homeFormation, awayFormation, inRealMatchMode, realMatchEventBanner);

    if (showCalibrationPanel && !inRealMatchMode) {
        drawCalibrationPanel(window, analytics, homeScore, awayScore, matchClockSeconds);
    }

    if (showTeamDetails && homeTeam && awayTeam && !inRealMatchMode) {
        drawTeamDetailsPanel(window, *homeTeam, *awayTeam);
    }

    drawMomentumAndStatsPanel(window, analytics, eventRecorder, selectedPlayerId, showHeatmap, showPassingNetwork, cameraFollowBall, players, spatialAnalytics);
}

void PitchRenderer::drawGrassAndStripes(sf::RenderWindow& window) {
    float margin = SimConfig::PITCH_MARGIN_M;
    sf::Vector2f outerTopLeft = pitchToScreen({-margin, -margin});
    sf::Vector2f outerSize = sf::Vector2f(metresToPixels(SimConfig::PITCH_LENGTH_M + 2.0f * margin),
                                          metresToPixels(SimConfig::PITCH_WIDTH_M + 2.0f * margin));

    sf::RectangleShape outerGrass(outerSize);
    outerGrass.setPosition(outerTopLeft);
    outerGrass.setFillColor(sf::Color(32, 85, 36));
    window.draw(outerGrass);

    constexpr int NUM_STRIPES = 10;
    float stripeWidthM = SimConfig::PITCH_LENGTH_M / static_cast<float>(NUM_STRIPES);

    for (int i = 0; i < NUM_STRIPES; ++i) {
        sf::Vector2f sPos = pitchToScreen({i * stripeWidthM, 0.0f});
        sf::RectangleShape stripe(sf::Vector2f(metresToPixels(stripeWidthM), metresToPixels(SimConfig::PITCH_WIDTH_M)));
        stripe.setPosition(sPos);
        stripe.setFillColor((i % 2 == 0) ? m_grassDark : m_grassLight);
        window.draw(stripe);
    }
}

void PitchRenderer::drawPitchMarkings(sf::RenderWindow& window) {
    float lineThickness = std::max(1.5f, metresToPixels(0.12f));

    sf::Vector2f pitchTL = pitchToScreen({0.0f, 0.0f});
    sf::RectangleShape boundary(sf::Vector2f(metresToPixels(SimConfig::PITCH_LENGTH_M),
                                            metresToPixels(SimConfig::PITCH_WIDTH_M)));
    boundary.setPosition(pitchTL);
    boundary.setFillColor(sf::Color::Transparent);
    boundary.setOutlineColor(m_lineColor);
    boundary.setOutlineThickness(lineThickness);
    window.draw(boundary);

    sf::RectangleShape halfwayLine(sf::Vector2f(lineThickness, metresToPixels(SimConfig::PITCH_WIDTH_M)));
    halfwayLine.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M * 0.5f, 0.0f}));
    halfwayLine.setFillColor(m_lineColor);
    window.draw(halfwayLine);

    float centerRadiusPx = metresToPixels(SimConfig::CENTER_CIRCLE_RADIUS_M);
    sf::CircleShape centerCircle(centerRadiusPx);
    centerCircle.setOrigin(sf::Vector2f(centerRadiusPx, centerRadiusPx));
    centerCircle.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M * 0.5f, SimConfig::PITCH_WIDTH_M * 0.5f}));
    centerCircle.setFillColor(sf::Color::Transparent);
    centerCircle.setOutlineColor(m_lineColor);
    centerCircle.setOutlineThickness(lineThickness);
    window.draw(centerCircle);

    float spotRadiusPx = std::max(2.5f, metresToPixels(0.25f));
    sf::CircleShape centerSpot(spotRadiusPx);
    centerSpot.setOrigin(sf::Vector2f(spotRadiusPx, spotRadiusPx));
    centerSpot.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M * 0.5f, SimConfig::PITCH_WIDTH_M * 0.5f}));
    centerSpot.setFillColor(m_lineColor);
    window.draw(centerSpot);

    float penAreaW = metresToPixels(SimConfig::PENALTY_AREA_LENGTH_M);
    float penAreaH = metresToPixels(SimConfig::PENALTY_AREA_WIDTH_M);
    float penAreaStartY_M = (SimConfig::PITCH_WIDTH_M - SimConfig::PENALTY_AREA_WIDTH_M) * 0.5f;

    sf::RectangleShape leftPenArea(sf::Vector2f(penAreaW, penAreaH));
    leftPenArea.setPosition(pitchToScreen({0.0f, penAreaStartY_M}));
    leftPenArea.setFillColor(sf::Color::Transparent);
    leftPenArea.setOutlineColor(m_lineColor);
    leftPenArea.setOutlineThickness(lineThickness);
    window.draw(leftPenArea);

    sf::RectangleShape rightPenArea(sf::Vector2f(penAreaW, penAreaH));
    rightPenArea.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M - SimConfig::PENALTY_AREA_LENGTH_M, penAreaStartY_M}));
    rightPenArea.setFillColor(sf::Color::Transparent);
    rightPenArea.setOutlineColor(m_lineColor);
    rightPenArea.setOutlineThickness(lineThickness);
    window.draw(rightPenArea);

    float goalAreaW = metresToPixels(SimConfig::GOAL_AREA_LENGTH_M);
    float goalAreaH = metresToPixels(SimConfig::GOAL_AREA_WIDTH_M);
    float goalAreaStartY_M = (SimConfig::PITCH_WIDTH_M - SimConfig::GOAL_AREA_WIDTH_M) * 0.5f;

    sf::RectangleShape leftGoalArea(sf::Vector2f(goalAreaW, goalAreaH));
    leftGoalArea.setPosition(pitchToScreen({0.0f, goalAreaStartY_M}));
    leftGoalArea.setFillColor(sf::Color::Transparent);
    leftGoalArea.setOutlineColor(m_lineColor);
    leftGoalArea.setOutlineThickness(lineThickness);
    window.draw(leftGoalArea);

    sf::RectangleShape rightGoalArea(sf::Vector2f(goalAreaW, goalAreaH));
    rightGoalArea.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M - SimConfig::GOAL_AREA_LENGTH_M, goalAreaStartY_M}));
    rightGoalArea.setFillColor(sf::Color::Transparent);
    rightGoalArea.setOutlineColor(m_lineColor);
    rightGoalArea.setOutlineThickness(lineThickness);
    window.draw(rightGoalArea);

    sf::CircleShape leftPenSpot(spotRadiusPx);
    leftPenSpot.setOrigin(sf::Vector2f(spotRadiusPx, spotRadiusPx));
    leftPenSpot.setPosition(pitchToScreen({SimConfig::PENALTY_SPOT_DIST_M, SimConfig::PITCH_WIDTH_M * 0.5f}));
    leftPenSpot.setFillColor(m_lineColor);
    window.draw(leftPenSpot);

    sf::CircleShape rightPenSpot(spotRadiusPx);
    rightPenSpot.setOrigin(sf::Vector2f(spotRadiusPx, spotRadiusPx));
    rightPenSpot.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M - SimConfig::PENALTY_SPOT_DIST_M, SimConfig::PITCH_WIDTH_M * 0.5f}));
    rightPenSpot.setFillColor(m_lineColor);
    window.draw(rightPenSpot);

    // 1. Official Penalty Arcs (The "D" outside the penalty box, 9.15m radius)
    float arcRadiusM = SimConfig::CENTER_CIRCLE_RADIUS_M; // 9.15m
    float theta0 = 0.925f; // ~53.0 deg
    constexpr int ARC_SEGS = 14;

    Vec2 leftSpot(SimConfig::PENALTY_SPOT_DIST_M, SimConfig::PITCH_WIDTH_M * 0.5f);
    for (int i = 0; i < ARC_SEGS; ++i) {
        float a1 = -theta0 + (static_cast<float>(i) / ARC_SEGS) * (2.0f * theta0);
        float a2 = -theta0 + (static_cast<float>(i + 1) / ARC_SEGS) * (2.0f * theta0);
        Vec2 p1 = leftSpot + Vec2(std::cos(a1) * arcRadiusM, std::sin(a1) * arcRadiusM);
        Vec2 p2 = leftSpot + Vec2(std::cos(a2) * arcRadiusM, std::sin(a2) * arcRadiusM);
        drawScreenLine(window, pitchToScreen(p1), pitchToScreen(p2), lineThickness, m_lineColor);
    }

    Vec2 rightSpot(SimConfig::PITCH_LENGTH_M - SimConfig::PENALTY_SPOT_DIST_M, SimConfig::PITCH_WIDTH_M * 0.5f);
    for (int i = 0; i < ARC_SEGS; ++i) {
        float a1 = 3.14159265f - theta0 + (static_cast<float>(i) / ARC_SEGS) * (2.0f * theta0);
        float a2 = 3.14159265f - theta0 + (static_cast<float>(i + 1) / ARC_SEGS) * (2.0f * theta0);
        Vec2 p1 = rightSpot + Vec2(std::cos(a1) * arcRadiusM, std::sin(a1) * arcRadiusM);
        Vec2 p2 = rightSpot + Vec2(std::cos(a2) * arcRadiusM, std::sin(a2) * arcRadiusM);
        drawScreenLine(window, pitchToScreen(p1), pitchToScreen(p2), lineThickness, m_lineColor);
    }

    // 2. Corner Arcs (1m radius at all four corners)
    float cArcR = 1.0f;
    constexpr int C_SEGS = 8;
    for (int i = 0; i < C_SEGS; ++i) {
        float a1 = (static_cast<float>(i) / C_SEGS) * 1.570796f;
        float a2 = (static_cast<float>(i + 1) / C_SEGS) * 1.570796f;
        drawScreenLine(window, pitchToScreen({std::cos(a1)*cArcR, std::sin(a1)*cArcR}),
                              pitchToScreen({std::cos(a2)*cArcR, std::sin(a2)*cArcR}), lineThickness, m_lineColor);
        drawScreenLine(window, pitchToScreen({std::cos(a1)*cArcR, SimConfig::PITCH_WIDTH_M - std::sin(a1)*cArcR}),
                              pitchToScreen({std::cos(a2)*cArcR, SimConfig::PITCH_WIDTH_M - std::sin(a2)*cArcR}), lineThickness, m_lineColor);
        drawScreenLine(window, pitchToScreen({SimConfig::PITCH_LENGTH_M - std::cos(a1)*cArcR, std::sin(a1)*cArcR}),
                              pitchToScreen({SimConfig::PITCH_LENGTH_M - std::cos(a2)*cArcR, std::sin(a2)*cArcR}), lineThickness, m_lineColor);
        drawScreenLine(window, pitchToScreen({SimConfig::PITCH_LENGTH_M - std::cos(a1)*cArcR, SimConfig::PITCH_WIDTH_M - std::sin(a1)*cArcR}),
                              pitchToScreen({SimConfig::PITCH_LENGTH_M - std::cos(a2)*cArcR, SimConfig::PITCH_WIDTH_M - std::sin(a2)*cArcR}), lineThickness, m_lineColor);
    }
}

void PitchRenderer::drawGoals(sf::RenderWindow& window) {
    float goalDepthPx = metresToPixels(SimConfig::GOAL_DEPTH_M);
    float goalWidthPx = metresToPixels(SimConfig::GOAL_WIDTH_M);
    float goalStartY_M = (SimConfig::PITCH_WIDTH_M - SimConfig::GOAL_WIDTH_M) * 0.5f;

    sf::Color netFill(255, 255, 255, 20);
    sf::Color netMesh(255, 255, 255, 60);
    sf::Color postWhite(255, 255, 255, 250);

    // Left Goal (Home side)
    sf::RectangleShape leftGoal(sf::Vector2f(goalDepthPx, goalWidthPx));
    leftGoal.setPosition(pitchToScreen({-SimConfig::GOAL_DEPTH_M, goalStartY_M}));
    leftGoal.setFillColor(netFill);
    leftGoal.setOutlineColor(postWhite);
    leftGoal.setOutlineThickness(2.2f);
    window.draw(leftGoal);

    // Left Goal Netting Mesh
    for (int i = 1; i <= 3; ++i) {
        float x = -SimConfig::GOAL_DEPTH_M + (static_cast<float>(i) * SimConfig::GOAL_DEPTH_M / 4.0f);
        drawScreenLine(window, pitchToScreen({x, goalStartY_M}), pitchToScreen({x, goalStartY_M + SimConfig::GOAL_WIDTH_M}), 1.0f, netMesh);
    }
    for (int j = 1; j <= 5; ++j) {
        float y = goalStartY_M + (static_cast<float>(j) * SimConfig::GOAL_WIDTH_M / 6.0f);
        drawScreenLine(window, pitchToScreen({-SimConfig::GOAL_DEPTH_M, y}), pitchToScreen({0.0f, y}), 1.0f, netMesh);
    }

    // Right Goal (Away side)
    sf::RectangleShape rightGoal(sf::Vector2f(goalDepthPx, goalWidthPx));
    rightGoal.setPosition(pitchToScreen({SimConfig::PITCH_LENGTH_M, goalStartY_M}));
    rightGoal.setFillColor(netFill);
    rightGoal.setOutlineColor(postWhite);
    rightGoal.setOutlineThickness(2.2f);
    window.draw(rightGoal);

    // Right Goal Netting Mesh
    for (int i = 1; i <= 3; ++i) {
        float x = SimConfig::PITCH_LENGTH_M + (static_cast<float>(i) * SimConfig::GOAL_DEPTH_M / 4.0f);
        drawScreenLine(window, pitchToScreen({x, goalStartY_M}), pitchToScreen({x, goalStartY_M + SimConfig::GOAL_WIDTH_M}), 1.0f, netMesh);
    }
    for (int j = 1; j <= 5; ++j) {
        float y = goalStartY_M + (static_cast<float>(j) * SimConfig::GOAL_WIDTH_M / 6.0f);
        drawScreenLine(window, pitchToScreen({SimConfig::PITCH_LENGTH_M, y}), pitchToScreen({SimConfig::PITCH_LENGTH_M + SimConfig::GOAL_DEPTH_M, y}), 1.0f, netMesh);
    }
}

void PitchRenderer::drawCornerFlags(sf::RenderWindow& window) {
    const std::vector<Vec2> corners = {
        {0.0f, 0.0f},
        {0.0f, SimConfig::PITCH_WIDTH_M},
        {SimConfig::PITCH_LENGTH_M, 0.0f},
        {SimConfig::PITCH_LENGTH_M, SimConfig::PITCH_WIDTH_M}
    };

    for (const auto& c : corners) {
        sf::Vector2f sPos = pitchToScreen(c);
        // Pole
        drawScreenLine(window, sPos, sf::Vector2f(sPos.x, sPos.y - 10.0f), 1.5f, sf::Color(220, 220, 220));
        // Triangular flag
        sf::ConvexShape flag(3);
        flag.setPoint(0, sf::Vector2f(sPos.x, sPos.y - 10.0f));
        flag.setPoint(1, sf::Vector2f(sPos.x + 6.0f, sPos.y - 7.5f));
        flag.setPoint(2, sf::Vector2f(sPos.x, sPos.y - 5.0f));
        flag.setFillColor(sf::Color(241, 196, 15)); // Bright yellow
        window.draw(flag);
    }
}

void PitchRenderer::drawTechnicalAreas(sf::RenderWindow& window, const TeamProfile* homeTeam, const TeamProfile* awayTeam) {
    sf::Color homeCol = homeTeam ? homeTeam->primaryColor : sf::Color(218, 41, 28);
    sf::Color awayCol = awayTeam ? awayTeam->primaryColor : sf::Color(41, 128, 185);

    // Home Dugout Area: x: 38m - 49m, y: -2.4m - -0.4m
    sf::Vector2f hTL = pitchToScreen({38.0f, -2.4f});
    sf::Vector2f hSize(metresToPixels(11.0f), metresToPixels(2.0f));
    sf::RectangleShape hBox(hSize);
    hBox.setPosition(hTL);
    hBox.setFillColor(sf::Color(homeCol.r, homeCol.g, homeCol.b, 25));
    hBox.setOutlineColor(sf::Color(255, 255, 255, 75));
    hBox.setOutlineThickness(1.0f);
    window.draw(hBox);

    sf::RectangleShape hBench(sf::Vector2f(metresToPixels(7.5f), metresToPixels(0.8f)));
    hBench.setPosition(pitchToScreen({39.8f, -2.0f}));
    hBench.setFillColor(homeCol);
    window.draw(hBench);

    // Away Dugout Area: x: 56m - 67m, y: -2.4m - -0.4m
    sf::Vector2f aTL = pitchToScreen({56.0f, -2.4f});
    sf::Vector2f aSize(metresToPixels(11.0f), metresToPixels(2.0f));
    sf::RectangleShape aBox(aSize);
    aBox.setPosition(aTL);
    aBox.setFillColor(sf::Color(awayCol.r, awayCol.g, awayCol.b, 25));
    aBox.setOutlineColor(sf::Color(255, 255, 255, 75));
    aBox.setOutlineThickness(1.0f);
    window.draw(aBox);

    sf::RectangleShape aBench(sf::Vector2f(metresToPixels(7.5f), metresToPixels(0.8f)));
    aBench.setPosition(pitchToScreen({57.8f, -2.0f}));
    aBench.setFillColor(awayCol);
    window.draw(aBench);
}

void PitchRenderer::drawTeamHulls(sf::RenderWindow& window, const std::vector<Player>& players, const TeamProfile* homeTeam, const TeamProfile* awayTeam) {
    for (TeamSide side : {TeamSide::Home, TeamSide::Away}) {
        TeamSpatialShape shape = SpatialAnalytics::calculateTeamShape(players, side, true);
        if (shape.hullVertices.size() < 3) continue;

        sf::Color teamColor = (side == TeamSide::Home)
            ? (homeTeam ? homeTeam->primaryColor : m_homeColor)
            : (awayTeam ? awayTeam->primaryColor : m_awayColor);

        // Semi-transparent polygon fill (14% opacity for professional broadcast overlay look)
        sf::ConvexShape hullPoly;
        hullPoly.setPointCount(shape.hullVertices.size());
        for (size_t i = 0; i < shape.hullVertices.size(); ++i) {
            hullPoly.setPoint(i, pitchToScreen(shape.hullVertices[i]));
        }
        hullPoly.setFillColor(sf::Color(teamColor.r, teamColor.g, teamColor.b, 22));
        hullPoly.setOutlineColor(sf::Color(teamColor.r, teamColor.g, teamColor.b, 115));
        hullPoly.setOutlineThickness(1.4f);
        window.draw(hullPoly);

        // Centroid marker (+)
        sf::Vector2f cPos = pitchToScreen(shape.centroid);
        float arm = 5.0f;
        sf::Color cCol(teamColor.r, teamColor.g, teamColor.b, 200);
        drawScreenLine(window, cPos - sf::Vector2f(arm, 0.0f), cPos + sf::Vector2f(arm, 0.0f), 1.5f, cCol);
        drawScreenLine(window, cPos - sf::Vector2f(0.0f, arm), cPos + sf::Vector2f(0.0f, arm), 1.5f, cCol);
    }
}

void PitchRenderer::drawOffsideLine(sf::RenderWindow& window, const std::vector<Player>& players, TeamSide possessionTeam, const Ball& ball) {
    TeamSide defendingTeam = (possessionTeam == TeamSide::Home) ? TeamSide::Away : TeamSide::Home;

    std::vector<float> defX;
    defX.reserve(11);
    for (const auto& p : players) {
        if (p.team == defendingTeam) {
            defX.push_back(p.position.x);
        }
    }
    if (defX.size() < 2) return;

    float offsideX = 52.5f;
    if (defendingTeam == TeamSide::Home) {
        std::sort(defX.begin(), defX.end());
        offsideX = std::min(52.5f, std::min(defX[1], ball.position.x));
    } else {
        std::sort(defX.begin(), defX.end(), std::greater<float>());
        offsideX = std::max(52.5f, std::max(defX[1], ball.position.x));
    }

    sf::Vector2f topPos = pitchToScreen({offsideX, 0.0f});
    sf::Vector2f btmPos = pitchToScreen({offsideX, SimConfig::PITCH_WIDTH_M});

    float segLen = 7.0f;
    float gapLen = 5.0f;
    float curY = topPos.y;
    sf::Color offsideCol(241, 196, 15, 175);
    while (curY < btmPos.y) {
        float nextY = std::min(btmPos.y, curY + segLen);
        drawScreenLine(window, {topPos.x, curY}, {topPos.x, nextY}, 1.4f, offsideCol);
        curY = nextY + gapLen;
    }

    if (m_fontLoaded) {
        std::ostringstream ss;
        ss << "OFFSIDE: " << std::fixed << std::setprecision(1) << offsideX << "m";
        sf::Text label(m_font, ss.str(), 9);
        label.setFillColor(offsideCol);
        label.setPosition(sf::Vector2f(topPos.x + 3.0f, topPos.y - 13.0f));
        window.draw(label);
    }
}

void PitchRenderer::drawBallTrail(sf::RenderWindow& window) {
    if (m_ballTrail.size() < 2) return;
    size_t count = m_ballTrail.size();
    for (size_t i = 1; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(count);
        float thickness = 1.0f + t * 2.2f;
        std::uint8_t alpha = static_cast<std::uint8_t>(t * 160.0f);
        sf::Color trailCol(255, 255, 220, alpha);
        drawScreenLine(window, m_ballTrail[i - 1], m_ballTrail[i], thickness, trailCol);
    }
}

void PitchRenderer::drawMiniPitchHeatmap(sf::RenderWindow& window, const SpatialAnalytics& spatialAnalytics, int selectedPlayerId, float px, float py, float pw, float ph) {
    // 1. Mini pitch background
    sf::RectangleShape miniPitch(sf::Vector2f(pw, ph));
    miniPitch.setPosition(sf::Vector2f(px, py));
    miniPitch.setFillColor(sf::Color(16, 50, 26));
    miniPitch.setOutlineColor(sf::Color(70, 135, 85));
    miniPitch.setOutlineThickness(1.2f);
    window.draw(miniPitch);

    // 2. Center dividing line
    sf::RectangleShape cLine(sf::Vector2f(1.0f, ph));
    cLine.setPosition(sf::Vector2f(px + pw * 0.5f, py));
    cLine.setFillColor(sf::Color(255, 255, 255, 60));
    window.draw(cLine);

    // 3. Center circle
    float cRad = ph * 0.22f;
    sf::CircleShape cCircle(cRad);
    cCircle.setPosition(sf::Vector2f(px + pw * 0.5f - cRad, py + ph * 0.5f - cRad));
    cCircle.setFillColor(sf::Color::Transparent);
    cCircle.setOutlineColor(sf::Color(255, 255, 255, 60));
    cCircle.setOutlineThickness(1.0f);
    window.draw(cCircle);

    // 4. Penalty boxes on both ends
    float boxW = pw * 0.16f;
    float boxH = ph * 0.58f;
    float boxY = py + (ph - boxH) * 0.5f;

    sf::RectangleShape lBox(sf::Vector2f(boxW, boxH));
    lBox.setPosition(sf::Vector2f(px, boxY));
    lBox.setFillColor(sf::Color::Transparent);
    lBox.setOutlineColor(sf::Color(255, 255, 255, 50));
    lBox.setOutlineThickness(1.0f);
    window.draw(lBox);

    sf::RectangleShape rBox(sf::Vector2f(boxW, boxH));
    rBox.setPosition(sf::Vector2f(px + pw - boxW, boxY));
    rBox.setFillColor(sf::Color::Transparent);
    rBox.setOutlineColor(sf::Color(255, 255, 255, 50));
    rBox.setOutlineThickness(1.0f);
    window.draw(rBox);

    // 5. Heatmap density tiles
    const auto* hm = spatialAnalytics.getHeatmap(selectedPlayerId);
    if (!hm || hm->maxCount <= 0) return;

    float cellW = pw / static_cast<float>(HEATMAP_COLS);
    float cellH = ph / static_cast<float>(HEATMAP_ROWS);

    for (int col = 0; col < HEATMAP_COLS; ++col) {
        for (int row = 0; row < HEATMAP_ROWS; ++row) {
            int cnt = hm->counts[col][row];
            if (cnt == 0) continue;

            float intensity = static_cast<float>(cnt) / static_cast<float>(hm->maxCount);
            intensity = std::clamp(intensity, 0.0f, 1.0f);

            sf::Color cellCol;
            if (intensity < 0.35f) {
                cellCol = sf::Color(255, 240, 0, static_cast<std::uint8_t>(intensity * 2.8f * 160.0f));
            } else if (intensity < 0.70f) {
                cellCol = sf::Color(255, 140, 0, static_cast<std::uint8_t>(160.0f + (intensity - 0.35f) * 75.0f));
            } else {
                cellCol = sf::Color(235, 25, 0, static_cast<std::uint8_t>(210.0f + (intensity - 0.70f) * 45.0f));
            }

            sf::RectangleShape cell(sf::Vector2f(cellW + 0.5f, cellH + 0.5f));
            cell.setPosition(sf::Vector2f(px + col * cellW, py + row * cellH));
            cell.setFillColor(cellCol);
            window.draw(cell);
        }
    }
}

void PitchRenderer::drawScreenLine(sf::RenderWindow& window, const sf::Vector2f& p1, const sf::Vector2f& p2, float thickness, const sf::Color& color) {
    sf::Vector2f dir = p2 - p1;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 1.0f) return;

    sf::RectangleShape line(sf::Vector2f(len, thickness));
    line.setOrigin(sf::Vector2f(0.0f, thickness * 0.5f));
    line.setPosition(p1);
    line.setFillColor(color);
#if SFML_VERSION_MAJOR >= 3
    line.setRotation(sf::degrees(std::atan2(dir.y, dir.x) * 180.0f / 3.14159265f));
#else
    line.setRotation(std::atan2(dir.y, dir.x) * 180.0f / 3.14159265f);
#endif
    window.draw(line);
}

void PitchRenderer::drawPassingNetwork(sf::RenderWindow& window, const SpatialAnalytics& spatialAnalytics, const std::vector<Player>& players) {
    const auto& connections = spatialAnalytics.getPassConnections();
    const auto& avgPositions = spatialAnalytics.getAveragePositions();

    // Draw connection lines between live player positions (updates dynamically for every position)
    for (const auto& conn : connections) {
        sf::Vector2f p1, p2;
        bool foundFrom = false, foundTo = false;

        for (const auto& p : players) {
            if (p.id == conn.fromPlayerId) {
                p1 = pitchToScreen(p.position);
                foundFrom = true;
            }
            if (p.id == conn.toPlayerId) {
                p2 = pitchToScreen(p.position);
                foundTo = true;
            }
        }

        // Fallback to average positions if player not found
        if (!foundFrom) {
            auto it = avgPositions.find(conn.fromPlayerId);
            if (it != avgPositions.end()) { p1 = pitchToScreen(it->second); foundFrom = true; }
        }
        if (!foundTo) {
            auto it = avgPositions.find(conn.toPlayerId);
            if (it != avgPositions.end()) { p2 = pitchToScreen(it->second); foundTo = true; }
        }
        if (!foundFrom || !foundTo) continue;

        // Thickness proportional to pass frequency
        float thickness = std::min(4.5f, 1.2f + static_cast<float>(conn.passCount) * 0.7f);
        sf::Color lineCol = (conn.team == TeamSide::Home)
            ? sf::Color(231, 76, 60, std::min(220, 85 + conn.passCount * 25))
            : sf::Color(52, 152, 219, std::min(220, 85 + conn.passCount * 25));

        drawScreenLine(window, p1, p2, thickness, lineCol);
    }

    // Draw average position node rings
    for (const auto& p : players) {
        sf::Vector2f screenPos = pitchToScreen(p.position);
        float radius = metresToPixels(SimConfig::PLAYER_RADIUS_M * 1.35f);

        sf::CircleShape node(radius);
        node.setOrigin(sf::Vector2f(radius, radius));
        node.setPosition(screenPos);
        node.setFillColor(sf::Color::Transparent);
        node.setOutlineColor(p.team == TeamSide::Home ? sf::Color(231, 76, 60, 200) : sf::Color(52, 152, 219, 200));
        node.setOutlineThickness(2.0f);
        window.draw(node);
    }
}

void PitchRenderer::drawTacticalLines(sf::RenderWindow& window, const std::vector<Player>& players, const Ball& ball) {
    (void)ball;
    // 1. Live Formation Unit Lines for both teams
    for (TeamSide side : {TeamSide::Home, TeamSide::Away}) {
        sf::Color defColor = (side == TeamSide::Home) ? sf::Color(235, 77, 75, 170) : sf::Color(52, 152, 219, 170);
        sf::Color midColor = (side == TeamSide::Home) ? sf::Color(241, 196, 15, 150) : sf::Color(72, 219, 251, 150);
        sf::Color attColor = (side == TeamSide::Home) ? sf::Color(255, 107, 107, 170) : sf::Color(10, 189, 227, 170);
        sf::Color spineColor = sf::Color(255, 255, 255, 75);

        std::vector<const Player*> defenders;
        std::vector<const Player*> midfielders;
        std::vector<const Player*> attackers;
        const Player* centralCB = nullptr;
        const Player* centralDM = nullptr;
        const Player* centralCAM = nullptr;
        const Player* striker = nullptr;

        for (const auto& p : players) {
            if (p.team != side) continue;
            switch (p.role) {
                case TacticalRole::LB:
                case TacticalRole::CB_LEFT:
                case TacticalRole::CB_RIGHT:
                case TacticalRole::RB:
                    defenders.push_back(&p);
                    if (p.role == TacticalRole::CB_LEFT || p.role == TacticalRole::CB_RIGHT) centralCB = &p;
                    break;
                case TacticalRole::DM:
                    midfielders.push_back(&p);
                    centralDM = &p;
                    break;
                case TacticalRole::CM_LEFT:
                case TacticalRole::CM_RIGHT:
                    midfielders.push_back(&p);
                    if (p.fcIqRole == FCIQRole::PlaymakerCAM) {
                        centralCAM = &p;
                    }
                    break;
                case TacticalRole::LW:
                case TacticalRole::RW:
                case TacticalRole::ST:
                    attackers.push_back(&p);
                    if (p.role == TacticalRole::ST) striker = &p;
                    break;
                case TacticalRole::GK:
                default:
                    break;
            }
        }

        auto sortByY = [](const Player* a, const Player* b) {
            return a->position.y < b->position.y;
        };
        std::sort(defenders.begin(), defenders.end(), sortByY);
        std::sort(midfielders.begin(), midfielders.end(), sortByY);
        std::sort(attackers.begin(), attackers.end(), sortByY);

        // Defensive line connecting defenders across pitch width
        for (size_t i = 0; i + 1 < defenders.size(); ++i) {
            drawScreenLine(window, pitchToScreen(defenders[i]->position), pitchToScreen(defenders[i + 1]->position), 2.2f, defColor);
        }

        // Midfield unit line
        for (size_t i = 0; i + 1 < midfielders.size(); ++i) {
            drawScreenLine(window, pitchToScreen(midfielders[i]->position), pitchToScreen(midfielders[i + 1]->position), 1.8f, midColor);
        }

        // Attacking line
        for (size_t i = 0; i + 1 < attackers.size(); ++i) {
            drawScreenLine(window, pitchToScreen(attackers[i]->position), pitchToScreen(attackers[i + 1]->position), 2.0f, attColor);
        }

        // Vertical tactical spine connectors (depth links)
        if (centralCB && centralDM) {
            drawScreenLine(window, pitchToScreen(centralCB->position), pitchToScreen(centralDM->position), 1.2f, spineColor);
        }
        if (centralDM && (centralCAM ? centralCAM : striker)) {
            const Player* nextTarget = centralCAM ? centralCAM : striker;
            drawScreenLine(window, pitchToScreen(centralDM->position), pitchToScreen(nextTarget->position), 1.2f, spineColor);
        }
        if (centralCAM && striker) {
            drawScreenLine(window, pitchToScreen(centralCAM->position), pitchToScreen(striker->position), 1.2f, spineColor);
        }
    }

    // 2. Dynamic Live Passing Vision Lines for Ball Possessor
    for (const auto& passer : players) {
        if (!passer.hasPossession) continue;

        sf::Vector2f passerScreen = pitchToScreen(passer.position);
        float bestOpenness = -100.0f;
        const Player* bestReceiver = nullptr;

        for (const auto& tm : players) {
            if (tm.team != passer.team || tm.id == passer.id) continue;
            float dist = (tm.position - passer.position).length();
            if (dist < 4.0f || dist > 48.0f) continue;

            // Measure nearest opponent to receiver
            float minOppDist = 999.0f;
            for (const auto& opp : players) {
                if (opp.team != passer.team) {
                    float od = (opp.position - tm.position).length();
                    if (od < minOppDist) minOppDist = od;
                }
            }

            sf::Vector2f tmScreen = pitchToScreen(tm.position);
            if (minOppDist >= 4.0f) {
                // Open option
                drawScreenLine(window, passerScreen, tmScreen, 1.4f, sf::Color(241, 196, 15, 140));
                if (minOppDist > bestOpenness) {
                    bestOpenness = minOppDist;
                    bestReceiver = &tm;
                }
            } else if (minOppDist >= 2.0f) {
                // Contested option
                drawScreenLine(window, passerScreen, tmScreen, 1.0f, sf::Color(255, 255, 255, 80));
            }
        }

        // Highlight best open option with bright neon green
        if (bestReceiver) {
            sf::Vector2f bestScreen = pitchToScreen(bestReceiver->position);
            drawScreenLine(window, passerScreen, bestScreen, 2.5f, sf::Color(46, 204, 113, 230));

            float reticleRadius = metresToPixels(SimConfig::PLAYER_RADIUS_M * 1.5f);
            sf::CircleShape targetRing(reticleRadius);
            targetRing.setOrigin(sf::Vector2f(reticleRadius, reticleRadius));
            targetRing.setPosition(bestScreen);
            targetRing.setFillColor(sf::Color::Transparent);
            targetRing.setOutlineColor(sf::Color(46, 204, 113, 220));
            targetRing.setOutlineThickness(2.0f);
            window.draw(targetRing);
        }
        break;
    }
}

static std::string getTacticalRoleShortStr(TacticalRole role) {
    switch (role) {
        case TacticalRole::GK: return "GK";
        case TacticalRole::LB: return "LB";
        case TacticalRole::CB_LEFT:
        case TacticalRole::CB_RIGHT: return "CB";
        case TacticalRole::RB: return "RB";
        case TacticalRole::DM: return "CDM";
        case TacticalRole::CM_LEFT:
        case TacticalRole::CM_RIGHT: return "CM";
        case TacticalRole::LW: return "LW";
        case TacticalRole::RW: return "RW";
        case TacticalRole::ST: return "ST";
        default: return "";
    }
}

void PitchRenderer::drawPlayers(sf::RenderWindow& window, const std::vector<Player>& players, int selectedPlayerId, const TeamProfile* homeTeam, const TeamProfile* awayTeam) {
    float playerRadiusPx = metresToPixels(SimConfig::PLAYER_RADIUS_M);

    sf::Color homeColor = homeTeam ? homeTeam->primaryColor : m_homeColor;
    sf::Color homeGKColor = homeTeam ? homeTeam->gkColor : m_homeGKColor;
    sf::Color homeSecondary = homeTeam ? homeTeam->secondaryColor : sf::Color::White;

    sf::Color awayColor = awayTeam ? awayTeam->primaryColor : m_awayColor;
    sf::Color awayGKColor = awayTeam ? awayTeam->gkColor : m_awayGKColor;
    sf::Color awaySecondary = awayTeam ? awayTeam->secondaryColor : sf::Color::White;

    for (const auto& player : players) {
        if (player.isRedCarded) continue; // Sent off player is off the pitch

        sf::Vector2f screenPos = pitchToScreen(player.position);

        // 1. Dynamic Running Velocity Vector Arrow (Subtle speed trail)
        float currentSpeed = player.velocity.length();
        if (currentSpeed > 0.45f) {
            Vec2 normV = player.velocity / currentSpeed;
            sf::Vector2f vDir(normV.x, normV.y);
            float arrowLen = std::min(metresToPixels(3.2f), metresToPixels(currentSpeed * 0.35f));
            sf::Vector2f arrowEnd = screenPos + vDir * arrowLen;
            sf::Color arrowCol = (player.team == TeamSide::Home)
                ? sf::Color(255, 210, 210, std::min(170, static_cast<int>(currentSpeed * 22.0f)))
                : sf::Color(210, 235, 255, std::min(170, static_cast<int>(currentSpeed * 22.0f)));
            drawScreenLine(window, screenPos, arrowEnd, 1.4f, arrowCol);
        }

        // 2. Selected Player Reticle (Neon Green ring)
        if (player.id == selectedPlayerId) {
            sf::CircleShape selRing(playerRadiusPx * 2.2f);
            selRing.setOrigin(sf::Vector2f(playerRadiusPx * 2.2f, playerRadiusPx * 2.2f));
            selRing.setPosition(screenPos);
            selRing.setFillColor(sf::Color::Transparent);
            selRing.setOutlineColor(sf::Color(46, 204, 113, 230)); // Bright Neon Green
            selRing.setOutlineThickness(3.0f);
            window.draw(selRing);
        }

        // 3. Active Possession Ring & Aura (Gold)
        if (player.hasPossession) {
            sf::CircleShape aura(playerRadiusPx * 1.85f);
            aura.setOrigin(sf::Vector2f(playerRadiusPx * 1.85f, playerRadiusPx * 1.85f));
            aura.setPosition(screenPos);
            aura.setFillColor(sf::Color(241, 196, 15, 30));
            window.draw(aura);

            sf::CircleShape ring(playerRadiusPx * 1.55f);
            ring.setOrigin(sf::Vector2f(playerRadiusPx * 1.55f, playerRadiusPx * 1.55f));
            ring.setPosition(screenPos);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(241, 196, 15, 220));
            ring.setOutlineThickness(2.2f);
            window.draw(ring);
        }

        bool isGK = (player.role == TacticalRole::GK);
        sf::Color bodyColor;
        sf::Color outlineColor;
        sf::Color numColor;

        if (player.team == TeamSide::Home) {
            bodyColor = isGK ? homeGKColor : homeColor;
            outlineColor = homeSecondary;
            numColor = homeSecondary;
        } else {
            bodyColor = isGK ? awayGKColor : awayColor;
            outlineColor = awaySecondary;
            numColor = awaySecondary;
        }

        // 4. Directional Heading Indicator (Sleek sightline pointer on circumference)
        float headRad = player.headingDeg * (3.14159265f / 180.0f);
        sf::Vector2f headDir(std::cos(headRad), std::sin(headRad));
        sf::Vector2f headPerp(-headDir.y, headDir.x);

        sf::ConvexShape sightPointer(3);
        sightPointer.setPoint(0, screenPos + headDir * (playerRadiusPx + 3.0f));
        sightPointer.setPoint(1, screenPos + headDir * (playerRadiusPx * 0.85f) + headPerp * 3.2f);
        sightPointer.setPoint(2, screenPos + headDir * (playerRadiusPx * 0.85f) - headPerp * 3.2f);
        sightPointer.setFillColor(outlineColor);
        window.draw(sightPointer);

        // 5. Main Player Circular Disc
        sf::CircleShape circle(playerRadiusPx);
        circle.setOrigin(sf::Vector2f(playerRadiusPx, playerRadiusPx));
        circle.setPosition(screenPos);
        circle.setFillColor(bodyColor);
        circle.setOutlineColor(outlineColor);
        circle.setOutlineThickness(1.6f);
        window.draw(circle);

        // 6. Yellow Card Booking Badge (Law 12)
        if (player.yellowCards > 0) {
            sf::RectangleShape yCard(sf::Vector2f(5.0f, 7.0f));
            yCard.setPosition(sf::Vector2f(screenPos.x + playerRadiusPx * 0.70f, screenPos.y - playerRadiusPx * 0.90f));
            yCard.setFillColor(sf::Color(241, 196, 15)); // Vivid Yellow Caution
            yCard.setOutlineColor(sf::Color(15, 15, 15));
            yCard.setOutlineThickness(1.0f);
            window.draw(yCard);
        }

        if (m_fontLoaded) {
            unsigned int charSize = static_cast<unsigned int>(std::max(9.0f, playerRadiusPx * 1.15f));
            sf::Text numText(m_font, std::to_string(player.shirtNumber), charSize);
            numText.setFillColor(numColor);

            auto bounds = numText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
            numText.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x * 0.5f,
                                          bounds.position.y + bounds.size.y * 0.5f));
#else
            numText.setOrigin(sf::Vector2f(bounds.left + bounds.width * 0.5f,
                                          bounds.top + bounds.height * 0.5f));
#endif
            numText.setPosition(screenPos);
            window.draw(numText);

            // Floating Name & Role Pill Badge for active carrier or selected player
            if (player.id == selectedPlayerId || player.hasPossession) {
                std::string roleStr = getTacticalRoleShortStr(player.role);
                std::string fullLabel = player.name + " [" + roleStr + "]";
                sf::Text nameTag(m_font, fullLabel, 9);
                nameTag.setFillColor(player.hasPossession ? sf::Color(241, 196, 15) : sf::Color::White);
                auto nameBounds = nameTag.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
                nameTag.setOrigin(sf::Vector2f(nameBounds.position.x + nameBounds.size.x * 0.5f, nameBounds.position.y + nameBounds.size.y));
#else
                nameTag.setOrigin(sf::Vector2f(nameBounds.left + nameBounds.width * 0.5f, nameBounds.top + nameBounds.height));
#endif
                nameTag.setPosition(sf::Vector2f(screenPos.x, screenPos.y - playerRadiusPx - 4.5f));
                window.draw(nameTag);
            }
        }
    }
}

void PitchRenderer::drawBall(sf::RenderWindow& window, const Ball& ball) {
    float ballRadiusPx = std::max(3.5f, metresToPixels(SimConfig::BALL_RADIUS_M));
    sf::Vector2f screenPos = pitchToScreen(ball.position);

    float ballElevationPx = metresToPixels(ball.height * 0.95f);
    sf::Vector2f visualBallPos(screenPos.x, screenPos.y - ballElevationPx);

    // Record ball trail
    m_ballTrail.push_back(visualBallPos);
    if (m_ballTrail.size() > 14) m_ballTrail.pop_front();

    // 1. Realistic ground shadow (offset and soft alpha scaling with height)
    float shadowOffset = 2.0f + ball.height * 2.2f;
    float shadowRadius = ballRadiusPx * (1.05f + ball.height * 0.08f);
    sf::CircleShape shadow(shadowRadius);
    shadow.setOrigin(sf::Vector2f(shadowRadius, shadowRadius));
    shadow.setPosition(sf::Vector2f(screenPos.x + shadowOffset, screenPos.y + shadowOffset));
    shadow.setFillColor(sf::Color(0, 0, 0, std::max(20, 100 - static_cast<int>(ball.height * 18.0f))));
    window.draw(shadow);

    // 2. Ball sphere (raised by elevation)
    sf::CircleShape ballShape(ballRadiusPx);
    ballShape.setOrigin(sf::Vector2f(ballRadiusPx, ballRadiusPx));
    ballShape.setPosition(visualBallPos);
    ballShape.setFillColor(sf::Color(255, 255, 255));
    ballShape.setOutlineColor(sf::Color(35, 35, 35));
    ballShape.setOutlineThickness(1.4f);
    window.draw(ballShape);

    // 3. Subtle pentagon seam center dot
    sf::CircleShape seam(ballRadiusPx * 0.32f);
    seam.setOrigin(sf::Vector2f(ballRadiusPx * 0.32f, ballRadiusPx * 0.32f));
    seam.setPosition(visualBallPos);
    seam.setFillColor(sf::Color(45, 45, 45, 180));
    window.draw(seam);
}

void PitchRenderer::drawHUD(
    sf::RenderWindow& window,
    int homeScore,
    int awayScore,
    float matchClockSeconds,
    const MatchAnalytics& analytics,
    MatchPhase phase,
    float firstHalfAddedTime,
    float secondHalfAddedTime,
    TeamTacticalStyle homeStyle,
    TeamTacticalStyle awayStyle,
    float simSpeed,
    bool showTacticalLines,
    const TeamProfile* homeTeam,
    const TeamProfile* awayTeam,
    FormationType homeFormation,
    FormationType awayFormation,
    bool inRealMatchMode,
    const std::string& realMatchEventBanner
) {
    float winW = static_cast<float>(window.getSize().x);

    sf::RectangleShape topBar(sf::Vector2f(winW, 54.0f));
    topBar.setFillColor(sf::Color(18, 22, 28));
    window.draw(topBar);

    if (!m_fontLoaded) return;

    if (inRealMatchMode) {
        // App Branding
        sf::Text titleText(m_font, "FUT ANALYTICS [REAL MATCH]", 13);
        titleText.setFillColor(sf::Color(241, 196, 15));
        titleText.setPosition(sf::Vector2f(20.0f, 10.0f));
        window.draw(titleText);

        std::ostringstream speedSs;
        speedSs << "METRICA MATCH 1 (25Hz) | SPD: " << std::fixed << std::setprecision(1) << simSpeed
                << "x | [<- / ->] Scrub 10s | [M] Toggle Live AI | [SPACE] Pause";
        sf::Text speedText(m_font, speedSs.str(), 10);
        speedText.setFillColor(sf::Color(46, 204, 113));
        speedText.setPosition(sf::Vector2f(230.0f, 12.0f));
        window.draw(speedText);

        // Home team label
        sf::Text homeText(m_font, "HOME (Real Tracking)", 11);
        homeText.setFillColor(sf::Color(235, 77, 75));
        homeText.setPosition(sf::Vector2f(20.0f, 30.0f));
        window.draw(homeText);

        // Clock
        int minutes = static_cast<int>(matchClockSeconds) / 60;
        int seconds = static_cast<int>(matchClockSeconds) % 60;
        std::ostringstream clockSs;
        clockSs << std::setfill('0') << std::setw(2) << minutes << ":"
                << std::setfill('0') << std::setw(2) << seconds << " (REAL MATCH)";
        sf::Text clockText(m_font, clockSs.str(), 13);
        clockText.setFillColor(sf::Color(241, 196, 15));
        auto clockBounds = clockText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
        clockText.setOrigin(sf::Vector2f(clockBounds.position.x + clockBounds.size.x * 0.5f,
                                        clockBounds.position.y + clockBounds.size.y * 0.5f));
#else
        clockText.setOrigin(sf::Vector2f(clockBounds.left + clockBounds.width * 0.5f,
                                        clockBounds.top + clockBounds.height * 0.5f));
#endif
        clockText.setPosition(sf::Vector2f(winW * 0.5f, 13.0f));
        window.draw(clockText);

        // Score
        std::ostringstream scoreSs;
        scoreSs << "HOME  " << homeScore << " - " << awayScore << "  AWAY";
        sf::Text scoreText(m_font, scoreSs.str(), 16);
        scoreText.setFillColor(sf::Color::White);
        auto scoreBounds = scoreText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
        scoreText.setOrigin(sf::Vector2f(scoreBounds.position.x + scoreBounds.size.x * 0.5f,
                                        scoreBounds.position.y + scoreBounds.size.y * 0.5f));
#else
        scoreText.setOrigin(sf::Vector2f(scoreBounds.left + scoreBounds.width * 0.5f,
                                        scoreBounds.top + scoreBounds.height * 0.5f));
#endif
        scoreText.setPosition(sf::Vector2f(winW * 0.5f, 30.0f));
        window.draw(scoreText);

        // Right side: Away label
        sf::Text awayText(m_font, "AWAY (Real Tracking)", 11);
        awayText.setFillColor(sf::Color(100, 180, 246));
        auto awayBounds = awayText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
        float aW = awayBounds.size.x;
#else
        float aW = awayBounds.width;
#endif
        awayText.setPosition(sf::Vector2f(winW - aW - 20.0f, 30.0f));
        window.draw(awayText);

        // Draw live real-match event banner if active
        if (!realMatchEventBanner.empty()) {
            sf::RectangleShape evBanner(sf::Vector2f(winW, 26.0f));
            evBanner.setPosition(sf::Vector2f(0.0f, 54.0f));
            evBanner.setFillColor(sf::Color(22, 32, 46, 230));
            evBanner.setOutlineColor(sf::Color(241, 196, 15, 180));
            evBanner.setOutlineThickness(1.0f);
            window.draw(evBanner);

            sf::Text evText(m_font, realMatchEventBanner, 11);
            evText.setFillColor(sf::Color(241, 196, 15));
            auto evBounds = evText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
            evText.setOrigin(sf::Vector2f(evBounds.position.x + evBounds.size.x * 0.5f, evBounds.position.y + evBounds.size.y * 0.5f));
#else
            evText.setOrigin(sf::Vector2f(evBounds.left + evBounds.width * 0.5f, evBounds.top + evBounds.height * 0.5f));
#endif
            evText.setPosition(sf::Vector2f(winW * 0.5f, 67.0f));
            window.draw(evText);
        }
        return;
    }

    // App Branding
    sf::Text titleText(m_font, "FUT ANALYTICS", 13);
    titleText.setFillColor(sf::Color(52, 152, 219));
    titleText.setPosition(sf::Vector2f(20.0f, 10.0f));
    window.draw(titleText);

    // Speed, Lines and Matchup card status
    std::ostringstream speedSs;
    speedSs << "SPD: " << std::fixed << std::setprecision(1) << simSpeed << "x [O:1x I:4x F:8x] | [L] LINES: "
            << (showTacticalLines ? "ON" : "OFF") << " | [V] MATCHUP CARD | [Z/X] TEAMS";
    sf::Text speedText(m_font, speedSs.str(), 10);
    speedText.setFillColor(sf::Color(155, 190, 230));
    speedText.setPosition(sf::Vector2f(140.0f, 12.0f));
    window.draw(speedText);

    // Empirical Trained Model Indicator
    sf::Text trainedBadge(m_font, "[AI: TRAINED ON 105 REAL MATCHES]", 10);
    trainedBadge.setFillColor(sf::Color(46, 204, 113));
    trainedBadge.setPosition(sf::Vector2f(660.0f, 12.0f));
    window.draw(trainedBadge);

    // Home tactical style and club name
    std::string homeClubShort = homeTeam ? homeTeam->shortName : "HOME";
    std::string homeTacticStr = homeClubShort + " [" + Tactics::formationToString(homeFormation) + "]: " + Tactics::tacticalStyleToString(homeStyle);
    sf::Text homeTacticText(m_font, homeTacticStr, 11);
    homeTacticText.setFillColor(homeTeam ? homeTeam->primaryColor : sf::Color(235, 77, 75));
    homeTacticText.setPosition(sf::Vector2f(20.0f, 30.0f));
    window.draw(homeTacticText);

    // Match clock with period & stoppage time
    int minutes = static_cast<int>(matchClockSeconds) / 60;
    int seconds = static_cast<int>(matchClockSeconds) % 60;
    std::ostringstream clockSs;

    int addedMins1 = static_cast<int>(firstHalfAddedTime / 60.0f);
    int addedMins2 = static_cast<int>(secondHalfAddedTime / 60.0f);

    if (phase == MatchPhase::HalfTime) {
        clockSs << "45:00 +" << addedMins1 << "' (HALF TIME - [SPACE])";
    } else if (phase == MatchPhase::FullTime) {
        clockSs << "90:00 +" << addedMins2 << "' (FULL TIME)";
    } else if (matchClockSeconds < 2700.0f) {
        clockSs << std::setfill('0') << std::setw(2) << minutes << ":"
                << std::setfill('0') << std::setw(2) << seconds << " (1ST HALF)";
    } else if (matchClockSeconds < 2700.0f + firstHalfAddedTime) {
        int addedSec = static_cast<int>(matchClockSeconds - 2700.0f);
        clockSs << "45:00 +" << (addedSec / 60) << ":" << std::setfill('0') << std::setw(2) << (addedSec % 60) << " (1ST HALF +" << addedMins1 << "')";
    } else if (matchClockSeconds < 5400.0f) {
        clockSs << std::setfill('0') << std::setw(2) << minutes << ":"
                << std::setfill('0') << std::setw(2) << seconds << " (2ND HALF)";
    } else {
        int addedSec = static_cast<int>(matchClockSeconds - 5400.0f);
        clockSs << "90:00 +" << (addedSec / 60) << ":" << std::setfill('0') << std::setw(2) << (addedSec % 60) << " (2ND HALF +" << addedMins2 << "')";
    }

    sf::Text clockText(m_font, clockSs.str(), 13);
    clockText.setFillColor(phase == MatchPhase::HalfTime || phase == MatchPhase::FullTime ? sf::Color(241, 196, 15) : sf::Color(46, 204, 113));
    auto clockBounds = clockText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
    clockText.setOrigin(sf::Vector2f(clockBounds.position.x + clockBounds.size.x * 0.5f,
                                    clockBounds.position.y + clockBounds.size.y * 0.5f));
#else
    clockText.setOrigin(sf::Vector2f(clockBounds.left + clockBounds.width * 0.5f,
                                    clockBounds.top + clockBounds.height * 0.5f));
#endif
    clockText.setPosition(sf::Vector2f(winW * 0.5f, 13.0f));
    window.draw(clockText);

    // Score with authentic club abbreviations
    std::string awayClubShort = awayTeam ? awayTeam->shortName : "AWAY";
    std::ostringstream scoreSs;
    scoreSs << homeClubShort << "  " << homeScore << " - " << awayScore << "  " << awayClubShort;
    sf::Text scoreText(m_font, scoreSs.str(), 16);
    scoreText.setFillColor(sf::Color::White);
    auto scoreBounds = scoreText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
    scoreText.setOrigin(sf::Vector2f(scoreBounds.position.x + scoreBounds.size.x * 0.5f,
                                    scoreBounds.position.y + scoreBounds.size.y * 0.5f));
#else
    scoreText.setOrigin(sf::Vector2f(scoreBounds.left + scoreBounds.width * 0.5f,
                                    scoreBounds.top + scoreBounds.height * 0.5f));
#endif
    scoreText.setPosition(sf::Vector2f(winW * 0.5f, 30.0f));
    window.draw(scoreText);

    std::ostringstream xgSs;
    xgSs << std::fixed << std::setprecision(2);
    xgSs << "xG: " << analytics.getHomeXG() << " - " << analytics.getAwayXG();
    sf::Text xgText(m_font, xgSs.str(), 10);
    xgText.setFillColor(sf::Color(180, 195, 210));
    auto xgBounds = xgText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
    xgText.setOrigin(sf::Vector2f(xgBounds.position.x + xgBounds.size.x * 0.5f,
                                 xgBounds.position.y + xgBounds.size.y * 0.5f));
#else
    xgText.setOrigin(sf::Vector2f(xgBounds.left + xgBounds.width * 0.5f,
                                 xgBounds.top + xgBounds.height * 0.5f));
#endif
    xgText.setPosition(sf::Vector2f(winW * 0.5f, 44.0f));
    window.draw(xgText);

    // Right side: Possession & Away tactical style
    std::ostringstream possSs;
    possSs << "Possession: " << static_cast<int>(analytics.getHomePossessionPct()) << "% - "
           << static_cast<int>(analytics.getAwayPossessionPct()) << "%";
    sf::Text possText(m_font, possSs.str(), 12);
    possText.setFillColor(sf::Color(241, 196, 15));
    auto possBounds = possText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
    float pW = possBounds.size.x;
#else
    float pW = possBounds.width;
#endif
    possText.setPosition(sf::Vector2f(winW - pW - 20.0f, 10.0f));
    window.draw(possText);

    std::string awayTacticStr = awayClubShort + " [" + Tactics::formationToString(awayFormation) + "]: " + Tactics::tacticalStyleToString(awayStyle);
    sf::Text awayTacticText(m_font, awayTacticStr, 11);
    awayTacticText.setFillColor(awayTeam ? awayTeam->primaryColor : sf::Color(100, 180, 246));
    auto awayBounds = awayTacticText.getLocalBounds();
#if SFML_VERSION_MAJOR >= 3
    float aW = awayBounds.size.x;
#else
    float aW = awayBounds.width;
#endif
    awayTacticText.setPosition(sf::Vector2f(winW - aW - 20.0f, 30.0f));
    window.draw(awayTacticText);
}

void PitchRenderer::drawCalibrationPanel(sf::RenderWindow& window, const MatchAnalytics& analytics, int homeScore, int awayScore, float matchTime) {
    if (!m_fontLoaded) return;

    CalibrationMetrics cm = analytics.getCalibrationMetrics(homeScore, awayScore, matchTime);

    float panelW = 320.0f;
    float panelH = 175.0f;
    float panelX = 20.0f;
    float panelY = 62.0f;

    sf::RectangleShape bg(sf::Vector2f(panelW, panelH));
    bg.setPosition(sf::Vector2f(panelX, panelY));
    bg.setFillColor(sf::Color(18, 24, 32, 235));
    bg.setOutlineColor(sf::Color(46, 204, 113, 180));
    bg.setOutlineThickness(1.5f);
    window.draw(bg);

    sf::Text header(m_font, "CALIBRATION BENCHMARKS [K]", 11);
    header.setFillColor(sf::Color(46, 204, 113));
    header.setPosition(sf::Vector2f(panelX + 12.0f, panelY + 8.0f));
    window.draw(header);

    sf::Text colHeader(m_font, "Metric                  Simulated     Target", 10);
    colHeader.setFillColor(sf::Color(140, 150, 165));
    colHeader.setPosition(sf::Vector2f(panelX + 12.0f, panelY + 26.0f));
    window.draw(colHeader);

    struct Row {
        std::string label;
        std::string sim;
        std::string target;
        sf::Color color;
    };

    std::ostringstream sGoals, sRegain, sPress, sGK, sSetPiece, sPassSeq, sShots;
    sGoals << std::fixed << std::setprecision(2) << cm.simulatedGoalsPerMatch;
    sRegain << std::fixed << std::setprecision(1) << cm.simulatedRegainTimeDiffS << "s";
    sPress << std::fixed << std::setprecision(1) << cm.simulatedPressTransitionPct << "%";
    sGK << std::fixed << std::setprecision(1) << cm.simulatedGKGoalKickPct << "%";
    sSetPiece << std::fixed << std::setprecision(1) << cm.simulatedSetPieceGoalPct << "%";
    sPassSeq << std::fixed << std::setprecision(1) << cm.simulatedAvgPassSequenceS << "s";
    sShots << static_cast<int>(cm.simulatedTotalShots) << " / " << std::fixed << std::setprecision(2) << cm.simulatedAvgXG;

    std::vector<Row> rows = {
        {"Goals / Match:", sGoals.str(), "2.7 - 3.0", sf::Color::White},
        {"Winner Regain Diff:", sRegain.str(), "~4.0s", sf::Color(241, 196, 15)},
        {"Press Transitions:", sPress.str(), "~40%", sf::Color(52, 152, 219)},
        {"GK Goal Kicks:", sGK.str(), "~50%", sf::Color::White},
        {"Set Piece Goals:", sSetPiece.str(), "25 - 27%", sf::Color(235, 77, 75)},
        {"Avg Pass Sequence:", sPassSeq.str(), "9 - 10s", sf::Color(46, 204, 113)},
        {"Shots / avg xG:", sShots.str(), "25 / 0.10", sf::Color(180, 195, 210)}
    };

    float rowY = panelY + 44.0f;
    for (const auto& r : rows) {
        sf::Text lbl(m_font, r.label, 9);
        lbl.setFillColor(sf::Color(200, 205, 215));
        lbl.setPosition(sf::Vector2f(panelX + 12.0f, rowY));
        window.draw(lbl);

        sf::Text val(m_font, r.sim, 9);
        val.setFillColor(r.color);
        val.setPosition(sf::Vector2f(panelX + 145.0f, rowY));
        window.draw(val);

        sf::Text tgt(m_font, r.target, 9);
        tgt.setFillColor(sf::Color(140, 150, 165));
        tgt.setPosition(sf::Vector2f(panelX + 225.0f, rowY));
        window.draw(tgt);

        rowY += 17.0f;
    }
}

void PitchRenderer::drawMomentumAndStatsPanel(
    sf::RenderWindow& window,
    const MatchAnalytics& analytics,
    const EventRecorder& eventRecorder,
    int selectedPlayerId,
    bool showHeatmap,
    bool showPassingNetwork,
    bool cameraFollowBall,
    const std::vector<Player>& players,
    const SpatialAnalytics& spatialAnalytics
) {
    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);
    float panelHeight = 115.0f;
    float panelY = winH - panelHeight;

    sf::RectangleShape panelBg(sf::Vector2f(winW, panelHeight));
    panelBg.setPosition(sf::Vector2f(0.0f, panelY));
    panelBg.setFillColor(sf::Color(22, 27, 34));
    window.draw(panelBg);

    if (!m_fontLoaded) return;

    // --- LEFT COLUMN: LIVE STATS SUMMARY ---
    float statsColX = 15.0f;
    sf::Text statsHeader(m_font, "MATCH STATS", 12);
    statsHeader.setFillColor(sf::Color(140, 150, 165));
    statsHeader.setPosition(sf::Vector2f(statsColX, panelY + 10.0f));
    window.draw(statsHeader);

    int homePasses = eventRecorder.getPassCount(TeamSide::Home);
    int homePassesComp = eventRecorder.getPassCount(TeamSide::Home, true);
    int awayPasses = eventRecorder.getPassCount(TeamSide::Away);
    int awayPassesComp = eventRecorder.getPassCount(TeamSide::Away, true);

    int homeShots = eventRecorder.getShotCount(TeamSide::Home);
    int homeShotsOnTarget = eventRecorder.getShotCount(TeamSide::Home, true);
    int awayShots = eventRecorder.getShotCount(TeamSide::Away);
    int awayShotsOnTarget = eventRecorder.getShotCount(TeamSide::Away, true);

    int homeCorners = eventRecorder.getCornerCount(TeamSide::Home);
    int awayCorners = eventRecorder.getCornerCount(TeamSide::Away);

    int homeFouls = eventRecorder.getFoulCount(TeamSide::Home);
    int awayFouls = eventRecorder.getFoulCount(TeamSide::Away);

    int homeY = eventRecorder.getYellowCardCount(TeamSide::Home);
    int awayY = eventRecorder.getYellowCardCount(TeamSide::Away);
    int homeR = eventRecorder.getRedCardCount(TeamSide::Home);
    int awayR = eventRecorder.getRedCardCount(TeamSide::Away);

    std::ostringstream s1, s2, s3, s4;
    int hPct = (homePasses > 0) ? (homePassesComp * 100 / homePasses) : 0;
    int aPct = (awayPasses > 0) ? (awayPassesComp * 100 / awayPasses) : 0;

    s1 << "Passes:   " << homePassesComp << "/" << homePasses << " (" << hPct << "%)  vs  "
       << awayPassesComp << "/" << awayPasses << " (" << aPct << "%)";
    s2 << std::fixed << std::setprecision(2)
       << "xG (OT):   " << analytics.getHomeXG() << " (" << homeShotsOnTarget << ")  vs  "
       << analytics.getAwayXG() << " (" << awayShotsOnTarget << ")";
    s3 << "Shots (Corn): " << homeShots << " (" << homeCorners << ")  vs  " << awayShots << " (" << awayCorners << ")";
    s4 << "Fouls (Cards): " << homeFouls << " (" << homeY << "Y " << homeR << "R)  vs  "
       << awayFouls << " (" << awayY << "Y " << awayR << "R)";

    sf::Text t1(m_font, s1.str(), 10);
    t1.setFillColor(sf::Color::White);
    t1.setPosition(sf::Vector2f(statsColX, panelY + 28.0f));
    window.draw(t1);

    sf::Text t2(m_font, s2.str(), 10);
    t2.setFillColor(sf::Color::White);
    t2.setPosition(sf::Vector2f(statsColX, panelY + 48.0f));
    window.draw(t2);

    sf::Text t3(m_font, s3.str(), 10);
    t3.setFillColor(sf::Color::White);
    t3.setPosition(sf::Vector2f(statsColX, panelY + 68.0f));
    window.draw(t3);

    sf::Text t4(m_font, s4.str(), 10);
    t4.setFillColor(sf::Color(241, 196, 15));
    t4.setPosition(sf::Vector2f(statsColX, panelY + 88.0f));
    window.draw(t4);

    // --- CENTER-LEFT COLUMN: FUT MATCH MOMENTUM GRAPH ---
    float graphX = 285.0f;
    float graphWidth = 345.0f;
    float graphCenterY = panelY + 58.0f;
    float maxGraphHeight = 38.0f;

    sf::Text momentumTitle(m_font, "FUT MATCH MOMENTUM", 12);
    momentumTitle.setFillColor(sf::Color(140, 150, 165));
    momentumTitle.setPosition(sf::Vector2f(graphX, panelY + 10.0f));
    window.draw(momentumTitle);

    sf::RectangleShape midLine(sf::Vector2f(graphWidth, 1.5f));
    midLine.setPosition(sf::Vector2f(graphX, graphCenterY));
    midLine.setFillColor(sf::Color(70, 80, 95));
    window.draw(midLine);

    const auto& history = analytics.getMomentumHistory();
    if (!history.empty()) {
        float barWidth = std::max(2.0f, graphWidth / 180.0f);

        for (size_t i = 0; i < history.size(); ++i) {
            float xPos = graphX + (static_cast<float>(i) * barWidth);
            float val = history[i].momentumValue;
            float barH = (std::abs(val) / 100.0f) * maxGraphHeight;

            sf::RectangleShape bar;
            if (val >= 0.0f) {
                bar.setSize(sf::Vector2f(barWidth - 0.5f, barH));
                bar.setPosition(sf::Vector2f(xPos, graphCenterY - barH));
                bar.setFillColor(sf::Color(218, 41, 28, 220));
            } else {
                bar.setSize(sf::Vector2f(barWidth - 0.5f, barH));
                bar.setPosition(sf::Vector2f(xPos, graphCenterY));
                bar.setFillColor(sf::Color(41, 128, 185, 220));
            }
            window.draw(bar);
        }
    }

    // --- CENTER-RIGHT COLUMN: TACTICAL HEATMAP MINI-PITCH (Below Pitch!) ---
    const Player* selPlayer = nullptr;
    for (const auto& p : players) {
        if (p.id == selectedPlayerId) {
            selPlayer = &p;
            break;
        }
    }

    float hmX = 645.0f;
    float hmY = panelY + 22.0f;
    float hmW = 185.0f;
    float hmH = 82.0f;

    std::string hmLabel = selPlayer ? ("HEATMAP: #" + std::to_string(selPlayer->shirtNumber) + " " + selPlayer->name) : "TACTICAL HEATMAP";
    sf::Text hmHeader(m_font, hmLabel, 10);
    hmHeader.setFillColor(sf::Color(140, 150, 165));
    hmHeader.setPosition(sf::Vector2f(hmX, panelY + 8.0f));
    window.draw(hmHeader);

    if (showHeatmap && selectedPlayerId != -1) {
        drawMiniPitchHeatmap(window, spatialAnalytics, selectedPlayerId, hmX, hmY, hmW, hmH);
    } else {
        drawMiniPitchHeatmap(window, spatialAnalytics, -1, hmX, hmY, hmW, hmH);
        sf::Text prompt(m_font, showHeatmap ? "Click player to view" : "[H] Heatmap is OFF", 9);
        prompt.setFillColor(sf::Color(180, 190, 200, 190));
        prompt.setPosition(sf::Vector2f(hmX + 22.0f, hmY + 32.0f));
        window.draw(prompt);
    }

    // --- RIGHT COLUMN: PLAYER FC IQ PROFILE & ATTRIBUTES ---
    float ctrlX = 845.0f;

    if (selPlayer) {
        std::string teamStr = (selPlayer->team == TeamSide::Home) ? "HOME" : "AWAY";
        std::ostringstream pHeader;
        pHeader << "#" << selPlayer->shirtNumber << " " << selPlayer->name
                << " [OVR " << selPlayer->attributes.overallRating << "] (" << teamStr << ")";
        if (selPlayer->yellowCards > 0) {
            pHeader << " [" << selPlayer->yellowCards << "Y]";
        }
        if (selPlayer->isRedCarded) {
            pHeader << " [RED]";
        }
        sf::Text pHeadText(m_font, pHeader.str(), 11);
        pHeadText.setFillColor(selPlayer->isRedCarded ? sf::Color(231, 76, 60) :
                              (selPlayer->team == TeamSide::Home ? sf::Color(235, 77, 75) : sf::Color(100, 180, 246)));
        pHeadText.setPosition(sf::Vector2f(ctrlX, panelY + 8.0f));
        window.draw(pHeadText);

        std::string roleStr = "Role: " + Tactics::fcIqRoleToString(selPlayer->fcIqRole);
        sf::Text rText(m_font, roleStr, 10);
        rText.setFillColor(sf::Color(241, 196, 15));
        rText.setPosition(sf::Vector2f(ctrlX, panelY + 26.0f));
        window.draw(rText);

        std::ostringstream attrSs1;
        if (selPlayer->role == TacticalRole::GK) {
            attrSs1 << "REF: " << static_cast<int>(selPlayer->attributes.gkReflexes * 100.0f)
                    << "  DIV: " << static_cast<int>(selPlayer->attributes.gkDiving * 100.0f)
                    << "  HAN: " << static_cast<int>(selPlayer->attributes.gkHandling * 100.0f);
        } else {
            attrSs1 << "PAC: " << static_cast<int>(selPlayer->attributes.pace)
                    << "  SHO: " << static_cast<int>(selPlayer->attributes.shooting)
                    << "  PAS: " << static_cast<int>(selPlayer->attributes.passing)
                    << "  DRI: " << static_cast<int>(selPlayer->attributes.dribbling);
        }
        sf::Text a1(m_font, attrSs1.str(), 10);
        a1.setFillColor(sf::Color::White);
        a1.setPosition(sf::Vector2f(ctrlX, panelY + 44.0f));
        window.draw(a1);

        std::ostringstream attrSs2;
        if (selPlayer->role == TacticalRole::GK) {
            attrSs2 << "PAS: " << static_cast<int>(selPlayer->attributes.passing)
                    << "  POS: " << static_cast<int>(selPlayer->attributes.gkPositioning * 100.0f)
                    << "  CMP: " << static_cast<int>(selPlayer->attributes.composure * 100.0f);
        } else {
            attrSs2 << "DEF: " << static_cast<int>(selPlayer->attributes.defending)
                    << "  PHY: " << static_cast<int>(selPlayer->attributes.physical)
                    << "  FIN: " << static_cast<int>(selPlayer->attributes.finishing * 100.0f)
                    << "  VIS: " << static_cast<int>(selPlayer->attributes.vision * 100.0f);
        }
        sf::Text a2(m_font, attrSs2.str(), 10);
        a2.setFillColor(sf::Color::White);
        a2.setPosition(sf::Vector2f(ctrlX, panelY + 62.0f));
        window.draw(a2);

        std::ostringstream ovrSs;
        ovrSs << "[H] Heat:" << (showHeatmap ? "ON" : "OFF")
              << " [N] Net:" << (showPassingNetwork ? "ON" : "OFF")
              << " [C] Cam:" << (cameraFollowBall ? "ON" : "OFF");
        sf::Text ovrText(m_font, ovrSs.str(), 9);
        ovrText.setFillColor(sf::Color(140, 150, 165));
        ovrText.setPosition(sf::Vector2f(ctrlX, panelY + 80.0f));
        window.draw(ovrText);

        std::ostringstream fitSs;
        fitSs << "Dist: " << static_cast<int>(selPlayer->distanceCoveredM) << "m"
              << "  Sprint: " << static_cast<int>(selPlayer->sprintDistanceM) << "m"
              << "  Energy: " << static_cast<int>(selPlayer->fatigue * 100.0f) << "%";
        sf::Text fitText(m_font, fitSs.str(), 9);
        fitText.setFillColor(sf::Color(46, 204, 113));
        fitText.setPosition(sf::Vector2f(ctrlX, panelY + 96.0f));
        window.draw(fitText);
    } else {
        sf::Text cHead(m_font, "CONTROLS & TACTICS", 11);
        cHead.setFillColor(sf::Color(140, 150, 165));
        cHead.setPosition(sf::Vector2f(ctrlX, panelY + 8.0f));
        window.draw(cHead);

        sf::Text l1(m_font, "Click any player to inspect FC IQ & ratings", 10);
        l1.setFillColor(sf::Color::White);
        l1.setPosition(sf::Vector2f(ctrlX, panelY + 26.0f));
        window.draw(l1);

        sf::Text l2(m_font, "[Z] Cycle Home Club  |  [X] Cycle Away Club", 10);
        l2.setFillColor(sf::Color(241, 196, 15));
        l2.setPosition(sf::Vector2f(ctrlX, panelY + 44.0f));
        window.draw(l2);

        sf::Text l3(m_font, "[V] Matchup Card  |  [T/Y] Tactics  |  [1-4/7-0] Form", 10);
        l3.setFillColor(sf::Color(52, 152, 219));
        l3.setPosition(sf::Vector2f(ctrlX, panelY + 62.0f));
        window.draw(l3);

        sf::Text l4(m_font, "[H] Heatmap  [L] Lines  [N] PassNet  [C] Cam  [R] Reset", 10);
        l4.setFillColor(sf::Color(46, 204, 113));
        l4.setPosition(sf::Vector2f(ctrlX, panelY + 80.0f));
        window.draw(l4);
    }
}

void PitchRenderer::drawTeamDetailsPanel(sf::RenderWindow& window, const TeamProfile& homeTeam, const TeamProfile& awayTeam) {
    if (!m_fontLoaded) return;

    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    float panelW = 780.0f;
    float panelH = 390.0f;
    float panelX = (winW - panelW) * 0.5f;
    float panelY = (winH - panelH) * 0.5f - 20.0f;

    // Dark card background
    sf::RectangleShape cardBg(sf::Vector2f(panelW, panelH));
    cardBg.setPosition(sf::Vector2f(panelX, panelY));
    cardBg.setFillColor(sf::Color(15, 20, 28, 248));
    cardBg.setOutlineColor(sf::Color(52, 152, 219, 220));
    cardBg.setOutlineThickness(2.0f);
    window.draw(cardBg);

    // Title header bar
    sf::RectangleShape headerBar(sf::Vector2f(panelW, 36.0f));
    headerBar.setPosition(sf::Vector2f(panelX, panelY));
    headerBar.setFillColor(sf::Color(24, 32, 45, 240));
    window.draw(headerBar);

    sf::Text title(m_font, "TACTICAL MATCHUP & TEAM STRENGTHS / WEAKNESSES  [V: Close | Z: Home | X: Away]", 12);
    title.setFillColor(sf::Color(241, 196, 15));
    title.setPosition(sf::Vector2f(panelX + 16.0f, panelY + 10.0f));
    window.draw(title);

    // Two Columns: Left for Home, Right for Away
    auto renderClubCard = [&](float startX, const TeamProfile& team) {
        // Club Header
        std::string teamHeader = team.name + " (" + team.league + ")";
        sf::Text clubText(m_font, teamHeader, 13);
        clubText.setFillColor(team.primaryColor);
        clubText.setPosition(sf::Vector2f(startX, panelY + 44.0f));
        window.draw(clubText);

        // Manager & Tactical Style
        std::ostringstream mgrSs;
        mgrSs << "Manager: " << team.manager << " | " << Tactics::formationToString(team.defaultFormation);
        sf::Text mgrText(m_font, mgrSs.str(), 10);
        mgrText.setFillColor(sf::Color(200, 210, 225));
        mgrText.setPosition(sf::Vector2f(startX, panelY + 66.0f));
        window.draw(mgrText);

        std::string styleLine = "Identity: " + team.styleDescription;
        sf::Text styleText(m_font, styleLine, 10);
        styleText.setFillColor(sf::Color(140, 160, 185));
        styleText.setPosition(sf::Vector2f(startX, panelY + 84.0f));
        window.draw(styleText);

        // Kit Swatches
        sf::RectangleShape kitPrimary(sf::Vector2f(16.0f, 16.0f));
        kitPrimary.setPosition(sf::Vector2f(startX, panelY + 104.0f));
        kitPrimary.setFillColor(team.primaryColor);
        kitPrimary.setOutlineColor(sf::Color::White);
        kitPrimary.setOutlineThickness(1.0f);
        window.draw(kitPrimary);

        sf::RectangleShape kitSecondary(sf::Vector2f(16.0f, 16.0f));
        kitSecondary.setPosition(sf::Vector2f(startX + 22.0f, panelY + 104.0f));
        kitSecondary.setFillColor(team.secondaryColor);
        kitSecondary.setOutlineColor(sf::Color::White);
        kitSecondary.setOutlineThickness(1.0f);
        window.draw(kitSecondary);

        sf::RectangleShape kitGk(sf::Vector2f(16.0f, 16.0f));
        kitGk.setPosition(sf::Vector2f(startX + 44.0f, panelY + 104.0f));
        kitGk.setFillColor(team.gkColor);
        kitGk.setOutlineColor(sf::Color::White);
        kitGk.setOutlineThickness(1.0f);
        window.draw(kitGk);

        sf::Text kitLabel(m_font, "Club Kit & GK Colors", 9);
        kitLabel.setFillColor(sf::Color(140, 150, 165));
        kitLabel.setPosition(sf::Vector2f(startX + 68.0f, panelY + 106.0f));
        window.draw(kitLabel);

        // STRENGTHS (Green)
        sf::Text strHeader(m_font, "TACTICAL STRENGTHS (+)", 11);
        strHeader.setFillColor(sf::Color(46, 204, 113));
        strHeader.setPosition(sf::Vector2f(startX, panelY + 128.0f));
        window.draw(strHeader);

        float curY = panelY + 146.0f;
        for (const auto& s : team.strengths) {
            std::string item = "+ " + s;
            sf::Text itemText(m_font, item, 9);
            itemText.setFillColor(sf::Color(220, 245, 230));
            itemText.setPosition(sf::Vector2f(startX + 6.0f, curY));
            window.draw(itemText);
            curY += 18.0f;
        }

        // WEAKNESSES (Red)
        curY += 6.0f;
        sf::Text weakHeader(m_font, "EXPOSED WEAKNESSES (-)", 11);
        weakHeader.setFillColor(sf::Color(235, 77, 75));
        weakHeader.setPosition(sf::Vector2f(startX, curY));
        window.draw(weakHeader);
        curY += 18.0f;

        for (const auto& w : team.weaknesses) {
            std::string item = "- " + w;
            sf::Text itemText(m_font, item, 9);
            itemText.setFillColor(sf::Color(255, 215, 215));
            itemText.setPosition(sf::Vector2f(startX + 6.0f, curY));
            window.draw(itemText);
            curY += 18.0f;
        }
    };

    renderClubCard(panelX + 16.0f, homeTeam);

    // Vertical Divider
    sf::RectangleShape div(sf::Vector2f(1.0f, panelH - 48.0f));
    div.setPosition(sf::Vector2f(panelX + panelW * 0.5f, panelY + 42.0f));
    div.setFillColor(sf::Color(50, 60, 75));
    window.draw(div);

    renderClubCard(panelX + panelW * 0.5f + 16.0f, awayTeam);

    // Footer Instruction
    sf::Text footer(m_font, "Press [Z] to Cycle Home Club (10 Teams) | [X] to Cycle Away Club | [V] to Toggle This Card", 10);
    footer.setFillColor(sf::Color(140, 160, 185));
    footer.setPosition(sf::Vector2f(panelX + 24.0f, panelY + panelH - 22.0f));
    window.draw(footer);
}
