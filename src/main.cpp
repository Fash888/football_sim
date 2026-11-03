#include <SFML/Graphics.hpp>
#include <iostream>
#include "Config.hpp"
#include "MatchEngine.hpp"
#include "TrackingSystem.hpp"
#include "PitchRenderer.hpp"
#include "RandomEngine.hpp"
#include "Tactics.hpp"
#include "RealMatchData.hpp"

int main() {
    sf::ContextSettings settings;
#if SFML_VERSION_MAJOR >= 3
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u(SimConfig::WINDOW_WIDTH, SimConfig::WINDOW_HEIGHT)),
        "FUT ANALYTICS - 2D Match Simulator & Tracking System",
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );
#else
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(
        sf::VideoMode(SimConfig::WINDOW_WIDTH, SimConfig::WINDOW_HEIGHT),
        "FUT ANALYTICS - 2D Match Simulator & Tracking System",
        sf::Style::Default,
        settings
    );
#endif

    window.setFramerateLimit(60);

    PitchRenderer renderer;
    renderer.updateView(window.getSize());

    unsigned int initialSeed = RandomEngine::generateRandomSeed();
    MatchEngine engine(initialSeed);
    TrackingSystem trackingSystem(10.0f);

    sf::Clock frameClock;
    float timeAccumulator = 0.0f;
    float simSpeed = 4.0f; // Fast, responsive, and engaging pacing (4x default)
    bool isPaused = false;

    // Real Match Optical Tracking Replay Dataset
    RealMatchDataset realMatchDataset;
    bool realMatchLoaded = realMatchDataset.loadMetricaSampleGame(
        "data/Sample_Game_1_Tracking_Home.csv",
        "data/Sample_Game_1_Tracking_Away.csv",
        "data/Sample_Game_1_Events.csv",
        2
    );
    bool inRealMatchMode = false;
    float realMatchClock = 0.0f;

    // Milestone 6 Interactive Analytics State
    int selectedPlayerId = 10; // Default: Home Striker
    bool showHeatmap = true;
    bool showPassingNetwork = false;
    bool showTacticalLines = true; // Tactical connection lines between players (always updates for every position)
    bool cameraFollowBall = false;
    bool showCalibrationPanel = true;
    bool showTeamDetails = false;

    std::cout << "========================================================================\n"
              << " FUT ANALYTICS - Advanced Match Simulation & Tracking System\n"
              << "========================================================================\n"
              << " - Initial Match Seed: " << initialSeed << "\n"
              << " - [M] Toggle Real Match Optical Tracking Replay (Metrica 25Hz Match 1: 3-2)\n"
              << " - [Left / Right] Scrub Real Match Timeline (-10s / +10s)\n"
              << " - [Z] Cycle Home Club (10 Teams)  |  [X] Cycle Away Club (10 Teams)\n"
              << " - [V] Toggle Matchup & Tactical Card (Strengths & Weaknesses)\n"
              << " - Click ANY player to select and view FC IQ role, ratings & heatmap\n"
              << " - [T] Cycle Home Tactic  |  [Y] Cycle Away Tactic\n"
              << " - [L] Tactical Connection Lines (ON/OFF)  |  [N] Passing Network\n"
              << " - [H] Heatmap  |  [C] Camera Follow  |  [K] Calibration Panel\n"
              << " - [R] / [S] Generate New Random Match (New Seed)\n"
              << " - Formations: [1-4] Home (4-3-3, 4-4-2, 4-2-3-1, 3-4-2-1), [7-0] Away\n"
              << " - Speeds: [P] Pause, [O] 1x, [I] 4x, [F] 8x, [Up/Down] Speed, [U] Instant to 90m\n"
              << " - [SPACE] Pause / Resume (or Resume from Half-Time)\n"
              << "========================================================================\n";

    std::vector<Player> realDisplayPlayers;
    Ball realDisplayBall;

    while (window.isOpen()) {
        float frameDt = frameClock.restart().asSeconds();
        if (frameDt > 0.1f) frameDt = 0.1f;

#if SFML_VERSION_MAJOR >= 3
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Left) {
                    sf::Vector2f mPos(static_cast<float>(mouseBtn->position.x), static_cast<float>(mouseBtn->position.y));
                    const auto& activePlayers = inRealMatchMode ? realDisplayPlayers : engine.getPlayers();
                    int clickedId = renderer.selectPlayerAtScreenPos(mPos, activePlayers);
                    if (clickedId != -1) {
                        selectedPlayerId = clickedId;
                        for (const auto& p : activePlayers) {
                            if (p.id == clickedId) {
                                std::cout << "[INFO] Selected Player #" << p.shirtNumber << " " << p.name
                                          << " | PAC:" << static_cast<int>(p.attributes.pace)
                                          << " | Dist: " << static_cast<int>(p.distanceCoveredM) << "m\n";
                                break;
                            }
                        }
                    }
                }
            } else if (const auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPress->code == sf::Keyboard::Key::Escape) {
                    window.close();
                } else if (keyPress->code == sf::Keyboard::Key::Space) {
                    if (engine.isHalfTime()) {
                        engine.startSecondHalf();
                        isPaused = false;
                        std::cout << "[INFO] Resuming match: Second Half Kickoff!\n";
                    } else {
                        isPaused = !isPaused;
                        std::cout << (isPaused ? "[INFO] Simulation Paused\n" : "[INFO] Simulation Resumed\n");
                    }
                } else if (keyPress->code == sf::Keyboard::Key::M) {
                    inRealMatchMode = !inRealMatchMode;
                    std::cout << "[MODE] " << (inRealMatchMode ? "Switched to REAL MATCH TRACKING REPLAY (Metrica 25Hz)" : "Switched to LIVE AI ENGINE") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::Left) {
                    if (inRealMatchMode) {
                        realMatchClock = std::max(0.0f, realMatchClock - 10.0f);
                        std::cout << "[REPLAY] Scrubbed -10s to " << realMatchClock / 60.0f << " min\n";
                    }
                } else if (keyPress->code == sf::Keyboard::Key::Right) {
                    if (inRealMatchMode) {
                        realMatchClock = std::min(realMatchDataset.getDurationSeconds(), realMatchClock + 10.0f);
                        std::cout << "[REPLAY] Scrubbed +10s to " << realMatchClock / 60.0f << " min\n";
                    }
                } else if (keyPress->code == sf::Keyboard::Key::Z) {
                    engine.cycleHomeTeam();
                } else if (keyPress->code == sf::Keyboard::Key::X) {
                    engine.cycleAwayTeam();
                } else if (keyPress->code == sf::Keyboard::Key::V) {
                    showTeamDetails = !showTeamDetails;
                    std::cout << "[INFO] Team Details Card: " << (showTeamDetails ? "ON" : "OFF") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::T) {
                    engine.cycleHomeTacticalStyle();
                } else if (keyPress->code == sf::Keyboard::Key::Y) {
                    engine.cycleAwayTacticalStyle();
                } else if (keyPress->code == sf::Keyboard::Key::H) {
                    showHeatmap = !showHeatmap;
                    std::cout << "[INFO] Heatmap: " << (showHeatmap ? "ON" : "OFF") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::L) {
                    showTacticalLines = !showTacticalLines;
                    std::cout << "[INFO] Tactical Connection Lines: " << (showTacticalLines ? "ON" : "OFF") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::N) {
                    showPassingNetwork = !showPassingNetwork;
                    std::cout << "[INFO] Passing Network: " << (showPassingNetwork ? "ON" : "OFF") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::C) {
                    cameraFollowBall = !cameraFollowBall;
                    if (!cameraFollowBall) renderer.updateView(window.getSize());
                    std::cout << "[INFO] Camera Follow Ball: " << (cameraFollowBall ? "ON" : "OFF") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::K) {
                    showCalibrationPanel = !showCalibrationPanel;
                    std::cout << "[INFO] Calibration Panel: " << (showCalibrationPanel ? "ON" : "OFF") << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::R || keyPress->code == sf::Keyboard::Key::S) {
                    unsigned int newSeed = RandomEngine::generateRandomSeed();
                    engine.reseed(newSeed);
                    engine.resetMatch();
                    trackingSystem.reset();
                    std::cout << "[INFO] Match restarted with new Stochastic Seed: " << newSeed << "!\n";
                } else if (keyPress->code == sf::Keyboard::Key::E) {
                    trackingSystem.exportPlayersCSV(engine.getPlayers());
                    trackingSystem.exportMatchCSV(engine.getHomeScore(), engine.getAwayScore(), engine.getMatchTime());
                } else if (keyPress->code == sf::Keyboard::Key::P) {
                    isPaused = true;
                    std::cout << "[INFO] Paused\n";
                } else if (keyPress->code == sf::Keyboard::Key::O) {
                    simSpeed = 1.0f;
                    isPaused = false;
                    std::cout << "[INFO] Speed: 1x\n";
                } else if (keyPress->code == sf::Keyboard::Key::I) {
                    simSpeed = 4.0f;
                    isPaused = false;
                    std::cout << "[INFO] Speed: 4x\n";
                } else if (keyPress->code == sf::Keyboard::Key::F) {
                    simSpeed = 8.0f;
                    isPaused = false;
                    std::cout << "[INFO] Speed: 8x\n";
                } else if (keyPress->code == sf::Keyboard::Key::U) {
                    // Instant mode: fast forward until Full Time
                    std::cout << "[INFO] Instant simulating match to Full Time...\n";
                    while (!engine.isFullTime() && engine.getMatchTime() < 5800.0f) {
                        if (engine.isHalfTime()) engine.startSecondHalf();
                        engine.update(SimConfig::SIMULATION_TIMESTEP);
                    }
                    std::cout << "[INFO] Instant simulation complete! Score: "
                              << engine.getHomeScore() << " - " << engine.getAwayScore() << "\n";
                } else if (keyPress->code == sf::Keyboard::Key::Up) {
                    simSpeed = std::min(32.0f, simSpeed * 2.0f);
                    std::cout << "[INFO] Speed: " << simSpeed << "x\n";
                } else if (keyPress->code == sf::Keyboard::Key::Down) {
                    simSpeed = std::max(0.5f, simSpeed * 0.5f);
                    std::cout << "[INFO] Speed: " << simSpeed << "x\n";
                } else if (keyPress->code == sf::Keyboard::Key::Num1) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_4_3_3);
                } else if (keyPress->code == sf::Keyboard::Key::Num2) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_4_4_2);
                } else if (keyPress->code == sf::Keyboard::Key::Num3) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_4_2_3_1);
                } else if (keyPress->code == sf::Keyboard::Key::Num4) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_3_4_2_1);
                } else if (keyPress->code == sf::Keyboard::Key::Num7) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_4_3_3);
                } else if (keyPress->code == sf::Keyboard::Key::Num8) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_4_4_2);
                } else if (keyPress->code == sf::Keyboard::Key::Num9) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_4_2_3_1);
                } else if (keyPress->code == sf::Keyboard::Key::Num0) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_3_4_2_1);
                }
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::FloatRect visibleArea(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)));
                window.setView(sf::View(visibleArea));
                renderer.updateView(window.getSize());
            }
        }
#else
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mPos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                    const auto& activePlayers = inRealMatchMode ? realDisplayPlayers : engine.getPlayers();
                    int clickedId = renderer.selectPlayerAtScreenPos(mPos, activePlayers);
                    if (clickedId != -1) {
                        selectedPlayerId = clickedId;
                        for (const auto& p : activePlayers) {
                            if (p.id == clickedId) {
                                std::cout << "[INFO] Selected Player #" << p.shirtNumber << " " << p.name
                                          << " | PAC:" << static_cast<int>(p.attributes.pace)
                                          << " | Dist: " << static_cast<int>(p.distanceCoveredM) << "m\n";
                                break;
                            }
                        }
                    }
                }
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                } else if (event.key.code == sf::Keyboard::Space) {
                    if (engine.isHalfTime()) {
                        engine.startSecondHalf();
                        isPaused = false;
                        std::cout << "[INFO] Resuming match: Second Half Kickoff!\n";
                    } else {
                        isPaused = !isPaused;
                        std::cout << (isPaused ? "[INFO] Simulation Paused\n" : "[INFO] Simulation Resumed\n");
                    }
                } else if (event.key.code == sf::Keyboard::M) {
                    inRealMatchMode = !inRealMatchMode;
                    std::cout << "[MODE] " << (inRealMatchMode ? "Switched to REAL MATCH TRACKING REPLAY (Metrica 25Hz)" : "Switched to LIVE AI ENGINE") << "\n";
                } else if (event.key.code == sf::Keyboard::Left) {
                    if (inRealMatchMode) {
                        realMatchClock = std::max(0.0f, realMatchClock - 10.0f);
                        std::cout << "[REPLAY] Scrubbed -10s to " << realMatchClock / 60.0f << " min\n";
                    }
                } else if (event.key.code == sf::Keyboard::Right) {
                    if (inRealMatchMode) {
                        realMatchClock = std::min(realMatchDataset.getDurationSeconds(), realMatchClock + 10.0f);
                        std::cout << "[REPLAY] Scrubbed +10s to " << realMatchClock / 60.0f << " min\n";
                    }
                } else if (event.key.code == sf::Keyboard::Z) {
                    engine.cycleHomeTeam();
                } else if (event.key.code == sf::Keyboard::X) {
                    engine.cycleAwayTeam();
                } else if (event.key.code == sf::Keyboard::V) {
                    showTeamDetails = !showTeamDetails;
                    std::cout << "[INFO] Team Details Card: " << (showTeamDetails ? "ON" : "OFF") << "\n";
                } else if (event.key.code == sf::Keyboard::T) {
                    engine.cycleHomeTacticalStyle();
                } else if (event.key.code == sf::Keyboard::Y) {
                    engine.cycleAwayTacticalStyle();
                } else if (event.key.code == sf::Keyboard::H) {
                    showHeatmap = !showHeatmap;
                } else if (event.key.code == sf::Keyboard::L) {
                    showTacticalLines = !showTacticalLines;
                    std::cout << "[INFO] Tactical Connection Lines: " << (showTacticalLines ? "ON" : "OFF") << "\n";
                } else if (event.key.code == sf::Keyboard::N) {
                    showPassingNetwork = !showPassingNetwork;
                } else if (event.key.code == sf::Keyboard::C) {
                    cameraFollowBall = !cameraFollowBall;
                    if (!cameraFollowBall) renderer.updateView(window.getSize());
                } else if (event.key.code == sf::Keyboard::K) {
                    showCalibrationPanel = !showCalibrationPanel;
                    std::cout << "[INFO] Calibration Panel: " << (showCalibrationPanel ? "ON" : "OFF") << "\n";
                } else if (event.key.code == sf::Keyboard::R || event.key.code == sf::Keyboard::S) {
                    unsigned int newSeed = RandomEngine::generateRandomSeed();
                    engine.reseed(newSeed);
                    engine.resetMatch();
                    trackingSystem.reset();
                    std::cout << "[INFO] Match restarted with new Stochastic Seed: " << newSeed << "!\n";
                } else if (event.key.code == sf::Keyboard::E) {
                    trackingSystem.exportPlayersCSV(engine.getPlayers());
                    trackingSystem.exportMatchCSV(engine.getHomeScore(), engine.getAwayScore(), engine.getMatchTime());
                } else if (event.key.code == sf::Keyboard::P) {
                    isPaused = true;
                } else if (event.key.code == sf::Keyboard::O) {
                    simSpeed = 1.0f;
                    isPaused = false;
                } else if (event.key.code == sf::Keyboard::I) {
                    simSpeed = 4.0f;
                    isPaused = false;
                } else if (event.key.code == sf::Keyboard::F) {
                    simSpeed = 8.0f;
                    isPaused = false;
                } else if (event.key.code == sf::Keyboard::U) {
                    while (!engine.isFullTime() && engine.getMatchTime() < 5800.0f) {
                        if (engine.isHalfTime()) engine.startSecondHalf();
                        engine.update(SimConfig::SIMULATION_TIMESTEP);
                    }
                } else if (event.key.code == sf::Keyboard::Up) {
                    simSpeed = std::min(32.0f, simSpeed * 2.0f);
                    std::cout << "[INFO] Speed: " << simSpeed << "x\n";
                } else if (event.key.code == sf::Keyboard::Down) {
                    simSpeed = std::max(0.5f, simSpeed * 0.5f);
                    std::cout << "[INFO] Speed: " << simSpeed << "x\n";
                } else if (event.key.code == sf::Keyboard::Num1) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_4_3_3);
                } else if (event.key.code == sf::Keyboard::Num2) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_4_4_2);
                } else if (event.key.code == sf::Keyboard::Num3) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_4_2_3_1);
                } else if (event.key.code == sf::Keyboard::Num4) {
                    engine.setTeamFormation(TeamSide::Home, FormationType::F_3_4_2_1);
                } else if (event.key.code == sf::Keyboard::Num7) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_4_3_3);
                } else if (event.key.code == sf::Keyboard::Num8) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_4_4_2);
                } else if (event.key.code == sf::Keyboard::Num9) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_4_2_3_1);
                } else if (event.key.code == sf::Keyboard::Num0) {
                    engine.setTeamFormation(TeamSide::Away, FormationType::F_3_4_2_1);
                }
            } else if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                renderer.updateView(window.getSize());
            }
        }
#endif

        if (!isPaused) {
            if (inRealMatchMode && realMatchLoaded) {
                realMatchClock += frameDt * simSpeed;
                if (realMatchClock > realMatchDataset.getDurationSeconds()) {
                    realMatchClock = 0.0f;
                }
            } else {
                timeAccumulator += frameDt * simSpeed;
                while (timeAccumulator >= SimConfig::SIMULATION_TIMESTEP) {
                    engine.update(SimConfig::SIMULATION_TIMESTEP);
                    trackingSystem.update(engine.getMatchTime(), engine.getPlayers(), engine.getBall());
                    timeAccumulator -= SimConfig::SIMULATION_TIMESTEP;
                }
            }
        }

        window.clear(sf::Color(15, 18, 22));

        if (inRealMatchMode && realMatchLoaded) {
            realDisplayPlayers.clear();
            const RealMatchFrame* rFrame = realMatchDataset.getFrameAtTime(realMatchClock);
            if (rFrame) {
                realDisplayBall.position = rFrame->ballPos;
                realDisplayBall.height = rFrame->ballHeight;
                for (const auto& rTrack : rFrame->players) {
                    Player p;
                    p.id = (rTrack.team == TeamSide::Home ? 0 : 50) + rTrack.shirtNumber;
                    p.shirtNumber = rTrack.shirtNumber;
                    p.team = rTrack.team;
                    p.name = (rTrack.team == TeamSide::Home ? "Home #" : "Away #") + std::to_string(rTrack.shirtNumber);
                    p.role = rTrack.isGoalkeeper ? TacticalRole::GK : TacticalRole::CM_LEFT;
                    p.position = rTrack.pitchPos;
                    p.velocity = rTrack.velocity;
                    p.distanceCoveredM = rTrack.distanceCoveredM;
                    p.attributes.pace = std::clamp(static_cast<int>(rTrack.speedMps * 10.0f + 50.0f), 50, 99);
                    p.attributes.maxSpeed = 9.0f;

                    // Dynamic body orientation (facing direction)
                    if (rTrack.velocity.lengthSq() > 0.20f) {
                        p.headingDeg = std::atan2(rTrack.velocity.y, rTrack.velocity.x) * (180.0f / 3.14159265f);
                    } else {
                        Vec2 toBall = rFrame->ballPos - rTrack.pitchPos;
                        p.headingDeg = std::atan2(toBall.y, toBall.x) * (180.0f / 3.14159265f);
                    }

                    // Ball proximity possession flag
                    float distToBall = (rTrack.pitchPos - rFrame->ballPos).length();
                    if (distToBall < 1.15f) {
                        p.hasPossession = true;
                    }

                    realDisplayPlayers.push_back(p);
                }
            }

            renderer.render(
                window,
                realDisplayPlayers,
                realDisplayBall,
                realMatchDataset.getHomeScore(realMatchClock),
                realMatchDataset.getAwayScore(realMatchClock),
                realMatchClock,
                engine.getAnalytics(),
                engine.getEventRecorder(),
                engine.getSpatialAnalytics(),
                selectedPlayerId,
                showHeatmap,
                showPassingNetwork,
                cameraFollowBall,
                false,
                MatchPhase::InPlay,
                180.0f,
                240.0f,
                engine.getHomeTacticalStyle(),
                engine.getAwayTacticalStyle(),
                showTacticalLines,
                simSpeed,
                &engine.getHomeTeamProfile(),
                &engine.getAwayTeamProfile(),
                false,
                engine.getHomeFormation(),
                engine.getAwayFormation(),
                true,
                realMatchDataset.getActiveEventText(realMatchClock)
            );
        } else {
            renderer.render(
                window,
                engine.getPlayers(),
                engine.getBall(),
                engine.getHomeScore(),
                engine.getAwayScore(),
                engine.getMatchTime(),
                engine.getAnalytics(),
                engine.getEventRecorder(),
                engine.getSpatialAnalytics(),
                selectedPlayerId,
                showHeatmap,
                showPassingNetwork,
                cameraFollowBall,
                showCalibrationPanel,
                engine.getPhase(),
                engine.getFirstHalfAddedTime(),
                engine.getSecondHalfAddedTime(),
                engine.getHomeTacticalStyle(),
                engine.getAwayTacticalStyle(),
                showTacticalLines,
                simSpeed,
                &engine.getHomeTeamProfile(),
                &engine.getAwayTeamProfile(),
                showTeamDetails,
                engine.getHomeFormation(),
                engine.getAwayFormation(),
                false,
                ""
            );
        }
        window.display();
    }

    trackingSystem.flushToCSV();
    trackingSystem.exportPlayersCSV(engine.getPlayers());
    trackingSystem.exportMatchCSV(engine.getHomeScore(), engine.getAwayScore(), engine.getMatchTime());

    return 0;
}
