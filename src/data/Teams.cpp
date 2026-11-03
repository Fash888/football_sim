#include "Teams.hpp"
#include <algorithm>

namespace {

RealTeamPlayer makeGK(const std::string& name, int num, FCIQRole role, int ovr,
                      float reflexes, float diving, float handling, float positioning, float passSkill = 0.85f) {
    RealTeamPlayer p;
    p.name = name;
    p.shirtNumber = num;
    p.role = TacticalRole::GK;
    p.fcIqRole = role;
    p.overallRating = ovr;
    p.attributes = PlayerAttributes(7.4f, 7.0f, passSkill, 23.0f, 0.40f,
                                    ovr, 68, 30, static_cast<int>(passSkill * 100), 60, 45, 80,
                                    0.25f, 0.70f, 0.35f, 0.88f, 0.75f,
                                    reflexes, diving, handling, positioning);
    return p;
}

RealTeamPlayer makeOutfield(const std::string& name, int num, TacticalRole role, FCIQRole fcIq,
                            int ovr, int pac, int sho, int pas, int dri, int def, int phy,
                            float fin, float vis, float crs, float comp, float defAw) {
    RealTeamPlayer p;
    p.name = name;
    p.shirtNumber = num;
    p.role = role;
    p.fcIqRole = fcIq;
    p.overallRating = ovr;
    float maxSpd = 6.4f + (pac / 100.0f) * 3.3f;
    float acc = 3.6f + (pac / 100.0f) * 1.8f;
    float passSk = 0.65f + (pas / 100.0f) * 0.32f;
    float shotPwr = 18.0f + (sho / 100.0f) * 14.0f;
    float tklSk = 0.50f + (def / 100.0f) * 0.45f;
    p.attributes = PlayerAttributes(maxSpd, acc, passSk, shotPwr, tklSk,
                                    ovr, pac, sho, pas, dri, def, phy,
                                    fin, vis, crs, comp, defAw);
    return p;
}

std::vector<TeamProfile> buildDatabase() {
    std::vector<TeamProfile> teams;
    teams.reserve(10);

    // =========================================================================
    // 1. MANCHESTER CITY (Pep Guardiola) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 0;
        t.name = "Manchester City";
        t.shortName = "MCI";
        t.league = "Premier League";
        t.manager = "Pep Guardiola";
        t.styleDescription = "Positional Play & Total Control";
        t.primaryColor = sf::Color(108, 171, 221); // Sky Blue
        t.secondaryColor = sf::Color(255, 255, 255);
        t.gkColor = sf::Color(241, 196, 15);
        t.defaultFormation = FormationType::F_4_3_3;
        t.tacticalStyle = TeamTacticalStyle::TikiTaka;
        t.defensiveMentality = DefensiveMentality::HighPress;

        t.strengths = {
            "Elite ball retention & press resistance",
            "Inside-channel vision & playmaking (De Bruyne)",
            "World-class central poacher (Haaland)"
        };
        t.weaknesses = {
            "Vulnerable to direct balls behind high backline",
            "Exposed wide channels when fullbacks invert"
        };

        t.passSkillMultiplier = 1.08f;
        t.pressIntensityMultiplier = 1.10f;
        t.counterSpeedMultiplier = 0.95f;
        t.defensiveTackleMultiplier = 0.98f;
        t.setPieceBonus = 1.05f;
        t.offsideLineOffsetM = 1.8f;
        t.widthMultiplier = 1.05f;
        t.staminaDrainMultiplier = 0.95f;

        t.squad = {
            makeGK("Ederson", 31, FCIQRole::SweeperKeeper, 88, 0.86f, 0.86f, 0.82f, 0.86f, 0.91f),
            makeOutfield("Kyle Walker", 2, TacticalRole::RB, FCIQRole::AttackingWingback, 84, 90, 63, 77, 78, 80, 81, 0.55f, 0.74f, 0.82f, 0.80f, 0.82f),
            makeOutfield("Ruben Dias", 3, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 88, 67, 39, 70, 69, 89, 87, 0.40f, 0.78f, 0.45f, 0.89f, 0.91f),
            makeOutfield("John Stones", 5, TacticalRole::CB_LEFT, FCIQRole::BallPlayingDefender, 85, 72, 51, 78, 77, 86, 80, 0.52f, 0.84f, 0.55f, 0.92f, 0.88f),
            makeOutfield("Josko Gvardiol", 24, TacticalRole::LB, FCIQRole::InvertedFullback, 83, 81, 64, 75, 79, 83, 83, 0.62f, 0.80f, 0.80f, 0.86f, 0.86f),
            makeOutfield("Rodri", 16, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 91, 66, 80, 86, 84, 87, 85, 0.78f, 0.92f, 0.76f, 0.95f, 0.89f),
            makeOutfield("Kevin De Bruyne", 17, TacticalRole::CM_RIGHT, FCIQRole::PlaymakerCAM, 90, 67, 87, 94, 87, 65, 74, 0.86f, 0.97f, 0.95f, 0.92f, 0.68f),
            makeOutfield("Bernardo Silva", 20, TacticalRole::CM_LEFT, FCIQRole::BoxToBox, 88, 69, 78, 86, 92, 70, 76, 0.79f, 0.90f, 0.84f, 0.94f, 0.78f),
            makeOutfield("Phil Foden", 47, TacticalRole::RW, FCIQRole::InsideForward, 88, 86, 86, 87, 90, 57, 64, 0.87f, 0.89f, 0.82f, 0.88f, 0.62f),
            makeOutfield("Erling Haaland", 9, TacticalRole::ST, FCIQRole::PoacherST, 91, 88, 92, 70, 81, 45, 88, 0.96f, 0.76f, 0.50f, 0.89f, 0.50f),
            makeOutfield("Jack Grealish", 10, TacticalRole::LW, FCIQRole::ClassicWinger, 84, 76, 76, 84, 88, 46, 69, 0.75f, 0.86f, 0.83f, 0.90f, 0.55f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 2. REAL MADRID (Carlo Ancelotti) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 1;
        t.name = "Real Madrid";
        t.shortName = "RMA";
        t.league = "La Liga";
        t.manager = "Carlo Ancelotti";
        t.styleDescription = "Direct Transitional Counter & Individual Flair";
        t.primaryColor = sf::Color(245, 245, 245); // Pure White
        t.secondaryColor = sf::Color(218, 165, 32); // Gold trim
        t.gkColor = sf::Color(26, 188, 156); // Cyan
        t.defaultFormation = FormationType::F_4_3_3;
        t.tacticalStyle = TeamTacticalStyle::CounterAttack;
        t.defensiveMentality = DefensiveMentality::MidBlock;

        t.strengths = {
            "Blistering transitional pace (Mbappe, Vinicius)",
            "Lethal clutch finishing in key moments",
            "Press-resistant progressive midfield (Bellingham, Valverde)"
        };
        t.weaknesses = {
            "Concedes territorial possession to opponents",
            "Gaps can appear between midfield and defense"
        };

        t.passSkillMultiplier = 1.02f;
        t.pressIntensityMultiplier = 0.95f;
        t.counterSpeedMultiplier = 1.22f;
        t.defensiveTackleMultiplier = 1.02f;
        t.setPieceBonus = 1.04f;
        t.offsideLineOffsetM = -1.2f;
        t.widthMultiplier = 1.02f;
        t.staminaDrainMultiplier = 0.95f;

        t.squad = {
            makeGK("Thibaut Courtois", 1, FCIQRole::TraditionalGK, 89, 0.88f, 0.85f, 0.89f, 0.90f, 0.76f),
            makeOutfield("Dani Carvajal", 2, TacticalRole::RB, FCIQRole::AttackingWingback, 86, 81, 56, 79, 80, 82, 81, 0.58f, 0.80f, 0.84f, 0.88f, 0.85f),
            makeOutfield("Eder Militao", 3, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 85, 85, 50, 71, 71, 85, 82, 0.45f, 0.74f, 0.55f, 0.82f, 0.88f),
            makeOutfield("Antonio Rudiger", 22, TacticalRole::CB_LEFT, FCIQRole::Stopper, 88, 82, 55, 71, 69, 86, 86, 0.48f, 0.72f, 0.48f, 0.85f, 0.89f),
            makeOutfield("Ferland Mendy", 23, TacticalRole::LB, FCIQRole::Stopper, 82, 89, 64, 74, 78, 81, 85, 0.52f, 0.72f, 0.74f, 0.80f, 0.86f),
            makeOutfield("Aurelien Tchouameni", 14, TacticalRole::DM, FCIQRole::AnchorCDM, 85, 72, 71, 81, 79, 83, 83, 0.68f, 0.82f, 0.72f, 0.87f, 0.87f),
            makeOutfield("Federico Valverde", 8, TacticalRole::CM_RIGHT, FCIQRole::BoxToBox, 88, 88, 82, 84, 84, 81, 84, 0.82f, 0.86f, 0.82f, 0.88f, 0.84f),
            makeOutfield("Jude Bellingham", 5, TacticalRole::CM_LEFT, FCIQRole::PlaymakerCAM, 90, 80, 87, 83, 88, 78, 83, 0.89f, 0.90f, 0.80f, 0.92f, 0.82f),
            makeOutfield("Rodrygo", 11, TacticalRole::RW, FCIQRole::InsideForward, 86, 89, 81, 81, 87, 32, 64, 0.84f, 0.82f, 0.80f, 0.85f, 0.56f),
            makeOutfield("Kylian Mbappe", 9, TacticalRole::ST, FCIQRole::PoacherST, 91, 97, 90, 80, 92, 36, 78, 0.95f, 0.84f, 0.78f, 0.91f, 0.45f),
            makeOutfield("Vinicius Jr.", 7, TacticalRole::LW, FCIQRole::InsideForward, 90, 95, 84, 81, 91, 29, 69, 0.88f, 0.85f, 0.82f, 0.87f, 0.50f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 3. ARSENAL (Mikel Arteta) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 2;
        t.name = "Arsenal";
        t.shortName = "ARS";
        t.league = "Premier League";
        t.manager = "Mikel Arteta";
        t.styleDescription = "Positional Control & Suffocating Defense";
        t.primaryColor = sf::Color(219, 0, 7); // Arsenal Crimson
        t.secondaryColor = sf::Color(255, 255, 255);
        t.gkColor = sf::Color(46, 204, 113); // Emerald Green
        t.defaultFormation = FormationType::F_4_3_3;
        t.tacticalStyle = TeamTacticalStyle::TikiTaka;
        t.defensiveMentality = DefensiveMentality::HighPress;

        t.strengths = {
            "Premier defensive compactness (Saliba, Gabriel)",
            "Deadly set-piece routines & corner dominance",
            "Intense central duel winning (Rice)"
        };
        t.weaknesses = {
            "Can struggle for breakthrough vs resilient low blocks",
            "Methodical build-up limits direct counter chances"
        };

        t.passSkillMultiplier = 1.04f;
        t.pressIntensityMultiplier = 1.15f;
        t.counterSpeedMultiplier = 1.00f;
        t.defensiveTackleMultiplier = 1.18f;
        t.setPieceBonus = 1.30f;
        t.offsideLineOffsetM = 1.4f;
        t.widthMultiplier = 1.02f;
        t.staminaDrainMultiplier = 1.00f;

        t.squad = {
            makeGK("David Raya", 22, FCIQRole::SweeperKeeper, 83, 0.84f, 0.82f, 0.81f, 0.82f, 0.87f),
            makeOutfield("Ben White", 4, TacticalRole::RB, FCIQRole::InvertedFullback, 84, 77, 42, 78, 78, 83, 76, 0.52f, 0.81f, 0.82f, 0.86f, 0.85f),
            makeOutfield("William Saliba", 2, TacticalRole::CB_RIGHT, FCIQRole::BallPlayingDefender, 87, 82, 40, 72, 73, 87, 83, 0.42f, 0.80f, 0.48f, 0.90f, 0.92f),
            makeOutfield("Gabriel Magalhaes", 6, TacticalRole::CB_LEFT, FCIQRole::Stopper, 86, 69, 40, 66, 66, 86, 84, 0.58f, 0.74f, 0.44f, 0.84f, 0.90f),
            makeOutfield("Jurrien Timber", 12, TacticalRole::LB, FCIQRole::InvertedFullback, 79, 81, 49, 75, 79, 80, 79, 0.54f, 0.78f, 0.76f, 0.85f, 0.84f),
            makeOutfield("Declan Rice", 41, TacticalRole::DM, FCIQRole::AnchorCDM, 87, 76, 72, 82, 79, 86, 86, 0.72f, 0.84f, 0.78f, 0.90f, 0.91f),
            makeOutfield("Thomas Partey", 5, TacticalRole::CM_RIGHT, FCIQRole::DeepLyingPlaymaker, 82, 65, 71, 81, 81, 81, 81, 0.68f, 0.86f, 0.74f, 0.88f, 0.84f),
            makeOutfield("Martin Odegaard", 8, TacticalRole::CM_LEFT, FCIQRole::PlaymakerCAM, 89, 75, 82, 89, 89, 65, 66, 0.82f, 0.95f, 0.88f, 0.92f, 0.72f),
            makeOutfield("Bukayo Saka", 7, TacticalRole::RW, FCIQRole::InsideForward, 87, 86, 83, 83, 87, 65, 76, 0.85f, 0.88f, 0.88f, 0.89f, 0.68f),
            makeOutfield("Kai Havertz", 29, TacticalRole::ST, FCIQRole::PoacherST, 83, 82, 80, 79, 82, 55, 76, 0.85f, 0.84f, 0.72f, 0.86f, 0.64f),
            makeOutfield("Gabriel Martinelli", 11, TacticalRole::LW, FCIQRole::ClassicWinger, 83, 89, 78, 77, 85, 45, 70, 0.80f, 0.78f, 0.82f, 0.82f, 0.56f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 4. LIVERPOOL (Arne Slot) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 3;
        t.name = "Liverpool";
        t.shortName = "LIV";
        t.league = "Premier League";
        t.manager = "Arne Slot";
        t.styleDescription = "High-Intensity Gegenpressing & Vertical Blitz";
        t.primaryColor = sf::Color(200, 16, 46); // All Red
        t.secondaryColor = sf::Color(255, 255, 255);
        t.gkColor = sf::Color(40, 40, 40); // Charcoal
        t.defaultFormation = FormationType::F_4_3_3;
        t.tacticalStyle = TeamTacticalStyle::Gegenpressing;
        t.defensiveMentality = DefensiveMentality::HighPress;

        t.strengths = {
            "Ferocious counter-pressing in opponent third",
            "Explosive vertical wing transitions (Salah, Diaz)",
            "Colossal aerial and physical dominance (Van Dijk)"
        };
        t.weaknesses = {
            "High defensive line vulnerable to through-balls",
            "High physical stamina drain late in matches"
        };

        t.passSkillMultiplier = 1.03f;
        t.pressIntensityMultiplier = 1.25f;
        t.counterSpeedMultiplier = 1.15f;
        t.defensiveTackleMultiplier = 1.05f;
        t.setPieceBonus = 1.10f;
        t.offsideLineOffsetM = 1.6f;
        t.widthMultiplier = 1.08f;
        t.staminaDrainMultiplier = 1.15f;

        t.squad = {
            makeGK("Alisson Becker", 1, FCIQRole::SweeperKeeper, 89, 0.89f, 0.86f, 0.85f, 0.90f, 0.86f),
            makeOutfield("Trent Alexander-Arnold", 66, TacticalRole::RB, FCIQRole::InvertedFullback, 86, 76, 79, 90, 80, 76, 73, 0.74f, 0.96f, 0.95f, 0.88f, 0.76f),
            makeOutfield("Ibrahima Konate", 5, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 83, 78, 34, 62, 67, 83, 82, 0.40f, 0.72f, 0.40f, 0.80f, 0.88f),
            makeOutfield("Virgil van Dijk", 4, TacticalRole::CB_LEFT, FCIQRole::BallPlayingDefender, 89, 78, 60, 72, 72, 89, 86, 0.58f, 0.82f, 0.55f, 0.92f, 0.93f),
            makeOutfield("Andrew Robertson", 26, TacticalRole::LB, FCIQRole::AttackingWingback, 85, 80, 61, 82, 79, 80, 76, 0.58f, 0.80f, 0.88f, 0.84f, 0.84f),
            makeOutfield("Ryan Gravenberch", 38, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 78, 77, 70, 77, 82, 71, 76, 0.72f, 0.85f, 0.76f, 0.88f, 0.82f),
            makeOutfield("Alexis Mac Allister", 10, TacticalRole::CM_RIGHT, FCIQRole::DeepLyingPlaymaker, 86, 70, 80, 85, 84, 78, 78, 0.78f, 0.90f, 0.82f, 0.91f, 0.80f),
            makeOutfield("Dominik Szoboszlai", 8, TacticalRole::CM_LEFT, FCIQRole::BoxToBox, 81, 82, 84, 82, 82, 62, 74, 0.82f, 0.87f, 0.86f, 0.86f, 0.74f),
            makeOutfield("Mohamed Salah", 11, TacticalRole::RW, FCIQRole::InsideForward, 89, 89, 87, 82, 88, 45, 75, 0.92f, 0.90f, 0.85f, 0.90f, 0.52f),
            makeOutfield("Darwin Nunez", 9, TacticalRole::ST, FCIQRole::PoacherST, 82, 90, 81, 71, 76, 42, 86, 0.85f, 0.76f, 0.68f, 0.78f, 0.50f),
            makeOutfield("Luis Diaz", 7, TacticalRole::LW, FCIQRole::InsideForward, 84, 90, 78, 75, 87, 42, 74, 0.83f, 0.80f, 0.78f, 0.84f, 0.55f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 5. BAYER LEVERKUSEN (Xabi Alonso) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 4;
        t.name = "Bayer Leverkusen";
        t.shortName = "B04";
        t.league = "Bundesliga";
        t.manager = "Xabi Alonso";
        t.styleDescription = "Fluid 3-4-2-1 Pocket Overload & High Technicality";
        t.primaryColor = sf::Color(227, 34, 25); // Leverkusen Red
        t.secondaryColor = sf::Color(25, 25, 25); // Matte Black
        t.gkColor = sf::Color(52, 152, 219); // Light Blue
        t.defaultFormation = FormationType::F_3_4_2_1;
        t.tacticalStyle = TeamTacticalStyle::TikiTaka;
        t.defensiveMentality = DefensiveMentality::MidBlock;

        t.strengths = {
            "Central overload combinations between dual 10s (Wirtz)",
            "Attacking wing-back width & scoring (Frimpong, Grimaldo)",
            "Unshakable late-game composure & clutch goals"
        };
        t.weaknesses = {
            "Space conceded behind high wing-backs on transitions",
            "Vulnerable to quick cross-field switches"
        };

        t.passSkillMultiplier = 1.07f;
        t.pressIntensityMultiplier = 1.05f;
        t.counterSpeedMultiplier = 1.08f;
        t.defensiveTackleMultiplier = 1.00f;
        t.setPieceBonus = 1.15f;
        t.offsideLineOffsetM = 0.5f;
        t.widthMultiplier = 1.12f;
        t.staminaDrainMultiplier = 0.96f;

        t.squad = {
            makeGK("Lukas Hradecky", 1, FCIQRole::TraditionalGK, 82, 0.82f, 0.80f, 0.79f, 0.82f, 0.78f),
            makeOutfield("Edmond Tapsoba", 12, TacticalRole::CB_LEFT, FCIQRole::BallPlayingDefender, 83, 78, 52, 73, 72, 83, 80, 0.48f, 0.80f, 0.50f, 0.88f, 0.86f),
            makeOutfield("Jonathan Tah", 4, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 86, 74, 35, 64, 61, 86, 84, 0.44f, 0.74f, 0.42f, 0.86f, 0.89f),
            makeOutfield("Piero Hincapie", 3, TacticalRole::LB, FCIQRole::Stopper, 81, 80, 42, 68, 69, 82, 79, 0.46f, 0.76f, 0.65f, 0.84f, 0.86f),
            makeOutfield("Jeremie Frimpong", 30, TacticalRole::RB, FCIQRole::AttackingWingback, 84, 94, 72, 79, 85, 75, 71, 0.80f, 0.80f, 0.85f, 0.82f, 0.75f),
            makeOutfield("Granit Xhaka", 34, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 86, 52, 75, 86, 77, 80, 82, 0.74f, 0.92f, 0.82f, 0.94f, 0.86f),
            makeOutfield("Robert Andrich", 8, TacticalRole::CM_RIGHT, FCIQRole::AnchorCDM, 82, 68, 73, 76, 73, 81, 84, 0.70f, 0.80f, 0.70f, 0.87f, 0.86f),
            makeOutfield("Alex Grimaldo", 20, TacticalRole::CM_LEFT, FCIQRole::AttackingWingback, 86, 85, 78, 87, 84, 77, 73, 0.84f, 0.90f, 0.92f, 0.88f, 0.78f),
            makeOutfield("Jonas Hofmann", 7, TacticalRole::RW, FCIQRole::InsideForward, 81, 78, 79, 81, 80, 58, 66, 0.81f, 0.84f, 0.82f, 0.86f, 0.64f),
            makeOutfield("Florian Wirtz", 10, TacticalRole::LW, FCIQRole::PlaymakerCAM, 88, 82, 81, 87, 89, 54, 68, 0.88f, 0.95f, 0.86f, 0.94f, 0.62f),
            makeOutfield("Victor Boniface", 22, TacticalRole::ST, FCIQRole::PoacherST, 82, 83, 82, 69, 82, 35, 84, 0.89f, 0.78f, 0.60f, 0.84f, 0.48f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 6. FC BARCELONA (Hansi Flick) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 5;
        t.name = "FC Barcelona";
        t.shortName = "FCB";
        t.league = "La Liga";
        t.manager = "Hansi Flick";
        t.styleDescription = "Aggressive High-Line Press & Direct Attack";
        t.primaryColor = sf::Color(0, 77, 152); // Blaugrana Blue
        t.secondaryColor = sf::Color(165, 0, 68); // Garnet Red
        t.gkColor = sf::Color(230, 126, 34); // Orange
        t.defaultFormation = FormationType::F_4_3_3;
        t.tacticalStyle = TeamTacticalStyle::Gegenpressing;
        t.defensiveMentality = DefensiveMentality::HighPress;

        t.strengths = {
            "Synchronized aggressive high offside trap",
            "Explosive wide isolation dribblers (Yamal, Raphinha)",
            "Clinical box movement & positioning (Lewandowski)"
        };
        t.weaknesses = {
            "Extreme high line can be exploited by fast breaks",
            "Vulnerable in physical aerial duels"
        };

        t.passSkillMultiplier = 1.05f;
        t.pressIntensityMultiplier = 1.25f;
        t.counterSpeedMultiplier = 1.10f;
        t.defensiveTackleMultiplier = 0.98f;
        t.setPieceBonus = 1.06f;
        t.offsideLineOffsetM = 3.2f; // Trademark Hansi Flick aggressive offside trap
        t.widthMultiplier = 1.08f;
        t.staminaDrainMultiplier = 1.12f;

        t.squad = {
            makeGK("Marc-Andre ter Stegen", 1, FCIQRole::SweeperKeeper, 89, 0.86f, 0.86f, 0.85f, 0.86f, 0.88f),
            makeOutfield("Jules Kounde", 23, TacticalRole::RB, FCIQRole::Stopper, 85, 84, 45, 74, 76, 85, 78, 0.50f, 0.78f, 0.82f, 0.85f, 0.88f),
            makeOutfield("Pau Cubarsi", 2, TacticalRole::CB_RIGHT, FCIQRole::BallPlayingDefender, 72, 68, 34, 68, 65, 75, 70, 0.40f, 0.86f, 0.50f, 0.90f, 0.87f),
            makeOutfield("Inigo Martinez", 5, TacticalRole::CB_LEFT, FCIQRole::Stopper, 81, 66, 52, 70, 67, 81, 80, 0.48f, 0.76f, 0.48f, 0.84f, 0.88f),
            makeOutfield("Alejandro Balde", 3, TacticalRole::LB, FCIQRole::AttackingWingback, 81, 91, 50, 73, 78, 75, 67, 0.54f, 0.76f, 0.80f, 0.80f, 0.80f),
            makeOutfield("Marc Casado", 17, TacticalRole::DM, FCIQRole::AnchorCDM, 73, 72, 58, 72, 74, 74, 72, 0.65f, 0.84f, 0.72f, 0.88f, 0.84f),
            makeOutfield("Pedri", 8, TacticalRole::CM_RIGHT, FCIQRole::DeepLyingPlaymaker, 86, 78, 69, 84, 88, 68, 67, 0.80f, 0.96f, 0.82f, 0.95f, 0.78f),
            makeOutfield("Dani Olmo", 20, TacticalRole::CM_LEFT, FCIQRole::PlaymakerCAM, 84, 77, 78, 82, 86, 50, 64, 0.86f, 0.91f, 0.84f, 0.90f, 0.65f),
            makeOutfield("Lamine Yamal", 19, TacticalRole::RW, FCIQRole::InsideForward, 81, 82, 75, 76, 82, 35, 50, 0.85f, 0.92f, 0.89f, 0.90f, 0.52f),
            makeOutfield("Robert Lewandowski", 9, TacticalRole::ST, FCIQRole::PoacherST, 88, 75, 88, 79, 84, 42, 80, 0.95f, 0.82f, 0.65f, 0.92f, 0.48f),
            makeOutfield("Raphinha", 11, TacticalRole::LW, FCIQRole::InsideForward, 84, 91, 80, 81, 85, 53, 73, 0.88f, 0.86f, 0.86f, 0.88f, 0.68f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 7. INTER MILAN (Simone Inzaghi) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 6;
        t.name = "Inter Milan";
        t.shortName = "INT";
        t.league = "Serie A";
        t.manager = "Simone Inzaghi";
        t.styleDescription = "Solid 3-5-2 Rest-Defense & Direct Strike Counters";
        t.primaryColor = sf::Color(0, 87, 156); // Nerazzurri Blue
        t.secondaryColor = sf::Color(17, 17, 17); // Black
        t.gkColor = sf::Color(241, 196, 15); // Yellow
        t.defaultFormation = FormationType::F_3_4_2_1;
        t.tacticalStyle = TeamTacticalStyle::CounterAttack;
        t.defensiveMentality = DefensiveMentality::MidBlock;

        t.strengths = {
            "Ironclad 3-CB defensive foundation (Bastoni, Acerbi, Pavard)",
            "Deadly strike partner combinations (Lautaro, Thuram)",
            "Midfield tactical control & duel strength"
        };
        t.weaknesses = {
            "Vulnerable to fast overlapping fullbacks",
            "Struggles when trailing against low blocks"
        };

        t.passSkillMultiplier = 1.02f;
        t.pressIntensityMultiplier = 1.00f;
        t.counterSpeedMultiplier = 1.15f;
        t.defensiveTackleMultiplier = 1.14f;
        t.setPieceBonus = 1.12f;
        t.offsideLineOffsetM = -0.5f;
        t.widthMultiplier = 1.04f;
        t.staminaDrainMultiplier = 0.95f;

        t.squad = {
            makeGK("Yann Sommer", 1, FCIQRole::TraditionalGK, 87, 0.86f, 0.82f, 0.83f, 0.87f, 0.82f),
            makeOutfield("Benjamin Pavard", 28, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 84, 73, 64, 76, 75, 84, 78, 0.52f, 0.78f, 0.75f, 0.86f, 0.88f),
            makeOutfield("Francesco Acerbi", 15, TacticalRole::CB_LEFT, FCIQRole::Stopper, 84, 54, 50, 65, 60, 86, 81, 0.40f, 0.72f, 0.40f, 0.88f, 0.90f),
            makeOutfield("Alessandro Bastoni", 95, TacticalRole::LB, FCIQRole::BallPlayingDefender, 87, 75, 40, 78, 75, 87, 83, 0.48f, 0.84f, 0.82f, 0.90f, 0.89f),
            makeOutfield("Denzel Dumfries", 2, TacticalRole::RB, FCIQRole::AttackingWingback, 82, 84, 66, 73, 78, 79, 88, 0.68f, 0.74f, 0.82f, 0.80f, 0.80f),
            makeOutfield("Hakan Calhanoglu", 20, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 86, 68, 82, 88, 83, 75, 73, 0.82f, 0.93f, 0.90f, 0.92f, 0.80f),
            makeOutfield("Nicolo Barella", 23, TacticalRole::CM_RIGHT, FCIQRole::BoxToBox, 87, 79, 77, 84, 85, 78, 82, 0.76f, 0.88f, 0.82f, 0.90f, 0.84f),
            makeOutfield("Henrikh Mkhitaryan", 22, TacticalRole::CM_LEFT, FCIQRole::PlaymakerCAM, 83, 74, 78, 81, 83, 67, 68, 0.78f, 0.86f, 0.80f, 0.88f, 0.74f),
            makeOutfield("Federico Dimarco", 32, TacticalRole::LW, FCIQRole::AttackingWingback, 84, 80, 76, 85, 82, 77, 74, 0.76f, 0.88f, 0.94f, 0.86f, 0.79f),
            makeOutfield("Marcus Thuram", 9, TacticalRole::RW, FCIQRole::InsideForward, 83, 87, 79, 74, 82, 45, 82, 0.85f, 0.78f, 0.74f, 0.83f, 0.54f),
            makeOutfield("Lautaro Martinez", 10, TacticalRole::ST, FCIQRole::PoacherST, 89, 81, 88, 76, 86, 48, 84, 0.93f, 0.80f, 0.65f, 0.89f, 0.58f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 8. BAYERN MUNICH (Vincent Kompany) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 7;
        t.name = "Bayern Munich";
        t.shortName = "BAY";
        t.league = "Bundesliga";
        t.manager = "Vincent Kompany";
        t.styleDescription = "Dominant Wing Play & Clinical Box Overload";
        t.primaryColor = sf::Color(220, 5, 45); // Bavarian Red
        t.secondaryColor = sf::Color(255, 255, 255);
        t.gkColor = sf::Color(39, 174, 96); // Green
        t.defaultFormation = FormationType::F_4_2_3_1;
        t.tacticalStyle = TeamTacticalStyle::WingPlay;
        t.defensiveMentality = DefensiveMentality::HighPress;

        t.strengths = {
            "Prolific high shot generation volume",
            "World-class playmaking and finishing (Kane)",
            "Unpredictable 1v1 dribblers (Musiala, Olise)"
        };
        t.weaknesses = {
            "Gaps between center-backs during counter transitions",
            "Prone to central midfield turnovers"
        };

        t.passSkillMultiplier = 1.04f;
        t.pressIntensityMultiplier = 1.12f;
        t.counterSpeedMultiplier = 1.05f;
        t.defensiveTackleMultiplier = 1.02f;
        t.setPieceBonus = 1.12f;
        t.offsideLineOffsetM = 1.5f;
        t.widthMultiplier = 1.15f;
        t.staminaDrainMultiplier = 1.05f;

        t.squad = {
            makeGK("Manuel Neuer", 1, FCIQRole::SweeperKeeper, 86, 0.84f, 0.80f, 0.85f, 0.88f, 0.89f),
            makeOutfield("Sacha Boey", 23, TacticalRole::RB, FCIQRole::AttackingWingback, 79, 87, 54, 71, 77, 76, 79, 0.50f, 0.72f, 0.78f, 0.80f, 0.81f),
            makeOutfield("Dayot Upamecano", 2, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 82, 82, 42, 68, 69, 82, 82, 0.42f, 0.76f, 0.45f, 0.80f, 0.87f),
            makeOutfield("Kim Min-jae", 3, TacticalRole::CB_LEFT, FCIQRole::Stopper, 83, 80, 35, 62, 64, 84, 84, 0.46f, 0.74f, 0.42f, 0.84f, 0.89f),
            makeOutfield("Alphonso Davies", 19, TacticalRole::LB, FCIQRole::AttackingWingback, 82, 95, 68, 77, 84, 75, 76, 0.65f, 0.78f, 0.84f, 0.82f, 0.81f),
            makeOutfield("Joshua Kimmich", 6, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 86, 68, 74, 88, 83, 81, 78, 0.75f, 0.94f, 0.91f, 0.92f, 0.84f),
            makeOutfield("Aleksandar Pavlovic", 16, TacticalRole::CM_RIGHT, FCIQRole::BoxToBox, 75, 70, 64, 76, 75, 74, 74, 0.68f, 0.85f, 0.78f, 0.86f, 0.80f),
            makeOutfield("Jamal Musiala", 42, TacticalRole::CM_LEFT, FCIQRole::PlaymakerCAM, 87, 84, 81, 81, 90, 63, 64, 0.87f, 0.91f, 0.80f, 0.90f, 0.60f),
            makeOutfield("Michael Olise", 17, TacticalRole::RW, FCIQRole::InsideForward, 82, 78, 76, 82, 83, 50, 64, 0.83f, 0.88f, 0.88f, 0.86f, 0.58f),
            makeOutfield("Harry Kane", 9, TacticalRole::ST, FCIQRole::PoacherST, 90, 65, 93, 84, 83, 49, 83, 0.96f, 0.92f, 0.82f, 0.94f, 0.55f),
            makeOutfield("Serge Gnabry", 7, TacticalRole::LW, FCIQRole::InsideForward, 82, 82, 82, 78, 81, 43, 68, 0.84f, 0.80f, 0.80f, 0.82f, 0.52f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 9. PARIS SAINT-GERMAIN (Luis Enrique) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 8;
        t.name = "Paris Saint-Germain";
        t.shortName = "PSG";
        t.league = "Ligue 1";
        t.manager = "Luis Enrique";
        t.styleDescription = "Positional Width & Explosive 1v1 Dribbling";
        t.primaryColor = sf::Color(0, 28, 63); // Midnight Navy
        t.secondaryColor = sf::Color(218, 41, 28); // Red
        t.gkColor = sf::Color(142, 68, 173); // Purple
        t.defaultFormation = FormationType::F_4_3_3;
        t.tacticalStyle = TeamTacticalStyle::WingPlay;
        t.defensiveMentality = DefensiveMentality::MidBlock;

        t.strengths = {
            "Explosive wide isolation dribblers (Dembélé, Barcola)",
            "Technical midfield circulation (Vitinha, Ruiz)",
            "Rapid recovery pace across the pitch"
        };
        t.weaknesses = {
            "Lack of traditional physical box forward",
            "Defensive tracking lapses on opponent fullbacks"
        };

        t.passSkillMultiplier = 1.05f;
        t.pressIntensityMultiplier = 1.08f;
        t.counterSpeedMultiplier = 1.16f;
        t.defensiveTackleMultiplier = 0.96f;
        t.setPieceBonus = 1.02f;
        t.offsideLineOffsetM = 1.0f;
        t.widthMultiplier = 1.15f;
        t.staminaDrainMultiplier = 0.98f;

        t.squad = {
            makeGK("Gianluigi Donnarumma", 1, FCIQRole::TraditionalGK, 89, 0.89f, 0.90f, 0.82f, 0.85f, 0.72f),
            makeOutfield("Achraf Hakimi", 2, TacticalRole::RB, FCIQRole::AttackingWingback, 84, 92, 75, 79, 80, 76, 78, 0.74f, 0.82f, 0.86f, 0.84f, 0.79f),
            makeOutfield("Marquinhos", 5, TacticalRole::CB_RIGHT, FCIQRole::BallPlayingDefender, 87, 78, 56, 75, 74, 89, 80, 0.48f, 0.78f, 0.50f, 0.88f, 0.90f),
            makeOutfield("Willian Pacho", 51, TacticalRole::CB_LEFT, FCIQRole::Stopper, 80, 78, 35, 60, 62, 81, 81, 0.38f, 0.72f, 0.42f, 0.82f, 0.87f),
            makeOutfield("Nuno Mendes", 25, TacticalRole::LB, FCIQRole::AttackingWingback, 83, 89, 61, 75, 80, 77, 76, 0.60f, 0.78f, 0.82f, 0.82f, 0.81f),
            makeOutfield("Vitinha", 17, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 85, 75, 74, 85, 86, 72, 69, 0.76f, 0.92f, 0.80f, 0.94f, 0.78f),
            makeOutfield("Warren Zaire-Emery", 33, TacticalRole::CM_RIGHT, FCIQRole::BoxToBox, 80, 78, 68, 78, 80, 76, 79, 0.72f, 0.84f, 0.78f, 0.87f, 0.81f),
            makeOutfield("Fabian Ruiz", 8, TacticalRole::CM_LEFT, FCIQRole::PlaymakerCAM, 82, 65, 76, 83, 82, 71, 71, 0.80f, 0.90f, 0.82f, 0.90f, 0.74f),
            makeOutfield("Ousmane Dembélé", 10, TacticalRole::RW, FCIQRole::ClassicWinger, 86, 93, 77, 81, 89, 36, 56, 0.78f, 0.86f, 0.88f, 0.85f, 0.48f),
            makeOutfield("Marco Asensio", 11, TacticalRole::ST, FCIQRole::PoacherST, 81, 78, 81, 81, 81, 45, 64, 0.86f, 0.84f, 0.78f, 0.86f, 0.52f),
            makeOutfield("Bradley Barcola", 29, TacticalRole::LW, FCIQRole::InsideForward, 80, 89, 73, 76, 83, 42, 62, 0.82f, 0.80f, 0.80f, 0.82f, 0.50f)
        };
        teams.push_back(t);
    }

    // =========================================================================
    // 10. ATLETICO MADRID (Diego Simeone) - EA FC 25 Squad
    // =========================================================================
    {
        TeamProfile t;
        t.id = 9;
        t.name = "Atletico Madrid";
        t.shortName = "ATM";
        t.league = "La Liga";
        t.manager = "Diego Simeone";
        t.styleDescription = "Resilient Low Block & Aggressive Counter Duels";
        t.primaryColor = sf::Color(203, 53, 36); // Rojiblanco Red
        t.secondaryColor = sf::Color(255, 255, 255);
        t.gkColor = sf::Color(44, 62, 80); // Dark Slate
        t.defaultFormation = FormationType::F_4_4_2;
        t.tacticalStyle = TeamTacticalStyle::CounterAttack;
        t.defensiveMentality = DefensiveMentality::LowBlock;

        t.strengths = {
            "Impenetrable penalty box defending & shot blocking",
            "Relentless duel winning, slide tackling & discipline",
            "Clinical counter-attack conversion (Griezmann, Alvarez)"
        };
        t.weaknesses = {
            "Low overall possession share",
            "Reduced attacking volume when required to dictate play"
        };

        t.passSkillMultiplier = 0.97f;
        t.pressIntensityMultiplier = 0.90f;
        t.counterSpeedMultiplier = 1.18f;
        t.defensiveTackleMultiplier = 1.25f;
        t.setPieceBonus = 1.12f;
        t.offsideLineOffsetM = -3.5f; // Deep low block
        t.widthMultiplier = 0.95f;
        t.staminaDrainMultiplier = 0.92f;

        t.squad = {
            makeGK("Jan Oblak", 13, FCIQRole::TraditionalGK, 88, 0.88f, 0.86f, 0.89f, 0.89f, 0.72f),
            makeOutfield("Nahuel Molina", 16, TacticalRole::RB, FCIQRole::AttackingWingback, 81, 84, 64, 75, 78, 76, 75, 0.65f, 0.76f, 0.82f, 0.80f, 0.80f),
            makeOutfield("Robin Le Normand", 24, TacticalRole::CB_RIGHT, FCIQRole::Stopper, 82, 68, 40, 66, 62, 83, 80, 0.44f, 0.72f, 0.40f, 0.85f, 0.88f),
            makeOutfield("Jose Maria Gimenez", 2, TacticalRole::CB_LEFT, FCIQRole::Stopper, 83, 70, 45, 58, 55, 84, 83, 0.40f, 0.70f, 0.38f, 0.86f, 0.90f),
            makeOutfield("Reinildo Mandava", 23, TacticalRole::LB, FCIQRole::Stopper, 80, 80, 46, 66, 68, 80, 81, 0.42f, 0.72f, 0.68f, 0.82f, 0.86f),
            makeOutfield("Rodrigo De Paul", 5, TacticalRole::CM_RIGHT, FCIQRole::BoxToBox, 84, 75, 77, 83, 82, 78, 82, 0.76f, 0.86f, 0.80f, 0.88f, 0.82f),
            makeOutfield("Koke", 6, TacticalRole::DM, FCIQRole::DeepLyingPlaymaker, 83, 62, 72, 84, 79, 79, 78, 0.70f, 0.90f, 0.82f, 0.92f, 0.84f),
            makeOutfield("Conor Gallagher", 4, TacticalRole::CM_LEFT, FCIQRole::BoxToBox, 81, 78, 74, 77, 78, 78, 82, 0.74f, 0.80f, 0.74f, 0.85f, 0.84f),
            makeOutfield("Samuel Lino", 12, TacticalRole::RW, FCIQRole::ClassicWinger, 80, 85, 76, 74, 82, 55, 68, 0.78f, 0.78f, 0.80f, 0.82f, 0.64f),
            makeOutfield("Antoine Griezmann", 7, TacticalRole::LW, FCIQRole::PlaymakerCAM, 88, 80, 88, 87, 87, 62, 72, 0.90f, 0.93f, 0.86f, 0.92f, 0.72f),
            makeOutfield("Julian Alvarez", 19, TacticalRole::ST, FCIQRole::PoacherST, 84, 84, 84, 79, 83, 54, 77, 0.90f, 0.84f, 0.76f, 0.88f, 0.62f)
        };
        teams.push_back(t);
    }

    return teams;
}

} // anonymous namespace

const std::vector<TeamProfile>& TeamDatabase::getAllTeams() {
    static const std::vector<TeamProfile> s_teams = buildDatabase();
    return s_teams;
}

const TeamProfile& TeamDatabase::getTeam(int index) {
    const auto& teams = getAllTeams();
    if (teams.empty()) {
        static TeamProfile fallback;
        return fallback;
    }
    int clamped = ((index % static_cast<int>(teams.size())) + static_cast<int>(teams.size())) % static_cast<int>(teams.size());
    return teams[clamped];
}

int TeamDatabase::getTeamCount() {
    return static_cast<int>(getAllTeams().size());
}

int TeamDatabase::findTeamIndex(const std::string& query) {
    const auto& teams = getAllTeams();
    for (size_t i = 0; i < teams.size(); ++i) {
        if (teams[i].shortName == query || teams[i].name == query) {
            return static_cast<int>(i);
        }
    }
    return 0;
}
