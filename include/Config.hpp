#pragma once

namespace SimConfig {
    // FIFA Standard Pitch Dimensions in metres
    constexpr float PITCH_LENGTH_M = 105.0f;
    constexpr float PITCH_WIDTH_M = 68.0f;
    constexpr float PITCH_MARGIN_M = 5.0f;

    // Pitch Markings (in metres)
    constexpr float CENTER_CIRCLE_RADIUS_M = 9.15f;
    constexpr float PENALTY_AREA_LENGTH_M = 16.5f;
    constexpr float PENALTY_AREA_WIDTH_M = 40.32f;
    constexpr float GOAL_AREA_LENGTH_M = 5.5f;
    constexpr float GOAL_AREA_WIDTH_M = 18.32f;
    constexpr float PENALTY_SPOT_DIST_M = 11.0f;
    constexpr float PENALTY_ARC_RADIUS_M = 9.15f;
    constexpr float CORNER_ARC_RADIUS_M = 1.0f;
    constexpr float GOAL_WIDTH_M = 7.32f;
    constexpr float GOAL_DEPTH_M = 2.44f;
    constexpr float GOAL_HEIGHT_M = 2.44f;

    // Dimensions
    constexpr float PLAYER_RADIUS_M = 0.85f;
    constexpr float BALL_RADIUS_M = 0.22f;

    // Simulation Timing
    constexpr float SIMULATION_TIMESTEP = 1.0f / 60.0f; // 60 Hz fixed physics step

    // Ball Dynamics & Aerodynamics
    constexpr float BALL_FRICTION_ROLL = 0.994f;        // Realistic professional pitch roll damping
    constexpr float BALL_AIR_DRAG = 0.996f;
    constexpr float GRAVITY = 9.81f;
    constexpr float BALL_BOUNCE_RESTITUTION = 0.55f;

    // Possession & Control
    constexpr float TOUCH_RADIUS_M = 1.4f;             // Radius within which player can control ball
    constexpr float DRIBBLE_OFFSET_M = 0.85f;          // Ball distance ahead while dribbling
    constexpr float TACKLE_DISTANCE_M = 1.1f;          // Required proximity for defender to dispossess

    // Realistic Player Kinematics (Metres / Second) - Calibrated with Metrica & FIFA Data
    constexpr float PLAYER_MAX_SPRINT_MPS = 8.8f;      // ~32 km/h top sprint
    constexpr float PLAYER_RUN_SPEED_MPS = 5.6f;       // ~20 km/h progressive run
    constexpr float PLAYER_JOG_SPEED_MPS = 3.8f;       // ~14 km/h tactical jog
    constexpr float PLAYER_WALK_SPEED_MPS = 1.8f;      // ~6.5 km/h recovery walk
    constexpr float PLAYER_DRIBBLE_SPEED_MPS = 5.2f;   // ~19 km/h controlled dribble
    constexpr float PLAYER_ACCELERATION_MPS2 = 4.5f;   // Realistic acceleration (3.5 - 5.0 m/s2)
    constexpr float PLAYER_DECELERATION_MPS2 = 6.8f;   // Natural stopping deceleration (5.5 - 7.5 m/s2)
    constexpr float PLAYER_TURN_RATE_RAD_PER_S = 7.5f; // Capped turning rate: cut speed on sharp turns

    // Ball Dynamics & Kicking Speeds - Calibrated from StatsBomb CL Final Real Event Data
    constexpr float SHORT_PASS_SPEED_MPS = 13.0f;      // Crisp 10-22m ground pass (mean ~12.8 m/s)
    constexpr float LONG_PASS_SPEED_MPS = 19.5f;       // Driven 25-45m switch (90th percentile ~19.5 m/s)
    constexpr float CHIP_PASS_SPEED_MPS = 16.0f;       // Aerial lofted ball over defensive block
    constexpr float SHOT_SPEED_MPS = 26.5f;            // ~95 km/h driven strike

    // Spearman Pitch Control Parameters (William Spearman 2018)
    constexpr float SPEARMAN_REACTION_TIME_S = 0.55f;  // Perception/reaction latency tau
    constexpr float SPEARMAN_SIGMA_BETA = 4.2f;        // Sharpness of pitch control boundary

    // Goalkeeper Real-World Physics & Shot Stopping
    constexpr float GK_ARM_SPAN_M = 1.35f;            // Standing arm reach from center
    constexpr float GK_MAX_DIVE_REACH_M = 2.75f;      // Max lateral dive extension
    constexpr float GK_DIVE_VELOCITY_MPS = 5.2f;      // Lateral dive explosion
    constexpr float GK_BASE_REACTION_DELAY_S = 0.22f; // Human visual reaction time to driven shot

    // Match Time Progression & Halves with Random Added Time (1 to 7 Minutes)
    constexpr float HALF_TIME_DURATION_S = 2700.0f;    // 45 minutes = 2700 simulated seconds
    constexpr float FULL_TIME_DURATION_S = 5400.0f;    // 90 minutes = 5400 simulated seconds
    constexpr float MIN_ADDED_TIME_S = 60.0f;          // 1 minute minimum stoppage time
    constexpr float MAX_ADDED_TIME_S = 420.0f;         // 7 minutes maximum stoppage time
    constexpr float DEFAULT_SIM_SPEED = 2.0f;          // Fast, enjoyable match pacing

    // Calibration Target Benchmarks (Over 100+ simulated matches)
    constexpr float CALIBRATION_TARGET_GOALS_MIN = 2.70f;
    constexpr float CALIBRATION_TARGET_GOALS_MAX = 3.00f;
    constexpr float CALIBRATION_TARGET_REGAIN_TIME_DIFF_S = 4.0f; // Winner regains ball ~4s faster
    constexpr float CALIBRATION_TARGET_TRANSITION_PRESS_PCT = 40.0f; // 40% transitions from press/counter-press
    constexpr float CALIBRATION_TARGET_GK_GOALKICK_PCT = 50.0f; // 50% goal kicks taken by GK (rest by CB)
    constexpr float CALIBRATION_TARGET_SET_PIECE_GOAL_PCT = 26.0f; // 25-27% of goals from set pieces
    constexpr float CALIBRATION_TARGET_AVG_PASS_SEQ_S = 9.5f; // 9-10s average sequence
    constexpr float CALIBRATION_TARGET_TOTAL_SHOTS = 25.0f; // ~25 shots per match
    constexpr float CALIBRATION_TARGET_AVG_XG = 0.10f; // Avg shot xG ~0.10

    // Decision & Shooting Thresholds
    constexpr float SHOOTING_DISTANCE_THRESHOLD_M = 24.5f; // StatsBomb empirical: shots taken <= 25m
    constexpr float DRIBBLE_MAX_DURATION_S = 1.4f;        // Average time before pass execution

    // Window defaults
    constexpr unsigned int WINDOW_WIDTH = 1280;
    constexpr unsigned int WINDOW_HEIGHT = 800;
}
