#pragma once
#include <random>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include "Vec2.hpp"

// =============================================================================
//  RandomEngine — Stochastic Subsystem for FUT ANALYTICS
//
//  Features:
//    - Seeded, 100% reproducible PRNG (preserves determinism for tests)
//    - Entropy-based seed generator for live dynamic matches
//    - Football-specific distributions:
//        * Pass accuracy error cones based on skill, pressure & fatigue
//        * Shot targeting & dispersion (corners, woodworks, crossbar)
//        * Multi-outcome tackling duels (clean win, loose 50/50 deflection, retained)
//        * First-touch imperfection & scatter
//        * Softmax/Weighted pass candidate selection for dynamic passing variety
//        * Player form drift (Brownian random walk over match minutes)
// =============================================================================
class RandomEngine {
public:
    explicit RandomEngine(unsigned int seed = 1337) {
        reseed(seed);
    }

    // Generate a high-entropy seed using random_device + clock
    static unsigned int generateRandomSeed() {
        std::random_device rd;
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return static_cast<unsigned int>(rd() ^ static_cast<unsigned int>(now));
    }

    void reseed(unsigned int seed) {
        m_seed = seed;
        m_rng.seed(seed);
    }

    unsigned int currentSeed() const { return m_seed; }

    // ── Fundamental Distributions ─────────────────────────────────────────────

    // Uniform float in [lo, hi]
    float uniform(float lo, float hi) {
        if (lo >= hi) return lo;
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(m_rng);
    }

    // Uniform integer in [lo, hi] inclusive
    int uniformInt(int lo, int hi) {
        if (lo >= hi) return lo;
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(m_rng);
    }

    // Bernoulli trial: returns true with probability p (0.0 to 1.0)
    bool chance(float probability) {
        if (probability <= 0.0f) return false;
        if (probability >= 1.0f) return true;
        return uniform(0.0f, 1.0f) < probability;
    }

    // Gaussian normal distribution clamped to 3 standard deviations
    float gaussian(float mean, float stddev) {
        std::normal_distribution<float> dist(mean, stddev);
        float val = dist(m_rng);
        return std::clamp(val, mean - 3.0f * stddev, mean + 3.0f * stddev);
    }

    // ── Football Stochastic Behaviors ─────────────────────────────────────────

    // Rotate a unit vector by an exact signed angle in degrees
    Vec2 rotateDirectionDeg(Vec2 baseDir, float angleDeg) {
        if (std::abs(angleDeg) <= 0.0001f) return baseDir;
        float rad = angleDeg * (3.141592653589793f / 180.0f);
        float c = std::cos(rad);
        float s = std::sin(rad);
        return Vec2(baseDir.x * c - baseDir.y * s,
                    baseDir.x * s + baseDir.y * c).normalized();
    }

    // Deviate a unit vector by a random angle in [-maxDeg, +maxDeg]
    Vec2 deviateDirection(Vec2 baseDir, float maxDeg) {
        float absMax = std::abs(maxDeg);
        if (absMax <= 0.001f) return baseDir;
        float angleDeg = uniform(-absMax, absMax);
        return rotateDirectionDeg(baseDir, angleDeg);
    }

    // Pass angle error: calibrated Gaussian distribution (crisp accuracy for rated players)
    float passDeviationDeg(float passSkill, float pressure = 0.0f, float fatigue = 0.0f) {
        float skillFactor = std::clamp(1.0f - passSkill, 0.0f, 1.0f);
        float stddev = 0.55f + (skillFactor * 1.6f) + (pressure * 1.1f) + (fatigue * 0.7f);
        return gaussian(0.0f, stddev);
    }

    // Pass speed variance: slight natural variance in kicking power (+/- 5%)
    float passSpeedVariance(float baseSpeed) {
        return baseSpeed * uniform(0.95f, 1.05f);
    }

    // Shot angle error: Gaussian dispersion toward target
    float shotDeviationDeg(float shotPower, float pressure = 0.0f) {
        float powerNorm = std::clamp(shotPower / 30.0f, 0.0f, 1.0f);
        float stddev = 1.0f + (1.0f - powerNorm) * 2.2f + (pressure * 1.4f);
        return gaussian(0.0f, stddev);
    }

    // First touch deflection/imperfection:
    // Elite players cushion with high precision; hard passes or under-pressure touches bounce slightly
    Vec2 touchScatter(float controlSkill, float incomingBallSpeed) {
        float speedFactor = std::clamp(incomingBallSpeed / 25.0f, 0.3f, 1.4f);
        float maxScatter = (1.0f - std::clamp(controlSkill, 0.5f, 0.99f)) * 0.40f * speedFactor;
        return Vec2(gaussian(0.0f, maxScatter * 0.5f), gaussian(0.0f, maxScatter * 0.5f));
    }

    // Tackle outcome duel:
    // 0 = Defender wins cleanly (possession won)
    // 1 = Loose ball deflection (ball knocked into open space for 50/50 duel!)
    // 2 = Dribbler retains ball (turn/body shield)
    // 3 = Foul committed by tackler (Law 12)
    int resolveTackleDuel(float tackleSkill, float dribblerSkill, float pressure = 1.0f) {
        float tScore = tackleSkill * (0.6f + pressure * 0.4f);
        float dScore = dribblerSkill * 0.75f;
        float total = tScore + dScore;
        if (total <= 0.001f) return 0;

        float winProb = std::clamp(tScore / total, 0.35f, 0.85f);
        float roll = uniform(0.0f, 1.0f);

        // Calibrated foul rate: ~8-12% of challenges result in fouls
        float foulChance = 0.095f * (1.20f - std::clamp(tackleSkill, 0.4f, 0.98f) * 0.35f);

        if (roll < foulChance) {
            return 3; // Foul committed
        } else if (roll < foulChance + winProb * 0.65f) {
            return 0; // Clean win
        } else if (roll < foulChance + winProb + 0.16f) {
            return 1; // Loose ball deflection
        } else {
            return 2; // Dribbler retains
        }
    }

    // Softmax / Weighted selection among candidates
    // Prevents always passing to the identical player; creates realistic distribution patterns
    int selectWeightedOption(const std::vector<float>& weights) {
        if (weights.empty()) return -1;
        if (weights.size() == 1) return 0;

        float sum = 0.0f;
        for (float w : weights) {
            sum += std::max(0.01f, w);
        }

        float r = uniform(0.0f, sum);
        float accum = 0.0f;
        for (size_t i = 0; i < weights.size(); ++i) {
            accum += std::max(0.01f, weights[i]);
            if (r <= accum) return static_cast<int>(i);
        }
        return static_cast<int>(weights.size() - 1);
    }

    // Form drift: Brownian random walk with mean reversion
    float driftForm(float currentForm) {
        float step = gaussian(0.0f, 0.015f);
        float newForm = currentForm + step;
        // Revert gently toward 1.0
        newForm += (1.0f - newForm) * 0.005f;
        return std::clamp(newForm, 0.78f, 1.22f);
    }

    // Decision timing jitter: variation in how long player surveys field before releasing ball
    float randomizedHoldTime(float roleAvgHoldTime = 0.85f) {
        return std::clamp(gaussian(roleAvgHoldTime, 0.25f), 0.25f, 1.6f);
    }

    std::mt19937& rng() { return m_rng; }

private:
    std::mt19937 m_rng;
    unsigned int m_seed{1337};
};
