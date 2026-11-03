#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>
#include "Config.hpp"
#include "Entities.hpp"
#include "MatchAnalytics.hpp"
#include "EventRecorder.hpp"
#include "SpatialAnalytics.hpp"
#include "Teams.hpp"

class PitchRenderer {
public:
    PitchRenderer();

    void updateView(const sf::Vector2u& windowSize);

    sf::Vector2f pitchToScreen(const Vec2& pitchPos) const;
    Vec2 screenToPitch(const sf::Vector2f& screenPos) const;
    float metresToPixels(float metres) const;

    void render(sf::RenderWindow& window,
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
                bool showCalibrationPanel = true,
                MatchPhase phase = MatchPhase::InPlay,
                float firstHalfAddedTime = 180.0f,
                float secondHalfAddedTime = 240.0f,
                TeamTacticalStyle homeStyle = TeamTacticalStyle::TikiTaka,
                TeamTacticalStyle awayStyle = TeamTacticalStyle::CounterAttack,
                bool showTacticalLines = true,
                float simSpeed = 4.0f,
                const TeamProfile* homeTeam = nullptr,
                const TeamProfile* awayTeam = nullptr,
                bool showTeamDetails = false,
                FormationType homeFormation = FormationType::F_4_3_3,
                FormationType awayFormation = FormationType::F_4_4_2,
                bool inRealMatchMode = false,
                const std::string& realMatchEventBanner = "");

    // Click selection query: returns player ID or -1
    int selectPlayerAtScreenPos(const sf::Vector2f& mousePos, const std::vector<Player>& players) const;

private:
    void drawGrassAndStripes(sf::RenderWindow& window);
    void drawPitchMarkings(sf::RenderWindow& window);
    void drawGoals(sf::RenderWindow& window);
    void drawMiniPitchHeatmap(sf::RenderWindow& window, const SpatialAnalytics& spatialAnalytics, int selectedPlayerId, float px, float py, float pw, float ph);
    void drawPassingNetwork(sf::RenderWindow& window, const SpatialAnalytics& spatialAnalytics, const std::vector<Player>& players);
    void drawTacticalLines(sf::RenderWindow& window, const std::vector<Player>& players, const Ball& ball);
    void drawTeamHulls(sf::RenderWindow& window, const std::vector<Player>& players, const TeamProfile* homeTeam, const TeamProfile* awayTeam);
    void drawOffsideLine(sf::RenderWindow& window, const std::vector<Player>& players, TeamSide possessionTeam, const Ball& ball);
    void drawCornerFlags(sf::RenderWindow& window);
    void drawTechnicalAreas(sf::RenderWindow& window, const TeamProfile* homeTeam, const TeamProfile* awayTeam);
    void drawBallTrail(sf::RenderWindow& window);
    void drawScreenLine(sf::RenderWindow& window, const sf::Vector2f& p1, const sf::Vector2f& p2, float thickness, const sf::Color& color);
    void drawPlayers(sf::RenderWindow& window, const std::vector<Player>& players, int selectedPlayerId, const TeamProfile* homeTeam, const TeamProfile* awayTeam);
    void drawBall(sf::RenderWindow& window, const Ball& ball);
    void drawHUD(sf::RenderWindow& window, int homeScore, int awayScore, float matchClockSeconds, const MatchAnalytics& analytics, MatchPhase phase, float firstHalfAddedTime, float secondHalfAddedTime, TeamTacticalStyle homeStyle, TeamTacticalStyle awayStyle, float simSpeed, bool showTacticalLines, const TeamProfile* homeTeam, const TeamProfile* awayTeam, FormationType homeFormation, FormationType awayFormation, bool inRealMatchMode = false, const std::string& realMatchEventBanner = "");
    void drawMomentumAndStatsPanel(sf::RenderWindow& window, const MatchAnalytics& analytics, const EventRecorder& eventRecorder, int selectedPlayerId, bool showHeatmap, bool showPassingNetwork, bool cameraFollowBall, const std::vector<Player>& players, const SpatialAnalytics& spatialAnalytics);
    void drawCalibrationPanel(sf::RenderWindow& window, const MatchAnalytics& analytics, int homeScore, int awayScore, float matchTime);
    void drawTeamDetailsPanel(sf::RenderWindow& window, const TeamProfile& homeTeam, const TeamProfile& awayTeam);

    sf::Font m_font;
    bool m_fontLoaded{false};

    float m_scale{1.0f};
    float m_offsetX{0.0f};
    float m_offsetY{0.0f};

    // Camera follow offset
    sf::Vector2f m_cameraCenter{SimConfig::PITCH_LENGTH_M * 0.5f, SimConfig::PITCH_WIDTH_M * 0.5f};

    // Tracking comet tail buffer
    std::deque<sf::Vector2f> m_ballTrail;

    sf::Color m_grassDark{42, 102, 45};
    sf::Color m_grassLight{48, 115, 51};
    sf::Color m_lineColor{240, 240, 240, 220};
    sf::Color m_homeColor{218, 41, 28};
    sf::Color m_homeGKColor{243, 156, 18};
    sf::Color m_awayColor{41, 128, 185};
    sf::Color m_awayGKColor{39, 174, 96};
};
