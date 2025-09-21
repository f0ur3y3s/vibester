// AIConfig.h
#pragma once

struct DifficultySettings {
    float reactionTimeBase;      // Base reaction time in frames (higher = slower)
    float reactionTimeVariance;  // Random variance in reaction time (higher = more inconsistent)
    float decisionQuality;       // Quality of decision making (higher = smarter decisions)
    float executionPrecision;    // Precision of input execution (higher = more precise)
    float riskTolerance;         // Willingness to take risks (higher = more aggressive)
    float comboProficiency;      // Ability to execute combos (higher = better combos)
    float adaptability;          // How quickly AI adapts to player patterns (higher = faster adaptation)
    float recoverySkill;         // Skill at recovering from being knocked off stage (higher = better recovery)
    float techSkill;             // Technical skill for advanced techniques (higher = better tech)

    // Initialize with specific difficulty presets
    DifficultySettings(float difficulty = 0.8f) {
        if (difficulty <= 0.25f) {
            // EASY - Very forgiving, slow reactions, poor decisions
            reactionTimeBase = 25.0f;           // Very slow reactions
            reactionTimeVariance = 15.0f;       // Very inconsistent
            decisionQuality = 0.3f;             // Makes poor decisions
            executionPrecision = 0.3f;          // Misses inputs often
            riskTolerance = 0.2f;               // Very cautious
            comboProficiency = 0.1f;            // Almost no combos
            adaptability = 0.1f;                // Doesn't adapt
            recoverySkill = 0.2f;               // Often fails recovery
            techSkill = 0.1f;                   // No advanced techniques
        }
        else if (difficulty <= 0.5f) {
            // MEDIUM - Average player level
            reactionTimeBase = 15.0f;           // Moderate reactions
            reactionTimeVariance = 8.0f;        // Somewhat inconsistent
            decisionQuality = 0.5f;             // Makes decent decisions
            executionPrecision = 0.6f;          // Occasionally misses inputs
            riskTolerance = 0.4f;               // Balanced risk-taking
            comboProficiency = 0.4f;            // Basic 2-3 hit combos
            adaptability = 0.4f;                // Slow adaptation
            recoverySkill = 0.5f;               // Average recovery
            techSkill = 0.3f;                   // Few advanced techniques
        }
        else if (difficulty <= 0.8f) {
            // HARD - Skilled player level
            reactionTimeBase = 8.0f;            // Quick reactions
            reactionTimeVariance = 4.0f;        // More consistent
            decisionQuality = 0.8f;             // Makes good decisions
            executionPrecision = 0.8f;          // Rarely misses inputs
            riskTolerance = 0.6f;               // Takes calculated risks
            comboProficiency = 0.7f;            // Good combos
            adaptability = 0.7f;                // Good adaptation
            recoverySkill = 0.8f;               // Strong recovery
            techSkill = 0.7f;                   // Several advanced techniques
        }
        else {
            // EXPERT - Tournament player level
            reactionTimeBase = 3.0f;            // Frame-perfect reactions
            reactionTimeVariance = 1.0f;        // Extremely consistent
            decisionQuality = 0.95f;            // Makes optimal decisions
            executionPrecision = 0.95f;         // Almost never misses inputs
            riskTolerance = 0.8f;               // Takes optimal risks
            comboProficiency = 0.95f;           // Advanced combos
            adaptability = 0.9f;                // Rapid adaptation
            recoverySkill = 0.95f;              // Expert recovery
            techSkill = 0.95f;                  // All advanced techniques
        }
    }
};

struct BehaviorSettings {
    float centerControlImportance;
    float offenseDefenseBalance;
    float aggressionLevel;
    float adaptabilityLevel;

    BehaviorSettings() :
        centerControlImportance(0.7f),
        offenseDefenseBalance(0.5f),
        aggressionLevel(0.5f),
        adaptabilityLevel(0.5f) {}
};

struct CombatSettings {
    float comboFollowUpThreshold;
    float pummleThreshold;
    float grabAttemptRate;
    float shieldFrequency;

    CombatSettings() :
        comboFollowUpThreshold(0.7f),
        pummleThreshold(0.5f),
        grabAttemptRate(0.3f),
        shieldFrequency(0.4f) {}
};

struct AIConfig {
    DifficultySettings difficulty;
    BehaviorSettings behavior;
    CombatSettings combat;

    AIConfig(float difficultyLevel = 0.8f) :
        difficulty(difficultyLevel) {}

    void SetDifficulty(float difficultyLevel) {
        difficulty = DifficultySettings(difficultyLevel);
    }
};
